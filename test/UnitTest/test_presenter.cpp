#include <gtest/gtest.h>

#include <boost/program_options.hpp>
#include <boost/system/error_code.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "retractor/lib/presenter.hpp"
#include "retractor/lib/qTree.hpp"
#include "retractor/lib/RQLParser.hpp"

// ctest -R '^ut-test_presenter' -V

namespace po = boost::program_options;

static po::variables_map makeVM(const std::vector<std::string> &args) {
  po::options_description desc;
  desc.add_options()        //
      ("dot", "")           //
      ("csv", "")           //
      ("fields", "")        //
      ("tags", "")          //
      ("streamprogs", "")   //
      ("rules", "")         //
      ("hideruleprog", "")  //
      ("transparent", "")   //
      ("diagram", po::value<std::string>(), "");
  po::variables_map vm;
  po::store(po::command_line_parser(args).options(desc).run(), vm);
  po::notify(vm);
  return vm;
}

static qTree coreInstance;
static bool initialized = false;

static bool initCore() {
  if (!initialized) {
    coreInstance.clear();
    auto [r1, k1, s1] = parserRQLString(coreInstance, "DECLARE a INTEGER, b BYTE STREAM src1, 1 BINFILE '/dev/urandom'");
    auto [r2, k2, s2] = parserRQLString(coreInstance, "DECLARE c INTEGER STREAM src2, 2 BINFILE '/dev/urandom'");
    initialized       = (r1 == "OK" && r2 == "OK");
  }
  return initialized;
}

TEST(presenter, parse_ok) { EXPECT_TRUE(initCore()); }

TEST(presenter, tags_without_fields_returns_error) {
  // covers: presenter.presenter.2~1
  EXPECT_TRUE(initCore());
  presenter p(coreInstance);
  auto vm = makeVM({"--tags"});

  // Sciezka bledu tlumaczy sie operatorowi na stderr - asercja jest na kodzie powrotu.
  testing::internal::CaptureStderr();
  const int rc = p.run(vm);
  testing::internal::GetCapturedStderr();

  EXPECT_EQ(rc, (int)boost::system::errc::invalid_argument);
}

TEST(presenter, default_mode_returns_success) {
  // covers: presenter.presenter.9~1
  EXPECT_TRUE(initCore());
  presenter p(coreInstance);
  auto vm = makeVM({});
  testing::internal::CaptureStdout();
  int rc          = p.run(vm);
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_EQ(rc, (int)boost::system::errc::success);
  EXPECT_NE(out.find("src1"), std::string::npos);
}

TEST(presenter, dot_mode_outputs_digraph) {
  EXPECT_TRUE(initCore());
  presenter p(coreInstance);
  auto vm = makeVM({"--dot"});
  testing::internal::CaptureStdout();
  int rc          = p.run(vm);
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_EQ(rc, (int)boost::system::errc::success);
  EXPECT_NE(out.find("digraph"), std::string::npos);
  EXPECT_NE(out.find("src1"), std::string::npos);
}

TEST(presenter, dot_mode_with_fields) {
  EXPECT_TRUE(initCore());
  presenter p(coreInstance);
  auto vm = makeVM({"--dot", "--fields"});
  testing::internal::CaptureStdout();
  int rc          = p.run(vm);
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_EQ(rc, (int)boost::system::errc::success);
  EXPECT_NE(out.find("digraph"), std::string::npos);
}

TEST(presenter, dot_mode_tags_requires_fields) {
  EXPECT_TRUE(initCore());
  presenter p(coreInstance);
  auto vm = makeVM({"--tags"});

  testing::internal::CaptureStderr();
  const int rc = p.run(vm);
  testing::internal::GetCapturedStderr();

  EXPECT_NE(rc, (int)boost::system::errc::success);
}

TEST(presenter, csv_mode_returns_success) {
  EXPECT_TRUE(initCore());
  presenter p(coreInstance);
  auto vm = makeVM({"--csv"});
  testing::internal::CaptureStdout();
  int rc          = p.run(vm);
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_EQ(rc, (int)boost::system::errc::success);
  EXPECT_NE(out.find("id_ref"), std::string::npos);
  EXPECT_NE(out.find("src1"), std::string::npos);
}

TEST(presenter, diagram_empty_grid_type_returns_error) {
  // covers: presenter.presenter.5~1
  EXPECT_TRUE(initCore());
  presenter p(coreInstance);
  auto vm = makeVM({"--diagram", ":2"});
  testing::internal::CaptureStderr();
  int rc = p.run(vm);
  testing::internal::GetCapturedStderr();
  EXPECT_EQ(rc, (int)boost::system::errc::invalid_argument);
}

TEST(presenter, diagram_invalid_grid_type_returns_error) {
  // covers: presenter.presenter.5~1
  EXPECT_TRUE(initCore());
  presenter p(coreInstance);
  auto vm = makeVM({"--diagram", "5:1"});
  testing::internal::CaptureStderr();
  int rc = p.run(vm);
  testing::internal::GetCapturedStderr();
  EXPECT_EQ(rc, (int)boost::system::errc::invalid_argument);
}

TEST(presenter, diagram_no_grid_returns_success) {
  EXPECT_TRUE(initCore());
  presenter p(coreInstance);
  auto vm = makeVM({"--diagram", "0:2"});
  testing::internal::CaptureStdout();
  int rc = p.run(vm);
  testing::internal::GetCapturedStdout();
  EXPECT_EQ(rc, (int)boost::system::errc::success);
}

TEST(presenter, diagram_with_grid_returns_success) {
  EXPECT_TRUE(initCore());
  presenter p(coreInstance);
  auto vm = makeVM({"--diagram", "1:3"});
  testing::internal::CaptureStdout();
  int rc = p.run(vm);
  testing::internal::GetCapturedStdout();
  EXPECT_EQ(rc, (int)boost::system::errc::success);
}

namespace {
std::string render(qTree &plan, const std::vector<std::string> &options) {
  presenter display(plan);
  testing::internal::CaptureStdout();
  const int result = display.run(makeVM(options));
  const auto text  = testing::internal::GetCapturedStdout();
  EXPECT_EQ(result, 0);
  return text;
}
}  // namespace

TEST(presenter, mode_precedence_and_read_only_plan_are_observable) {
  // covers: presenter.presenter.1~1, presenter.presenter.2~1
  qTree plan;
  ASSERT_EQ(std::get<0>(parserRQLString(plan, "DECLARE a INTEGER STREAM src, 1 BINFILE 'source.bin'")), "OK");
  const auto revision = plan.planRevision();
  const auto before   = render(plan, {});
  EXPECT_EQ(render(plan, {"--dot", "--csv", "--diagram", "invalid"}), render(plan, {"--dot"}));
  EXPECT_EQ(render(plan, {"--csv", "--diagram", "invalid"}), render(plan, {"--csv"}));
  EXPECT_EQ(plan.planRevision(), revision);
  EXPECT_EQ(plan.size(), 1U);
  EXPECT_EQ(plan[0].filename, "source.bin");
  EXPECT_EQ(render(plan, {}), before);
}

TEST(presenter, dot_colors_dependencies_and_optional_details_are_printed) {
  // covers: presenter.presenter.3~1
  qTree plan;
  ASSERT_EQ(std::get<0>(parserRQLString(plan,
                                        "STORAGE 'output'\nDECLARE a INTEGER STREAM binary, 1 BINFILE 'in.bin'\n"
                                        "DECLARE b BYTE STREAM text, 2 TEXTFILE 'in.txt'\n"
                                        "DECLARE c BYTE STREAM devsrc, 3 DEVICE '/dev/zero'\n"
                                        "SELECT a STREAM result FROM binary")),
            "OK");
  auto generated = plan.getQuery("result");
  generated.id   = "STREAM_generated";
  plan.push_back(generated);
  const auto plain = render(plan, {"--dot"});
  EXPECT_FALSE(plain.contains(":STORAGE"));
  EXPECT_TRUE(plain.contains("fillcolor=Skyblue"));
  EXPECT_TRUE(plain.contains("fillcolor=\"#FFF2CC\""));
  EXPECT_TRUE(plain.contains("fillcolor=PaleGreen"));
  EXPECT_TRUE(plain.contains("fillcolor=Sienna"));
  EXPECT_TRUE(plain.contains("fillcolor=Gray"));
  EXPECT_TRUE(plain.contains("result -> binary"));
  EXPECT_TRUE(plain.contains("interval=1/1"));
  const auto details = render(plan, {"--dot", "--fields", "--tags", "--streamprogs", "--transparent"});
  EXPECT_TRUE(details.contains("a(INTEGER)"));
  EXPECT_TRUE(details.contains("<tag0>"));
  EXPECT_TRUE(details.contains("prg_result"));
  EXPECT_TRUE(details.contains("bgcolor=\"transparent\""));
  EXPECT_FALSE(plain.contains("<tag0>"));
  EXPECT_FALSE(plain.contains("prg_result"));
}

TEST(presenter, csv_tables_have_ordered_headers_and_tab_separated_rows) {
  // covers: presenter.presenter.4~1
  qTree plan;
  ASSERT_EQ(std::get<0>(parserRQLString(plan, "DECLARE a INTEGER STREAM src, 1 BINFILE 'source.bin'")), "OK");
  const auto text      = render(plan, {"--csv"});
  std::size_t previous = 0;
  for (const auto header : {"id\tlen prg\tlen sch\tinterval\tfilename\n", "qcnt\tid_ref\ttoken\tvalue\n",
                            "fcnt\tid_ref\tfName\n", "fcnt_ref\tid_ref\ttoken\tvalue\n", "qcnt\tid\tid_ref\taction\n"}) {
    const auto position = text.find(header, previous);
    ASSERT_NE(position, std::string::npos) << header << text;
    EXPECT_GE(position, previous);
    previous = position + std::string(header).size();
  }
  EXPECT_TRUE(text.contains("src\t0\t1\t1/1\tsource.bin\t\n"));
  EXPECT_TRUE(text.contains("1\tsrc\ta\t\n"));
}

TEST(presenter, listing_contains_boundaries_timeouts_windows_and_both_rule_actions) {
  // covers: presenter.presenter.3~1, presenter.presenter.7~1
  qTree plan;
  query node(boost::rational<int>(1, 2), "result");
  node.filename       = "source.bin";
  node.startupLatency = 3;
  node.logicalOrigin  = 2;
  node.timeoutSeconds = boost::rational<int>(1, 4);
  node.lProgram.emplace_back(PUSH_STREAM, std::string("src"));
  node.windowGroups.push_back(windowGroup{.source = "src", .slot = 1, .width = 4});
  node.lRules.emplace_back("capture", std::list<token>{token(PUSH_VAL, 1)});
  node.lRules.back().action         = rule::DUMP;
  node.lRules.back().dumpRange      = {-1, 2};
  node.lRules.back().dump_retention = 7;
  node.lRules.emplace_back("notify", std::list<token>{token(PUSH_VAL, 1)});
  node.lRules.back().action        = rule::SYSTEM;
  node.lRules.back().systemCommand = "echo marker";
  plan.push_back(node);
  const auto text = render(plan, {});
  EXPECT_TRUE(text.contains("result(1/2)\ttail=3\torigin=2\tsource.bin\ttimeout=1/4\n"));
  EXPECT_TRUE(text.contains("PUSH_STREAM(src)"));
  EXPECT_TRUE(text.contains("WINDOW 0: src[1] rows=4"));
  EXPECT_TRUE(text.contains("RULE capture\n\t\tPUSH_VAL(1)\n\t\tDO DUMP -1 TO 2 RETENTION 7"));
  EXPECT_TRUE(text.contains("RULE notify\n\t\tPUSH_VAL(1)\n\t\tDO SYSTEM 'echo marker'"));
  const auto dot = render(plan, {"--dot", "--rules"});
  EXPECT_TRUE(dot.contains("capture"));
  EXPECT_TRUE(dot.contains("notify"));
}

TEST(presenter, plain_listing_omits_optional_annotations_byte_for_byte) {
  // covers: presenter.presenter.8~1
  qTree plan;
  plan.push_back(query(0, ":STORAGE"));
  plan.push_back(query(1, "src"));
  plan[1].filename = "source.bin";
  EXPECT_EQ(render(plan, {}), ":STORAGE\nsrc(1/1)\tsource.bin\n");
}

TEST(presenter, diagram_rejects_non_numeric_grid_type_and_cycle_count) {
  // covers: presenter.presenter.5~1
  qTree plan;
  ASSERT_EQ(std::get<0>(parserRQLString(plan, "DECLARE a INTEGER STREAM src, 1 BINFILE 'source.bin'")), "OK");
  for (const auto *argument : {"nonnumeric:1", "0x:1", "1 :1", "0:", "0:abc", "0:2x", "0:0", "0:-1", "0:99999999999"}) {
    SCOPED_TRACE(argument);
    presenter display(plan);
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    const int result  = display.run(makeVM({"--diagram", argument}));
    const auto error  = testing::internal::GetCapturedStderr();
    const auto output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(result, static_cast<int>(boost::system::errc::invalid_argument));
    EXPECT_TRUE(error.contains("is invalid")) << error;
    EXPECT_TRUE(output.empty()) << output;
  }
}

TEST(presenter, empty_diagram_is_fatal_eperm) {
  // covers: presenter.presenter.6~1
  qTree empty;
  presenter display(empty);
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  const int result  = display.run(makeVM({"--diagram", "0"}));
  const auto error  = testing::internal::GetCapturedStderr();
  const auto output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(result, static_cast<int>(boost::system::errc::operation_not_permitted));
  EXPECT_TRUE(error.contains("FATAL: presenter: plan holds no streams")) << error;
  EXPECT_FALSE(output.contains("Grid time")) << output;
}

TEST(presenter, external_option_type_error_returns_interrupted_and_diagnostic) {
  // covers: presenter.presenter.9~1
  qTree plan;
  presenter display(plan);
  po::variables_map options;
  options.insert({"diagram", po::variable_value(123, false)});
  testing::internal::CaptureStderr();
  const int result = display.run(options);
  const auto error = testing::internal::GetCapturedStderr();
  EXPECT_EQ(result, static_cast<int>(boost::system::errc::interrupted));
  EXPECT_FALSE(error.empty());
}
