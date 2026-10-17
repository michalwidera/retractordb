#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>

#include "logCapture.hpp"
#include "rdb/faccbindev.hpp"
#include "rdbResult.hpp"
#include "retractor/lib/appConfig.hpp"
#include "retractor/lib/bus.hpp"
#include "retractor/lib/compiler.hpp"
#include "retractor/lib/CRSMath.hpp"
#include "retractor/lib/dataModel.hpp"
#include "retractor/lib/executorsmState.hpp"
#include "retractor/lib/lockManager.hpp"
#include "retractor/lib/planSource.hpp"
#include "retractor/lib/qTree.hpp"
#include "retractor/lib/RQLParser.hpp"

// Wszystko, co dolacza executorsm.hpp, jest wyzej - pod makrem przechodzi wylacznie cialo klasy.
#define private public
#include "retractor/lib/executorsm.hpp"
#undef private

// Kontrakt etapu 9b (#521): dyspozytor komend i kanal `reset-*` wolane wprost, bez IPC i bez petli
// slotow. Model ma dwa zapisane rekordy strumienia `result`, wiec odpowiedzi `get` i `detail` maja
// znane wartosci.
class ExecutorsmCommandsTest : public ::testing::Test {
 protected:
  std::filesystem::path sandBoxFolder = std::filesystem::temp_directory_path() / "test_executorsmCommands_files";
  qTree plan;
  std::unique_ptr<dataModel> model;

  [[nodiscard]] std::string planText(const std::string &extra = {}) const {
    return "STORAGE '" + sandBoxFolder.string() + "'\nDECLARE value INTEGER STREAM src, 1 BINFILE '" +
           (sandBoxFolder / "input.bin").string() + "'\nSELECT src[0] STREAM result FROM src RETENTION 4 STORAGE MEMORY\n" +
           extra;
  }

  static boost::property_tree::ptree command(const std::string &name, const std::string &argument = {},
                                             const std::string &id = {}) {
    boost::property_tree::ptree request;
    request.put("db.message", name);
    if (!argument.empty()) request.put("db.argument", argument);
    if (!id.empty()) request.put("db.id", id);
    return request;
  }

  static std::string reply(const boost::property_tree::ptree &response) { return response.get("db", ""); }
  static std::string error(const boost::property_tree::ptree &response) { return response.get("error.response", ""); }

  void SetUp() override {
    std::filesystem::remove_all(sandBoxFolder);
    std::filesystem::create_directories(sandBoxFolder);
    std::ofstream(sandBoxFolder / "input.bin", std::ios::binary).write("\0\0\0\0", 4);
    ASSERT_EQ(parsePlanText(plan, planText()).status, "OK");
    ASSERT_EQ(compiler(plan).compile(), "OK");
    model        = rdbtest::ok(dataModel::create(plan));
    auto &output = *model->qSet.at("result")->outputPayload;
    for (int value : {5, 6}) {
      output.getPayload()->setItem(0, value);
      ASSERT_EQ(rdbtest::ok(output.write()), rdb::WriteStatus::Ok);
    }
    pProc                       = model.get();
    executorsm::coreInstancePtr = &plan;
    esm::dataModelExpected      = false;
    iLoopLimitCnt               = executorsm::inifitie_loop;
  }

  void TearDown() override {
    for (int id = 0; id <= 20; ++id)  // domkniecie transferow: niekompletny commit usuwa wpis
      static_cast<void>(executorsm::resetCommit(command("reset-commit", {}, std::to_string(id))));
    esm::planSwapInFlight   = false;
    esm::planResetRequested = false;
    esm::pendingPlanText.clear();
    executorsm::cfgUnrestricted = false;
    iLoopLimitCnt               = executorsm::inifitie_loop;
    executorsm::coreInstancePtr = nullptr;
    pProc                       = nullptr;
    model.reset();
    std::filesystem::remove_all(sandBoxFolder);
  }
};

TEST_F(ExecutorsmCommandsTest, hello_answers_world) {
  // covers: executorsm.executorsm.9~1
  EXPECT_EQ(reply(executorsm::commandProcessor(command("hello"))), "world");
}

// get: kazdy wezel planu z okresem, rozmiarem w bajtach, liczba rekordow, lokalizacja i pojemnoscia.
TEST_F(ExecutorsmCommandsTest, get_lists_every_stream_with_its_parameters) {
  // covers: executorsm.executorsm.9~1
  const auto response = executorsm::commandProcessor(command("get"));
  EXPECT_EQ(response.get("db.stream.result", ""), "result");
  EXPECT_EQ(response.get("db.stream.result.duration", ""), "1");
  EXPECT_EQ(response.get("db.stream.result.count", ""), "2");
  EXPECT_EQ(response.get("db.stream.result.size", ""), "8");  // 2 rekordy po 4 B
  EXPECT_EQ(response.get("db.stream.result.cap", -1), plan.maxCapacity["result"]);
  EXPECT_EQ(response.get("db.stream.src", ""), "src");
  EXPECT_EQ(response.get("db.stream.src.size", ""), "-1");  // deklaracja nie ma rozmiaru
}

// detail: schemat, okres, lokalizacja, pojemnosc, liczba rekordow i zapytanie jednego strumienia.
TEST_F(ExecutorsmCommandsTest, detail_describes_schema_period_location_capacity_records_and_query) {
  // covers: executorsm.executorsm.9~1
  const auto response  = executorsm::commandProcessor(command("detail", "result"));
  const auto fieldName = plan.getQuery("result").lSchema.front().field_.rname;
  EXPECT_EQ(response.get("db.stream", ""), "result");
  EXPECT_EQ(response.get("db.field." + fieldName, ""), fieldName);
  EXPECT_EQ(response.get("db.field_type." + fieldName, ""), "INTEGER");
  EXPECT_EQ(response.get("db.count", ""), "1");
  EXPECT_EQ(response.get("db.duration", ""), "1");
  EXPECT_EQ(response.get("db.location", "?"), plan.getQuery("result").filename);
  EXPECT_EQ(response.get("db.cap", -1), plan.maxCapacity["result"]);
  EXPECT_EQ(response.get("db.size", ""), "8");
  EXPECT_EQ(response.get("db.count_records", ""), "2");
  EXPECT_EQ(response.get("db.is_declaration", ""), "false");
  EXPECT_EQ(response.get("db.query", ""), std::to_string(plan.getQuery("result").lProgram.size()) + " tokens");
}

// Nazwa spoza planu to odpowiedz z bledem i nazwa, a nie koniec procesu.
TEST_F(ExecutorsmCommandsTest, unknown_stream_is_an_error_response_naming_it) {
  // covers: executorsm.executorsm.10~1
  EXPECT_EQ(error(executorsm::commandProcessor(command("detail", "ghost"))), "unknown stream: ghost");
  EXPECT_EQ(error(executorsm::commandProcessor(command("show", "ghost", "7"))), "unknown stream: ghost");
  EXPECT_EQ(reply(executorsm::commandProcessor(command("hello"))), "world");
}

// Bez modelu komendy, ktore go wymagaja, dostaja odpowiedz braku planu; w trakcie zamykania -
// odpowiedz zamykania serwera.
TEST_F(ExecutorsmCommandsTest, commands_needing_a_model_answer_no_plan_or_stopping) {
  // covers: executorsm.executorsm.10~1
  pProc = nullptr;
  for (const char *name : {"get", "detail", "show", "adhoc"})
    EXPECT_EQ(reply(executorsm::commandProcessor(command(name, "result", "7"))), "no active plan") << name;
  iLoopLimitCnt = executorsm::stop_now;
  for (const char *name : {"get", "detail", "show", "adhoc"})
    EXPECT_EQ(reply(executorsm::commandProcessor(command(name, "result", "7"))), "server stopping") << name;
}

// Wyjatek w obsludze komendy (tu: identyfikator klienta, ktory nie jest liczba) wraca jako
// error.response, a nastepna komenda jest obslugiwana. Zlamany niezmiennik (brak planu w
// dyspozytorze) daje "engine error" i wpis krytyczny w logu.
TEST_F(ExecutorsmCommandsTest, handler_failures_become_error_responses) {
  // covers: executorsm.executorsm.12~1
  EXPECT_THAT(error(executorsm::commandProcessor(command("show", "result", "not-a-number"))),
              ::testing::StartsWith("command processor failure: "));
  EXPECT_EQ(reply(executorsm::commandProcessor(command("hello"))), "world");

  const LogCapture log;
  executorsm::coreInstancePtr = nullptr;
  EXPECT_THAT(error(executorsm::commandProcessor(command("get"))), ::testing::StartsWith("engine error: "));
  EXPECT_THAT(log.text(), ::testing::HasSubstr("critical"));
  EXPECT_THAT(log.text(), ::testing::HasSubstr("Engine invariant violated while handling command 'get'"));
}

TEST_F(ExecutorsmCommandsTest, kill_stops_the_loop) {
  // covers: executorsm.executorsm.9~1
  static_cast<void>(executorsm::commandProcessor(command("kill")));
  EXPECT_EQ(iLoopLimitCnt.load(), executorsm::stop_now);
}

// Limity kanalu reset: liczba porcji 0..512, najwyzej 8 transferow naraz (wznowienie wlasnego
// sie nie liczy), porcja ponad zapowiedz i tekst ponad 204800 B koncza transfer odmowa.
TEST_F(ExecutorsmCommandsTest, reset_transfer_limits) {
  // covers: executorsm.executorsm.9~1, executorsm.executorsm.14~1
  const auto resetBegin  = [](const boost::property_tree::ptree &request) { return executorsm::resetBegin(request); };
  const auto resetChunk  = [](const boost::property_tree::ptree &request) { return executorsm::resetChunk(request); };
  const auto resetCommit = [](const boost::property_tree::ptree &request) { return executorsm::resetCommit(request); };
  EXPECT_THAT(reply(resetBegin(command("reset-begin", "-1", "1"))), ::testing::HasSubstr("chunk count out of range"));
  EXPECT_THAT(reply(resetBegin(command("reset-begin", "513", "1"))), ::testing::HasSubstr("chunk count out of range"));
  EXPECT_EQ(reply(resetBegin(command("reset-begin", "512", "1"))), "OK");
  for (int id = 2; id <= 8; ++id)
    EXPECT_EQ(reply(resetBegin(command("reset-begin", "1", std::to_string(id)))), "OK") << id;
  EXPECT_THAT(reply(resetBegin(command("reset-begin", "1", "9"))), ::testing::HasSubstr("too many plan transfers"));
  EXPECT_EQ(reply(resetBegin(command("reset-begin", "1", "8"))), "OK");  // ten sam klient zaczyna od nowa

  EXPECT_THAT(reply(resetChunk(command("reset-chunk", "x", "10"))), ::testing::HasSubstr("no plan transfer in progress"));
  EXPECT_EQ(reply(resetChunk(command("reset-chunk", "x", "2"))), "OK");
  EXPECT_THAT(reply(resetChunk(command("reset-chunk", "y", "2"))), ::testing::HasSubstr("more chunks than the announced 1"));
  EXPECT_THAT(reply(resetCommit(command("reset-commit", {}, "2"))), ::testing::HasSubstr("no plan transfer in progress"));

  const std::string slab(200000, 'x');
  EXPECT_EQ(reply(resetChunk(command("reset-chunk", slab, "1"))), "OK");
  EXPECT_THAT(reply(resetChunk(command("reset-chunk", std::string(4801, 'x'), "1"))),
              ::testing::HasSubstr("plan text exceeds 204800 bytes"));

  EXPECT_EQ(reply(resetBegin(command("reset-begin", "2", "3"))), "OK");
  EXPECT_EQ(reply(resetChunk(command("reset-chunk", "a", "3"))), "OK");
  EXPECT_THAT(reply(resetCommit(command("reset-commit", {}, "3"))), ::testing::HasSubstr("incomplete transfer (1 of 2)"));
}

// reset-commit sprawdza plan jak przy starcie i nie rusza planu dzialajacego: odmowa parsowania,
// kompilacji i DO SYSTEM (bez service.unrestricted) zostawia plan i nie zglasza wymiany; plan
// przyjety czeka w esm::pendingPlanText, a kolejny commit w trakcie wymiany jest odrzucany.
TEST_F(ExecutorsmCommandsTest, reset_commit_validates_without_touching_the_running_plan) {
  // covers: executorsm.executorsm.14~1, executorsm.executorsm.15~1
  const auto commit = [&](const std::string &text, const std::string &id) {
    EXPECT_EQ(reply(executorsm::resetBegin(command("reset-begin", "1", id))), "OK");
    EXPECT_EQ(reply(executorsm::resetChunk(command("reset-chunk", text, id))), "OK");
    return reply(executorsm::resetCommit(command("reset-commit", {}, id)));
  };
  const auto runningRevision = plan.planRevision();
  const auto runningSize     = plan.size();
  const std::string withSystem =
      planText("RULE shell ON result WHEN result[0] > 0 DO SYSTEM 'touch " + (sandBoxFolder / "fired").string() + "'\n");

  EXPECT_THAT(commit("SELECT FROM", "1"), ::testing::StartsWith("Fail parse:"));
  // Kolizja nazw substratow (#474) przechodzi parser i odpada dopiero w kompilacji.
  const std::string input     = "BINFILE '" + (sandBoxFolder / "input.bin").string() + "'\n";
  const std::string collision = "STORAGE '" + sandBoxFolder.string() + "'\nDECLARE v INTEGER STREAM A_B, 1/100 " + input +
                                "DECLARE v INTEGER STREAM C, 1/100 " + input + "DECLARE v INTEGER STREAM A, 1/50 " + input +
                                "DECLARE v INTEGER STREAM B_C, 1/50 " + input +
                                "SELECT * STREAM first FROM (A_B + C)>1 STORAGE MEMORY\n"
                                "SELECT * STREAM second FROM (A + B_C)>1 STORAGE MEMORY\n";
  EXPECT_EQ(commit(collision, "2"), "Fail compile:Substrate name 'STREAM_ADD_A_B_C' denotes two different programs");
  EXPECT_THAT(commit(withSystem, "3"), ::testing::HasSubstr("uses DO SYSTEM"));
  EXPECT_FALSE(esm::planResetRequested.load());
  EXPECT_FALSE(esm::planSwapInFlight.load());
  EXPECT_TRUE(esm::pendingPlanText.empty());

  executorsm::cfgUnrestricted = true;
  EXPECT_EQ(commit(withSystem, "4"), "OK");
  EXPECT_TRUE(esm::planResetRequested.load());
  EXPECT_TRUE(esm::planSwapInFlight.load());
  EXPECT_EQ(esm::pendingPlanText, withSystem);
  EXPECT_THAT(commit(planText(), "5"), ::testing::HasSubstr("a plan reload is already in progress"));

  EXPECT_EQ(plan.planRevision(), runningRevision);
  EXPECT_EQ(plan.size(), runningSize);
  EXPECT_FALSE(std::filesystem::exists(sandBoxFolder / "fired"));
}
