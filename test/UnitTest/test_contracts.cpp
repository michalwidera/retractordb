#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

#include "cmdID.hpp"
#include "constants.hpp"
#include "fldType.hpp"
#include "logCapture.hpp"
#include "osPlatform.hpp"
#include "rdb/accessorFactory.hpp"
#include "rdb/convertTypes.hpp"
#include "rdb/ICommand.hpp"
#include "rdb/indexRecord.hpp"
#include "rdb/memoryStore.hpp"
#include "rdb/metaIndexStore.hpp"
#include "rdb/metaShadow.hpp"
#include "rdb/payload.hpp"
#include "rdb/rleSegment.hpp"
#include "rdb/storageFile.hpp"
#include "rdbResult.hpp"
#include "retractor/lib/checkedArith.hpp"
#include "retractor/lib/CRSMath.hpp"
#include "retractor/lib/qTree.hpp"
#include "retractor/lib/query.hpp"
#include "retractor/lib/token.hpp"
#include "rqlFunctions.hpp"
#include "serviceLockDir.hpp"

// covers: token.token.2~1, query.query.1~1
static_assert(std::is_same_v<decltype(std::declval<const token &>().getVT()), const rdb::descFldVT &>);
// covers: crsmath.time-line.3~1
static_assert(!std::is_copy_constructible_v<CRationalStreamMath::TimeLine>);
static_assert(!std::is_copy_assignable_v<CRationalStreamMath::TimeLine>);
// covers: xtrdb.command.virtual-dtor~1
static_assert(std::has_virtual_destructor_v<ICommand>);
// covers: storagefile.storage-fd.7~1
static_assert(!std::is_copy_constructible_v<rdb::StorageFd>);
static_assert(!std::is_copy_assignable_v<rdb::StorageFd>);
// covers: fainterface.file-interface.1~1
static_assert(std::is_abstract_v<rdb::FileInterface>);
static_assert(std::has_virtual_destructor_v<rdb::FileInterface>);

TEST(RequirementContracts, memory_bucket_identity_survives_other_insertions_and_erasures) {
  // covers: memorystore.memory-store.1~1, memorystore.memory-store.3~1, memorystore.memory-store.4~1
  rdb::MemoryStore store;
  EXPECT_TRUE(store.empty());
  auto &original = store.bucket("kept");
  EXPECT_TRUE(original.data.empty());
  EXPECT_TRUE(original.nulls.empty());
  EXPECT_EQ(original.writeCount, 0U);
  original.writeCount = 17;
  for (int i = 0; i < 100; ++i)
    store.bucket(std::to_string(i));
  for (int i = 0; i < 100; ++i)
    store.erase(std::to_string(i));
  EXPECT_EQ(store.size(), 1U);
  EXPECT_EQ(&store.bucket("kept"), &original);
  EXPECT_EQ(store.bucket("kept").writeCount, 17U);
  store.erase("kept");
  EXPECT_TRUE(store.empty());
  EXPECT_EQ(store.bucket("kept").writeCount, 0U);
}

TEST(RequirementContracts, rle_split_preserves_prefix_suffix_and_clears_gap_flags) {
  // covers: rlesegment.split-segment.1~1, rlesegment.split-segment.2~1, rlesegment.sum-non-gap-records.1~1
  const rdb::IndexRecord segment{.nullBitset = {true, false}, .recordCount = 3, .isGap = true};
  const std::vector<bool> changed{false, true};
  for (size_t offset = 0; offset < 3; ++offset) {
    const auto parts = rdb::splitSegment(segment, offset, changed);
    ASSERT_EQ(parts.size(), offset == 1 ? 3U : 2U);
    size_t count = 0;
    for (const auto &part : parts) {
      EXPECT_FALSE(part.isGap);
      count += part.recordCount;
    }
    EXPECT_EQ(count, 3U);
    const auto changedPosition = offset == 0 ? 0 : 1;
    EXPECT_EQ(parts[changedPosition].recordCount, 1U);
    EXPECT_EQ(parts[changedPosition].nullBitset, changed);
    if (offset > 0) {
      EXPECT_EQ(parts.front().recordCount, offset);
      EXPECT_EQ(parts.front().nullBitset, segment.nullBitset);
    }
    if (offset < 2) {
      EXPECT_EQ(parts.back().recordCount, 2 - offset);
      EXPECT_EQ(parts.back().nullBitset, segment.nullBitset);
    }
  }
  const auto singleton = rdb::splitSegment({.nullBitset = {true}, .recordCount = 1}, 0, changed);
  ASSERT_EQ(singleton.size(), 1U);
  EXPECT_EQ(singleton[0].nullBitset, changed);
  EXPECT_FALSE(singleton[0].isGap);
  EXPECT_EQ(rdb::sumNonGapRecords({segment, {.recordCount = 4}, {.recordCount = 8, .isGap = true}, {.recordCount = 2}}), 6U);
  EXPECT_EQ(rdb::sumNonGapRecords({}), 0U);
}

TEST(RequirementContracts, index_and_shadow_frames_match_the_legacy_binary_layout) {
  // covers: indexrecord.index-record.1~1, indexrecord.index-record.2~1, indexrecord.index-record.3~1
  // covers: indexrecord.index-record.4~1, indexrecord.index-record.5~1, metashadow.shadow-override.2~1
  // covers: metashadow.shadow-override.3~1, metashadow.shadow-override.4~1
  const std::vector<bool> bits{true, false, true, false, false, false, false, true, true};
  const size_t count = 42, bitCount = bits.size();
  std::vector<std::byte> expected(1 + 2 * sizeof(size_t) + 2, std::byte{0});
  std::memcpy(expected.data() + 1, &count, sizeof(count));
  std::memcpy(expected.data() + 1 + sizeof(count), &bitCount, sizeof(bitCount));
  expected[expected.size() - 2] = std::byte{0x85};
  expected.back()               = std::byte{0x01};
  EXPECT_EQ((rdb::IndexRecord{.nullBitset = bits, .recordCount = count}.serialize()), expected);
  EXPECT_EQ((rdb::metaShadow::ShadowOverride{.recordIndex = count, .nullBitset = bits}.serialize()), expected);
  for (const auto flag : {std::byte{0}, std::byte{1}, std::byte{0xFE}}) {
    expected[0]       = flag;
    const auto record = rdbtest::ok(rdb::IndexRecord::deserialize(expected));
    EXPECT_EQ(record.isGap, flag != std::byte{0});
    EXPECT_EQ(record.recordCount, count);
    EXPECT_EQ(record.nullBitset, bits);
    const auto shadow = rdbtest::ok(rdb::metaShadow::ShadowOverride::deserialize(expected));
    EXPECT_EQ(shadow.recordIndex, count);
    EXPECT_EQ(shadow.nullBitset, bits);
  }
  EXPECT_RDB_ERROR(rdb::IndexRecord::deserialize({}), rdb::Errc::IO);
  EXPECT_RDB_ERROR(rdb::metaShadow::ShadowOverride::deserialize({}), rdb::Errc::IO);
}

TEST(RequirementContracts, field_helpers_preserve_flat_slot_layout) {
  // covers: fldtype.get-stringdesc-fld.1~1, fldtype.flat-element-count.1~1, fldtype.flat-slot-field.1~1
  EXPECT_EQ(rdb::GetStringdescFld(rdb::NULLTYPE), "NULL");
  EXPECT_EQ(rdb::GetStringdescFld(rdb::RATIONAL), "RATIONAL");
  EXPECT_EQ(rdb::flatElementCount({"text", 1, 10, rdb::STRING}), 1);
  EXPECT_EQ(rdb::flatElementCount({"array", 4, 10, rdb::INTEGER}), 10);
  EXPECT_EQ(rdb::flatElementCount({"null", 0, 3, rdb::NULLTYPE}), 3);
  const auto text = rdb::flatSlotField("s", rdb::STRING, 12);
  EXPECT_EQ(text.rname, "s");
  EXPECT_EQ(text.rtype, rdb::STRING);
  EXPECT_EQ(text.rlen, 1);
  EXPECT_EQ(text.rarray, 12);
  const auto number = rdb::flatSlotField("n", rdb::DOUBLE, 8);
  EXPECT_EQ(number.rname, "n");
  EXPECT_EQ(number.rtype, rdb::DOUBLE);
  EXPECT_EQ(number.rlen, 8);
  EXPECT_EQ(number.rarray, 1);
}

TEST(RequirementContracts, token_default_reference_conversions_and_diagnostics) {
  // covers: cmdid.get-stringcommand-id.1~1, token.token.1~1, token.token.3~1, token.token.4~1, token.token.5~1, token.token.6~1
  const token empty;
  EXPECT_EQ(empty.getCommandID(), VOID_COMMAND);
  EXPECT_EQ(std::get<int>(empty.getVT()), 0);
  const token number(PUSH_VAL, 7);
  EXPECT_EQ(number.getRI(), boost::rational<int>(7));
  EXPECT_EQ(&number.getVT(), &number.getVT());
  const std::vector<std::pair<rdb::descFldVT, std::string>> values{{std::monostate{}, "null"},
                                                                   {std::string("abc"), "abc"},
                                                                   {uint8_t{8}, "8"},
                                                                   {7, "7"},
                                                                   {7U, "7"},
                                                                   {boost::rational<int>(3, 2), "3/2"},
                                                                   {std::make_pair(2, 3), "2,3"},
                                                                   {std::make_pair(std::string("source"), 4), "source"}};
  for (const auto &[value, expected] : values)
    EXPECT_EQ(token(PUSH_VAL, value).getStr_(), expected);
  token reference(PUSH_ID, std::make_pair(std::string("source"), 4));
  EXPECT_EQ(reference.getStrCommandID(), "PUSH_ID");
  EXPECT_EQ(GetStringcommand_id(PUSH_ID), "PUSH_ID");
  std::ostringstream diagnostic;
  diagnostic << reference;
  EXPECT_EQ(diagnostic.str(), "PUSH_ID(source[4])");
}

TEST(RequirementContracts, ipc_names_preserve_legacy_names_and_bound_the_instance_token) {
  // covers: constants.short-server-tag.1~1, constants.short-server-tag.2~1, constants.server-name-token.1~1
  // covers: constants.server-name-token.2~1, constants.with-server-suffix.1~1, constants.names-for-token.1~1
  // covers: constants.names-for-token.2~1, constants.identity-lock-path.1~1, constants.names.1~1, constants.service-name.1~1
  // covers: constants.service-lock-file.1~1, constants.service-lock-dir.1~1, constants.service-lock-instance.1~1
  // covers: constants.service-lock-instance.2~1, constants.service-lock-instance.3~1, constants.service-lock-instance.4~1
  using namespace ipc;
  EXPECT_EQ(shortServerTag(""), "04fd0bfc1");
  EXPECT_EQ(shortServerTag("server"), "0c31509af");
  EXPECT_EQ(serverNameToken(""), "");
  const auto limit = kMaxObjectNameLength - kResponseQueuePrefix.size() - 2 - kObjectNameTailBudget;
  const std::string boundary(limit, 'a');
  EXPECT_EQ(serverNameToken(boundary), boundary);
  EXPECT_EQ(serverNameToken(boundary + 'b'), shortServerTag(boundary + 'b'));
  for (const std::string name : {std::string(), std::string("server"), boundary + 'b'}) {
    const auto token  = serverNameToken(name);
    const auto result = names(name);
    const auto direct = namesForToken(token);
    EXPECT_EQ(result.shmemSegment, direct.shmemSegment);
    EXPECT_EQ(result.queryQueue, direct.queryQueue);
    EXPECT_EQ(result.responseQueuePrefix, direct.responseQueuePrefix);
    const auto suffix = token.empty() ? "" : "." + token;
    EXPECT_EQ(result.shmemSegment, std::string(kShmemSegment) + suffix);
    EXPECT_EQ(result.queryQueue, std::string(kQueryQueue) + suffix);
    EXPECT_EQ(result.responseQueuePrefix, std::string(kResponseQueuePrefix) + suffix + (token.empty() ? "" : "."));
    EXPECT_EQ(serviceName(name), std::string(kServiceLockFamily) + (name.empty() ? "" : "." + name));
    EXPECT_EQ(serviceLockFile(serviceName(name)), serviceName(name) + ".lock");
    EXPECT_EQ(serviceLockInstance(serviceLockFile(serviceName(name))), name);
  }
  EXPECT_EQ(withServerSuffix("base", ""), "base");
  EXPECT_EQ(withServerSuffix("base", "srv"), "base.srv");
  EXPECT_EQ(identityLockPath("queue"), std::string(kMachineLockDir) + "/xretractor_ipc.queue.lock");
  EXPECT_EQ(identityLockPath("queue", "/locks"), "/locks/xretractor_ipc.queue.lock");
  EXPECT_EQ(serviceLockDir(""), std::filesystem::temp_directory_path());
  EXPECT_EQ(serviceLockDir("/custom"), "/custom");
  for (const char *file : {"other.lock", "xretractor_service", "xretractor_service..lock", "xretractor_servicex.lock"})
    EXPECT_EQ(serviceLockInstance(file), std::nullopt);
  EXPECT_EQ(serviceLockInstance("xretractor_service.unsafe/name.lock"), "unsafe/name");
}

TEST(RequirementContracts, meta_store_cache_mutations_refusal_and_abandon) {
  // covers: metaindexstore.meta-index-store.1~1, metaindexstore.meta-index-store.2~1, metaindexstore.meta-index-store.3~1
  // covers: metaindexstore.meta-index-store.4~1, metaindexstore.meta-index-store.5~1
  namespace fs           = std::filesystem;
  const std::string file = "contracts_index.meta";
  fs::remove_all(file);
  fs::remove_all(file + ".tmp");
  const rdb::IndexRecord first{.nullBitset = {true, false}, .recordCount = 2};
  const rdb::IndexRecord next{.nullBitset = {false, true}, .recordCount = 3};
  rdb::MetaIndexStore store(file, first.serialize().size());
  EXPECT_EQ(store.path(), file);
  ASSERT_RDB_OK(store.rewrite({first}));
  EXPECT_EQ(fs::file_size(file), 8 + first.serialize().size());
  std::ifstream input(file, std::ios::binary);
  char header[8];
  input.read(header, sizeof(header));
  EXPECT_EQ(std::string(header, sizeof(header)), std::string(8, '\0'));
  ASSERT_EQ(store.readAll().size(), 1U);
  // Wazny cache nie powinien ponownie czytac pliku; przyrostowa mutacja go uzupelnia.
  fs::remove(file);
  EXPECT_EQ(store.readAll().front().recordCount, 2U);
  ASSERT_RDB_OK(store.rewrite({first}));
  store.appendEntry(next);
  ASSERT_EQ(store.readAll().size(), 2U);
  store.overwriteLast(first);
  EXPECT_EQ(store.readAll().back().recordCount, 2U);
  fs::create_directory(file + ".tmp");
  EXPECT_RDB_ERROR(store.rewrite({next}), rdb::Errc::IO);
  ASSERT_EQ(store.readAll().size(), 2U);
  EXPECT_EQ(store.readAll().back().recordCount, 2U);
  fs::remove_all(file + ".tmp");
  store.abandon();
  fs::remove(file);
  EXPECT_TRUE(store.empty());
  store.saveHeader();
  store.appendEntry(first);
  store.overwriteLast(next);
  ASSERT_RDB_OK(store.rewrite({first}));
  EXPECT_FALSE(fs::exists(file));
  rdb::MetaIndexStore inert("", first.serialize().size());
  inert.saveHeader();
  inert.appendEntry(first);
  EXPECT_TRUE(inert.readAll().empty());
}

TEST(RequirementContracts, writable_backends_clear_null_metadata_except_memory) {
  // covers: fainterface.file-interface.1~1, fainterface.file-interface.3~1, fainterface.file-interface.4~1
  rdb::Descriptor descriptor("a", 1, 1, rdb::BYTE);
  for (const char *type : {"GENERIC", "POSIX", "POSIXSHD", "DIRECT", "DEFAULT", "MEMORY"}) {
    SCOPED_TRACE(type);
    const std::string file = std::string("contracts_backend_") + type;
    auto backend           = rdbtest::ok(rdb::makeAccessor(type, file, descriptor, false, -1));
    const uint8_t value    = 42;
    ASSERT_EQ(backend->write(&value, {true}, SIZE_MAX), EXIT_SUCCESS);
    uint8_t loaded = 0;
    std::vector<bool> nulls{true};
    ASSERT_EQ(backend->read(&loaded, nulls, 0), EXIT_SUCCESS);
    EXPECT_EQ(loaded, value);
    EXPECT_EQ(nulls, std::string(type) == "MEMORY" ? std::vector<bool>{true} : std::vector<bool>{});
    backend.reset();
    std::filesystem::remove(file);
    std::filesystem::remove(file + ".shadow");
  }
}

TEST(RequirementContracts, storage_fd_flags_offsets_eof_and_lifetime) {
  // covers: storagefile.open-storage-file.1~1, storagefile.storage-fd.1~1, storagefile.storage-fd.3~1
  // covers: storagefile.storage-fd.4~1, storagefile.storage-fd.5~1, storagefile.storage-fd.6~1
  const std::string file = "contracts_fd";
  const int raw          = rdb::openStorageFile(file, O_RDWR | O_CREAT | O_TRUNC, 0600);
  ASSERT_GE(raw, 0);
  EXPECT_NE(fcntl(raw, F_GETFD) & FD_CLOEXEC, 0);
  close(raw);
  const std::vector<std::byte> bytes{std::byte{1}, std::byte{2}, std::byte{3}};
  {
    const rdb::StorageFd fd(file, O_RDWR);
    ASSERT_TRUE(fd.isOpen());
    EXPECT_EQ(fd.size(), 0);
    ASSERT_TRUE(fd.write(bytes));
    ASSERT_TRUE(fd.writeAt(std::span(bytes).first(1), 1));
    std::vector<std::byte> read(8, std::byte{0});
    EXPECT_EQ(fd.readAt(read, 0), 3);
    EXPECT_EQ(read[0], std::byte{1});
    EXPECT_EQ(read[1], std::byte{1});
    EXPECT_EQ(read[2], std::byte{3});
    EXPECT_EQ(fd.readAt(read, 3), 0);
    EXPECT_EQ(fd.size(), 3);
  }
  std::filesystem::remove(file);
}

TEST(RequirementContracts, storage_file_symlink_refusal_preserves_errno_bytes_and_descriptor_lifetime) {
  // covers: storagefile.open-storage-file.2~1, storagefile.storage-fd.1~1, storagefile.storage-fd.2~1
  // covers: storagefile.storage-fd.7~1
  const std::string target = "contracts_link_target";
  const std::string link   = "contracts_link";
  std::ofstream(target, std::ios::binary) << "original";
  std::filesystem::create_symlink(target, link);
  LogCapture log;
  errno = 0;
  EXPECT_EQ(rdb::openStorageFile(link, O_WRONLY | O_TRUNC), -1);
  EXPECT_EQ(errno, ELOOP);
  {
    rdb::StorageFd refused(link, O_WRONLY | O_TRUNC);
    EXPECT_EQ(errno, ELOOP);
    EXPECT_FALSE(refused.isOpen());
  }
  EXPECT_NE(log.text().find("symbolic link"), std::string::npos);
  std::ifstream input(target, std::ios::binary);
  EXPECT_EQ(std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()), "original");
  {
    const rdb::StorageFd allowed(link, O_RDONLY, 0600, true);
    EXPECT_TRUE(allowed.isOpen());
    EXPECT_EQ(allowed.size(), 8);
  }
  int owned = -1;
  {
    const int before = ::open(target.c_str(), O_RDONLY);
    ASSERT_GE(before, 0);
    ::close(before);
    rdb::StorageFd file(target, O_RDONLY);
    // W jednowatkowym tescie open przydziela najnizszy wolny numer.
    owned = before;
    EXPECT_NE(::fcntl(owned, F_GETFD) & FD_CLOEXEC, 0);
  }
  errno = 0;
  EXPECT_EQ(::fcntl(owned, F_GETFD), -1);
  EXPECT_EQ(errno, EBADF);
  std::filesystem::remove(link);
  std::filesystem::remove(target);
}

TEST(RequirementContracts, payload_decimal_hex_and_string_padding) {
  // covers: payload.payload.6~1, payload.payload.9~1
  rdb::payload value(rdb::Descriptor("n", 4, 1, rdb::INTEGER));
  value.setItem(0, 42);
  std::ostringstream decimal, hex;
  decimal << rdb::singleLineFormat << value;
  value.setHex(true);
  hex << rdb::singleLineFormat << value;
  EXPECT_EQ(decimal.str(), "{ n:42 }");
  EXPECT_EQ(hex.str(), "{ n:0000002a }");
  rdb::payload text(rdb::Descriptor("s", 1, 4, rdb::STRING));
  text.setItem(0, std::string("abcdef"));
  EXPECT_EQ(std::any_cast<std::string>(*text.getItem(0)), "abcd");
  text.setItem(0, std::string("x"));
  EXPECT_EQ(std::any_cast<std::string>(*text.getItem(0)), "x");
  EXPECT_EQ(text.span()[1], 0);
  EXPECT_EQ(text.span()[3], 0);
}

TEST(RequirementContracts, checked_arithmetic_bounds_and_reduced_rationals) {
  // covers: checkedarith.add.1~1, checkedarith.add.2~1, checkedarith.add.3~1, checkedarith.sub.1~1, checkedarith.sub.2~1
  // covers: checkedarith.sub.3~1, checkedarith.mul.1~1, checkedarith.mul.2~1, checkedarith.mul.3~1, checkedarith.div.1~1
  // covers: checkedarith.div.2~1, checkedarith.neg.1~1, checkedarith.neg.2~1, checkedarith.narrowed.1~1
  // covers: checkedarith.narrowed.2~1, checkedarith.narrowed.3~1
  using namespace checkedArith;
  using ratio      = boost::rational<int>;
  constexpr int hi = std::numeric_limits<int>::max(), lo = std::numeric_limits<int>::min();
  constexpr unsigned uhi = std::numeric_limits<unsigned>::max();
  EXPECT_EQ(add(hi - 1, 1), hi);
  EXPECT_EQ(add(hi, 1), std::nullopt);
  EXPECT_EQ(sub(lo + 1, 1), lo);
  EXPECT_EQ(sub(lo, 1), std::nullopt);
  EXPECT_EQ(mul(hi, 1), hi);
  EXPECT_EQ(mul(hi, 2), std::nullopt);
  EXPECT_EQ(add(uhi - 1, 1U), uhi);
  EXPECT_EQ(add(uhi, 1U), std::nullopt);
  EXPECT_EQ(sub(1U, 1U), 0U);
  EXPECT_EQ(sub(0U, 1U), std::nullopt);
  EXPECT_EQ(mul(uhi, 1U), uhi);
  EXPECT_EQ(mul(uhi, 2U), std::nullopt);
  EXPECT_EQ(checkedArith::div(-7, 2), -3);
  EXPECT_EQ(checkedArith::div(lo, -1), std::nullopt);
  EXPECT_EQ(neg(-1), 1);
  EXPECT_EQ(neg(lo), std::nullopt);
  EXPECT_EQ(add(ratio(1, 65536), ratio(1, 65536)), ratio(1, 32768));
  EXPECT_EQ(sub(ratio(3, 65536), ratio(1, 65536)), ratio(1, 32768));
  EXPECT_EQ(mul(ratio(2, 3), ratio(3, 2)), ratio(1));
  EXPECT_EQ(checkedArith::div(ratio(2, 3), ratio(4, 3)), ratio(1, 2));
  EXPECT_EQ(checkedArith::div(ratio(1), ratio(0)), std::nullopt);
  EXPECT_EQ(add(ratio(hi), ratio(1)), std::nullopt);
  EXPECT_EQ(sub(ratio(lo), ratio(1)), std::nullopt);
  EXPECT_EQ(mul(ratio(hi), ratio(2)), std::nullopt);
  EXPECT_EQ(neg(ratio(lo)), std::nullopt);
  EXPECT_EQ(neg(ratio(-2, 3)), ratio(2, 3));
  EXPECT_EQ(detail::narrowed(0, 0), std::nullopt);
  EXPECT_EQ(detail::narrowed(2, -4), ratio(-1, 2));
  EXPECT_EQ(detail::narrowed(static_cast<int64_t>(hi) * 2, 2), ratio(hi));
  EXPECT_EQ(detail::narrowed(static_cast<int64_t>(hi) + 1, 1), std::nullopt);
  EXPECT_EQ(detail::narrowed(1, static_cast<int64_t>(hi) + 1), std::nullopt);
}

TEST(RequirementContracts, query_helpers_identify_nodes_dependencies_and_reset_state) {
  // covers: query.reduction-result-field.1~1, query.reduction-result-field.2~1, query.source-kind-keyword.1~1, query.query.2~1
  // covers: query.query.3~1, query.query.4~1, query.query.6~1, query.query.8~1, query.query.10~1, query.operator-less.1~1
  EXPECT_EQ(sourceKindKeyword(sourceKind::none), "");
  EXPECT_EQ(sourceKindKeyword(sourceKind::binFile), "BINFILE");
  EXPECT_EQ(sourceKindKeyword(sourceKind::textFile), "TEXTFILE");
  EXPECT_EQ(sourceKindKeyword(sourceKind::device), "DEVICE");
  for (const auto type : {rdb::BYTE, rdb::INTEGER, rdb::UINT, rdb::RATIONAL})
    EXPECT_EQ(reductionResultField(type, 4), (std::pair{rdb::RATIONAL, static_cast<int>(sizeof(boost::rational<int>))}));
  for (const auto type : {rdb::FLOAT, rdb::DOUBLE, rdb::STRING})
    EXPECT_EQ(reductionResultField(type, 13), (std::pair{type, 13}));
  query node(2, "STREAM_example");
  EXPECT_TRUE(node.isGenerated());
  EXPECT_TRUE(node.isDeclaration());
  EXPECT_FALSE(node.isCompilerDirective());
  node.lProgram = {token(PUSH_STREAM, std::string("a")), token(PUSH_STREAM, std::string("b")), token(STREAM_ADD)};
  EXPECT_EQ(node.getDepStream(), (std::vector<std::string>{"a", "b"}));
  EXPECT_FALSE(node.isReductionRequired());
  node.lProgram.push_back(token(STREAM_SUM));
  EXPECT_TRUE(node.isReductionRequired());
  node.filename     = "source";
  node.kind         = sourceKind::device;
  node.isDisposable = node.isOneShot = node.isHold = node.isSubstrat = node.isDeprecatedFile = true;
  node.declarationLine                                                                       = 7;
  node.deprecatedNotations                                                                   = {"old"};
  node.generatorSize                                                                         = 8;
  node.timeoutSeconds                                                                        = 3;
  node.policy                                                                                = {"MEMORY", 10};
  node.retention                                                                             = rdb::retention_t{2, 3};
  EXPECT_EQ(node.storageType(), "MEMORY");
  node.reset();
  EXPECT_TRUE(node.id.empty());
  EXPECT_TRUE(node.filename.empty());
  EXPECT_TRUE(node.lProgram.empty());
  EXPECT_TRUE(node.lSchema.empty());
  EXPECT_TRUE(node.deprecatedNotations.empty());
  EXPECT_EQ(node.rInterval, 0);
  EXPECT_FALSE(node.isDisposable || node.isOneShot || node.isHold || node.isSubstrat || node.isDeprecatedFile);
  EXPECT_EQ(node.kind, sourceKind::none);
  EXPECT_EQ(node.declarationLine, 0U);
  EXPECT_EQ(node.generatorSize, query::notAGenerator);
  EXPECT_EQ(node.policy, (std::pair<std::string, size_t>{"DEFAULT", 0}));
  EXPECT_EQ(node.retention.segments, 0);
  EXPECT_EQ(node.retention.capacity, 0);
  EXPECT_FALSE(node.timeoutSeconds.has_value());
  node.id = ":STORAGE";
  EXPECT_TRUE(node.isCompilerDirective());
  EXPECT_TRUE(query(1, "a") < query(2, "b"));
  EXPECT_FALSE(query(2, "a") < query(2, "b"));
}

TEST(RequirementContracts, window_identity_compares_programs_or_slots) {
  // covers: query.window-group.1~1, query.window-group.2~1, query.window-group.3~1
  const windowGroup base{.source = "src", .slot = 1, .width = 3};
  auto changed = base;
  EXPECT_TRUE(base.sameAs(changed));
  changed.slot = 2;
  EXPECT_FALSE(base.sameAs(changed));
  changed        = base;
  changed.source = "other";
  EXPECT_FALSE(base.sameAs(changed));
  changed       = base;
  changed.width = 4;
  EXPECT_FALSE(base.sameAs(changed));
  changed           = base;
  changed.valueType = rdb::DOUBLE;
  EXPECT_FALSE(base.sameAs(changed));
  changed         = base;
  changed.program = {token(PUSH_VAL, 1)};
  EXPECT_FALSE(base.sameAs(changed));
  auto equivalent = changed;
  equivalent.slot = 99;
  EXPECT_TRUE(changed.sameAs(equivalent));
  equivalent.program = {token(PUSH_VAL, 2)};
  EXPECT_FALSE(changed.sameAs(equivalent));
  equivalent.program = {token(PUSH_ID, 1)};
  EXPECT_FALSE(changed.sameAs(equivalent));
}

TEST(RequirementContracts, from_arguments_follow_the_compiled_program_shape) {
  // covers: query.get-args.1~1, query.get-args.2~1, query.get-args.3~1
  for (int size = 1; size <= 3; ++size) {
    std::list<token> program{token(PUSH_STREAM, std::string("left"))};
    if (size == 2) program.emplace_back(STREAM_SUM);
    if (size == 3) {
      program.emplace_back(PUSH_STREAM, std::string("right"));
      program.emplace_back(STREAM_ADD);
    }
    const auto [left, right, command] = GetArgs(program);
    EXPECT_EQ(left, "left");
    EXPECT_EQ(right, size == 3 ? "right" : "");
    EXPECT_EQ(command.getCommandID(), size == 1 ? PUSH_STREAM : (size == 2 ? STREAM_SUM : STREAM_ADD));
  }
}

TEST(RequirementContractsDeathTest, oversized_from_and_unknown_fields_are_invariant_failures) {
  // covers: query.query.9~1, query.get-args.4~1
  std::list<token> oversized(4, token(PUSH_STREAM, std::string("src")));
  ASSERT_DEATH(GetArgs(oversized), "program too large");
  query node(1, "node");
  field absent({"missing", 4, 1, rdb::INTEGER}, token(PUSH_VAL, 1));
  ASSERT_DEATH(node.getFieldIndex(absent), "missing");
}

TEST(RequirementContracts, catalogue_fallback_values_and_process_snapshot) {
  // covers: osplatform.inspect-process.1~1, osplatform.inspect-process.2~1, osplatform.shared-memory-backing-path.1~2
  // covers: rqlfunctions.find-rql-function.1~1, rqlfunctions.find-rql-function.2~1, converttypes.null-fallback-value.2~1
  const auto canonical = rdb::findRqlFunction("To_InTeGeR");
  ASSERT_TRUE(canonical.has_value());
  EXPECT_EQ(canonical->canonical, "to_integer");
  EXPECT_EQ(rdb::findRqlFunction("does_not_exist"), std::nullopt);
  for (const auto type : {rdb::TYPE, rdb::REF, rdb::RETENTION, rdb::RETMEMORY})
    EXPECT_EQ(std::get<uint8_t>(nullFallbackValue(type)), 0);
  for (const int pid : {0, -1}) {
    const auto snapshot = osplat::inspectProcess(pid);
    EXPECT_FALSE(snapshot.found);
    EXPECT_FALSE(snapshot.unreadable);
  }
  const auto self = osplat::inspectProcess(getpid());
  EXPECT_TRUE(self.found);
  EXPECT_FALSE(self.unreadable);
  EXPECT_FALSE(self.zombie);
  EXPECT_NE(self.startTime, 0U);
  EXPECT_EQ(osplat::sharedMemoryBackingPath(), RDB_HAS_BOOST_POSIX_SHM ? "/dev/shm" : "/tmp");
}

TEST(RequirementContracts, storage_descriptor_keeps_schema_and_source_or_output_configuration) {
  // covers: query.query.5~1
  using item         = std::tuple<std::string, int, int, rdb::descFld>;
  const auto entries = [](const rdb::Descriptor &descriptor) {
    std::vector<item> result;
    for (const auto &f : descriptor)
      result.emplace_back(f.rname, f.rlen, f.rarray, f.rtype);
    return result;
  };
  query node(1, "src");
  node.filename = "input.txt";
  node.lSchema.emplace_back(rdb::rField("a", 4, 2, rdb::INTEGER), std::list<token>{});
  node.lSchema.emplace_back(rdb::rField("s", 1, 7, rdb::STRING), std::list<token>{});
  const std::vector<item> schema{{"a", 4, 2, rdb::INTEGER}, {"s", 1, 7, rdb::STRING}};
  for (const auto &[kind, type] : std::vector<std::pair<sourceKind, std::string>>{
           {sourceKind::binFile, "BINFILE"}, {sourceKind::textFile, "TEXTSOURCE"}, {sourceKind::device, "DEVICE"}}) {
    node.kind     = kind;
    auto expected = schema;
    expected.emplace_back("input.txt", 0, 0, rdb::REF);
    expected.emplace_back(type, 0, 0, rdb::TYPE);
    EXPECT_EQ(entries(node.descriptorStorage()), expected);
  }
  node.lProgram = {token(PUSH_STREAM, std::string("parent"))};
  EXPECT_EQ(entries(node.descriptorStorage()), schema);
  node.retention = {.segments = 3, .capacity = 11};
  for (const auto *profile : {"MEMORY", "POSIX"}) {
    node.policy   = {profile, 13};
    auto expected = schema;
    expected.emplace_back("", 3, 11, rdb::RETENTION);
    expected.emplace_back("", 13, 0, rdb::RETMEMORY);
    expected.emplace_back(profile, 0, 0, rdb::TYPE);
    EXPECT_EQ(entries(node.descriptorStorage()), expected);
  }
}

TEST(RequirementContracts, from_descriptor_covers_reducers_hash_windows_and_zero_length_fields) {
  // covers: query.query.7~1
  qTree plan;
  query source(1, "src");
  source.kind     = sourceKind::binFile;
  source.filename = "src.bin";
  source.lSchema.emplace_back(rdb::rField("a", 4, 2, rdb::INTEGER), std::list<token>{});
  plan.push_back(source);
  query other = source;
  other.id    = "other";
  other.lSchema.clear();
  other.lSchema.emplace_back(rdb::rField("b", 8, 2, rdb::DOUBLE), std::list<token>{});
  plan.push_back(other);
  query output(1, "out");
  for (const auto op : {STREAM_AVG, STREAM_MIN, STREAM_MAX, STREAM_SUM}) {
    output.lProgram       = {token(PUSH_STREAM, std::string("src")), token(op)};
    const auto descriptor = output.descriptorFrom(plan);
    ASSERT_EQ(descriptor.size(), 1U);
    EXPECT_EQ(descriptor[0].rname, "out_0");
    EXPECT_EQ(descriptor[0].rtype, rdb::RATIONAL);
    EXPECT_EQ(descriptor[0].rlen, static_cast<int>(sizeof(boost::rational<int>)));
    EXPECT_EQ(descriptor[0].rarray, 1);
  }
  output.lProgram = {token(PUSH_STREAM, std::string("src")), token(PUSH_STREAM, std::string("other")), token(STREAM_HASH)};
  const auto hash = output.descriptorFrom(plan);
  ASSERT_EQ(hash.size(), 2U);
  for (size_t i = 0; i < 2; ++i) {
    EXPECT_EQ(hash[i].rname, "out_" + std::to_string(i));
    EXPECT_EQ(hash[i].rtype, rdb::DOUBLE);
    EXPECT_EQ(hash[i].rlen, 8);
    EXPECT_EQ(hash[i].rarray, 1);
  }
  query mixed(1, "mixed");
  mixed.kind    = sourceKind::binFile;
  mixed.lSchema = source.lSchema;
  mixed.lSchema.emplace_back(rdb::rField("wider", 8, 1, rdb::DOUBLE), std::list<token>{});
  plan.push_back(mixed);
  for (const int length : {-3, 3}) {
    output.lProgram   = {token(PUSH_STREAM, std::string("mixed")), token(STREAM_AGSE, std::pair{1, length})};
    const auto window = output.descriptorFrom(plan);
    ASSERT_EQ(window.size(), 3U);
    for (size_t i = 0; i < 3; ++i) {
      EXPECT_EQ(window[i].rname, "out_" + std::to_string(i));
      EXPECT_EQ(window[i].rtype, rdb::DOUBLE);
      EXPECT_EQ(window[i].rlen, 8);
      EXPECT_EQ(window[i].rarray, 1);
    }
  }
  plan["src"].lSchema.emplace_back(rdb::rField("empty", 0, 1, rdb::INTEGER), std::list<token>{});
  for (const auto op : {PUSH_STREAM, STREAM_TIMEMOVE, STREAM_SUBTRACT, STREAM_DEHASH_DIV, STREAM_DEHASH_MOD, STREAM_ADD}) {
    output.lProgram = {token(PUSH_STREAM, std::string("src"))};
    if (op == STREAM_ADD) output.lProgram.emplace_back(PUSH_STREAM, std::string("other"));
    if (op != PUSH_STREAM) output.lProgram.emplace_back(op, 1);
    const auto descriptor = output.descriptorFrom(plan);
    ASSERT_EQ(descriptor.size(), op == STREAM_ADD ? 2U : 1U);
    EXPECT_EQ(descriptor[0].rtype, rdb::INTEGER);
    EXPECT_EQ(descriptor[0].rlen, 4);
    EXPECT_EQ(descriptor[0].rarray, 2);
    if (op == STREAM_ADD) EXPECT_EQ(descriptor[1].rtype, rdb::DOUBLE);
  }
}
