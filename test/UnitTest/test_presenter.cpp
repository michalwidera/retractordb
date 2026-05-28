#include <gtest/gtest.h>

#include <boost/program_options.hpp>
#include <boost/system/error_code.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "retractor/lib/presenter.hpp"
#include "retractor/lib/qTree.hpp"

// ctest -R '^ut-test_presenter' -V

extern std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &sInputFile);

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
    auto [r1, k1, s1] = parserRQLString(coreInstance, "DECLARE a INTEGER, b BYTE STREAM src1, 1 FILE '/dev/urandom'");
    auto [r2, k2, s2] = parserRQLString(coreInstance, "DECLARE c INTEGER STREAM src2, 2 FILE '/dev/urandom'");
    initialized       = (r1 == "OK" && r2 == "OK");
  }
  return initialized;
}

TEST(presenter, parse_ok) { EXPECT_TRUE(initCore()); }

TEST(presenter, tags_without_fields_returns_error) {
  EXPECT_TRUE(initCore());
  presenter p(coreInstance);
  auto vm = makeVM({"--tags"});
  EXPECT_EQ(p.run(vm), (int)boost::system::errc::invalid_argument);
}

TEST(presenter, default_mode_returns_success) {
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
  EXPECT_NE(p.run(vm), (int)boost::system::errc::success);
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
  EXPECT_TRUE(initCore());
  presenter p(coreInstance);
  auto vm = makeVM({"--diagram", ":2"});
  testing::internal::CaptureStderr();
  int rc = p.run(vm);
  testing::internal::GetCapturedStderr();
  EXPECT_EQ(rc, (int)boost::system::errc::invalid_argument);
}

TEST(presenter, diagram_invalid_grid_type_returns_error) {
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
