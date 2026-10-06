#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>

#include "rdb/faccbindev.hpp"
#include "retractor/lib/appConfig.hpp"
#include "retractor/lib/bus.hpp"
#include "retractor/lib/compiler.hpp"
#include "retractor/lib/CRSMath.hpp"
#include "retractor/lib/dataModel.hpp"
#include "retractor/lib/executorsmState.hpp"
#include "retractor/lib/lockManager.hpp"
#include "retractor/lib/qTree.hpp"
#include "retractor/lib/RQLParser.hpp"

// Wszystko, co dolacza executorsm.hpp, jest wyzej - pod makrem przechodzi wylacznie cialo klasy.
#define private public
#include "retractor/lib/executorsm.hpp"
#undef private

// Regula dolaczana ad-hoc bez zegara i IPC: attachAdHocRule wolane wprost miedzy zapisami
// strumienia, wiec granica historii jest znana co do rekordu. Strumien MEMORY ma pierscien
// o rozmiarze H+1 - najmniejszym, ktory obsluzy zakres siegajacy H rekordow wstecz.
class ExecutorsmAdHocRuleTest : public ::testing::TestWithParam<size_t> {
 protected:
  std::filesystem::path sandBoxFolder = std::filesystem::temp_directory_path() / "test_executorsmAdHoc_files";
  qTree plan;
  std::unique_ptr<dataModel> model;

  void SetUp() override {
    if (std::filesystem::is_directory(sandBoxFolder)) std::filesystem::remove_all(sandBoxFolder);
    std::filesystem::create_directories(sandBoxFolder);
    const auto inputFile = sandBoxFolder / "input.bin";
    std::ofstream(inputFile, std::ios::binary).write("\0\0\0\0", 4);
    const auto [status, keyword, streamName] = parserRQLString(
        plan, "STORAGE '" + sandBoxFolder.string() + "'\nDECLARE value INTEGER STREAM src, 1 BINFILE '" + inputFile.string() +
                  "'\nSELECT src[0] STREAM result FROM src RETENTION " + std::to_string(GetParam() + 1) + " STORAGE MEMORY\n");
    ASSERT_EQ(status, "OK");
    compiler compilePlan(plan);
    ASSERT_EQ(compilePlan.compile(), "OK");
    model                       = std::make_unique<dataModel>(plan);
    pProc                       = model.get();
    executorsm::coreInstancePtr = &plan;
  }

  void TearDown() override {
    executorsm::coreInstancePtr = nullptr;
    pProc                       = nullptr;
    model.reset();
    if (std::filesystem::is_directory(sandBoxFolder)) std::filesystem::remove_all(sandBoxFolder);
  }
};

TEST_P(ExecutorsmAdHocRuleTest, attached_rule_first_dump_contains_only_records_after_attachment) {
  auto &runtime = *model->qSet.at("result");
  auto &output  = *runtime.outputPayload;
  for (int value = 10; value <= 12; ++value) {
    output.getPayload()->setItem(0, value);
    static_cast<void>(output.write());
  }

  const auto historyDepth                  = static_cast<int>(GetParam());
  qTree copy                               = plan;
  const auto [status, keyword, streamName] = parserRQLString(
      copy, "RULE guard ON result WHEN result[0] > 0 DO DUMP " + std::to_string(-historyDepth) + " TO 1 RETENTION 4");
  ASSERT_EQ(status, "OK");
  ASSERT_EQ(executorsm::attachAdHocRule(copy, "result").get<std::string>("db"), "OK");

  const auto firstDump = sandBoxFolder / "result_guard_dump_0.tmp";
  for (int value = 13; value < 13 + historyDepth; ++value) {
    output.getPayload()->setItem(0, value);
    static_cast<void>(output.write());
    runtime.constructRulesAndUpdate(plan.getQuery("result"));
    EXPECT_FALSE(std::filesystem::exists(firstDump)) << "Regula wyzwolona przed zgromadzeniem wlasnej historii";
  }

  output.getPayload()->setItem(0, 13 + historyDepth);
  static_cast<void>(output.write());
  runtime.constructRulesAndUpdate(plan.getQuery("result"));
  std::ifstream dump(firstDump, std::ios::binary);
  ASSERT_TRUE(dump.is_open()) << "Regula nie wyzwolona przy pierwszym kompletnym oknie po dolaczeniu";
  for (int expectedValue = 13; expectedValue <= 13 + historyDepth; ++expectedValue) {
    std::int32_t actualValue = 0;
    dump.read(reinterpret_cast<char *>(&actualValue), sizeof(actualValue));
    ASSERT_EQ(dump.gcount(), sizeof(actualValue));
    EXPECT_EQ(actualValue, expectedValue);
  }
  EXPECT_EQ(dump.peek(), std::ifstream::traits_type::eof());
}

// #380: regula ad-hoc, ktorej plik zrzutu pokrywa sie z plikiem reguly dolaczonej wczesniej -
// tu Guard i guard na result, jeden plik na systemie nieczulym na wielkosc liter - dostaje odmowe
// parsera, a zywy plan zostaje bez niej.
TEST_P(ExecutorsmAdHocRuleTest, attached_rule_sharing_a_dump_file_is_refused) {
  testing::internal::CaptureStderr();
  ASSERT_EQ(executorsm::getAdHoc("RULE Guard ON result WHEN result[0] > 0 DO DUMP 0 TO 1").get<std::string>("db"), "OK");
  const auto reply = executorsm::getAdHoc("RULE guard ON result WHEN result[0] > 1 DO DUMP 0 TO 1").get<std::string>("db");
  testing::internal::GetCapturedStderr();
  EXPECT_NE(reply.find("would write the same dump file"), std::string::npos) << reply;
  ASSERT_EQ(plan.getQuery("result").lRules.size(), 1U);
  EXPECT_EQ(plan.getQuery("result").lRules.front().name, "Guard");
}

TEST_P(ExecutorsmAdHocRuleTest, attached_rule_rejects_history_equal_to_memory_capacity) {
  const auto capacity                      = plan.getQuery("result").policy.second;
  qTree copy                               = plan;
  const auto [status, keyword, streamName] = parserRQLString(
      copy, "RULE guard ON result WHEN result[0] > 0 DO DUMP -" + std::to_string(capacity) + " TO 1 RETENTION 4");
  ASSERT_EQ(status, "OK");
  const auto reply = executorsm::attachAdHocRule(copy, "result").get<std::string>("db");
  EXPECT_TRUE(reply.starts_with("Rejected:")) << reply;
  EXPECT_NE(reply.find("needs " + std::to_string(capacity + 1)), std::string::npos) << reply;
  EXPECT_TRUE(plan.getQuery("result").lRules.empty());
}

INSTANTIATE_TEST_SUITE_P(BoundarySizes, ExecutorsmAdHocRuleTest, ::testing::Values(size_t{0}, size_t{1}, size_t{2}));
