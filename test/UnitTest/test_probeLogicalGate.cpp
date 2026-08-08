#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <boost/rational.hpp>

#include "rdb/descriptor.hpp"
#include "rdb/probe.hpp"
#include "rdb/storage.hpp"

// ctest -R '^ut_probeLogicalGate' -V

//
// Bramka aparatury dla metryki pierwotnej K23/H9 (research_plan.md §10).
//
// Różnica wobec test_probe.cpp: tam liczniki są wołane BEZPOŚREDNIO, więc sprawdzają samą
// arytmetykę sondy. Tu każdy przypadek przechodzi PRAWDZIWĄ ścieżką zapisu `storage::write()`,
// bo §10 pyta o „rzeczywisty zapis jednego rekordu pośredniego podczas wykonania planu".
// Defekt wpięcia (zła rola, zgubiony zapis, policzony rekord pochłonięty przez detekcję gap)
// jest widoczny tylko na tym poziomie — licznik sam z siebie przechodziłby dalej.
//
// Każdy przypadek działa w OBU wariantach kompilacji. Przy wyłączonej sondzie oczekiwaną
// wartością jest zero: to kontrola, że instrument nie przecieka do produkcji. Stąd mnożnik
// `probeOn` — kontrakt jest dwustronny, nie ma tu GTEST_SKIP.
//
namespace {

constexpr unsigned long long probeOn = rdb_probe_materialize ? 1 : 0;

/// Magazyn zapisywalny o jednorazowym życiu: `setDisposable(true)` kasuje plik danych,
/// `.desc` i `.meta` w destruktorze, więc kolejne przypadki startują od czystego stanu.
class writableStorage {
 public:
  writableStorage(const std::string &name, rdb::Descriptor descriptor, bool substrate)
      : descriptor_(std::move(descriptor)),
        storage_(name, name, "") {
    storage_.attachDescriptor(&descriptor_);
    storage_.setDisposable(true);
    storage_.markAsSubstrate(substrate);
  }

  /// Zapis rekordu z jawną mapą NULL. `allNull` wyzwala maszynę detekcji gap, jeżeli
  /// magazyn ją skonfigurował.
  void write(bool allNull = false) {
    auto *payload = storage_.getPayload();
    payload->setNullBitset(std::vector<bool>(descriptor_.size(), allNull));
    storage_.write();
  }

  /// Nadpisanie rekordu o podanym indeksie — druga gałąź `storage::write()`.
  void overwrite(std::size_t recordIndex) {
    auto *payload = storage_.getPayload();
    payload->setNullBitset(std::vector<bool>(descriptor_.size(), false));
    storage_.write(recordIndex);
  }

  void configureGapDetection(int nullFillCount) { storage_.configureGapDetection(boost::rational<int>(1, 1), nullFillCount); }

  [[nodiscard]] std::size_t canonicalBytes() const { return rdb::probe::canonicalRecordBytes(descriptor_); }

 private:
  rdb::Descriptor descriptor_;
  rdb::storage storage_;
};

rdb::Descriptor oneInteger() { return rdb::Descriptor{{"a", 4, 1, rdb::INTEGER}}; }

rdb::Descriptor memoryBackedInteger() {
  // Pole TYPE nie jest danymi: attachStorage() czyta z niego nazwę typu magazynu, więc
  // `rname == "MEMORY"` wybiera backend pamięciowy (accessorFactory.cc).
  return rdb::Descriptor{{"a", 4, 1, rdb::INTEGER}, {"MEMORY", 0, 0, rdb::TYPE}};
}

}  // namespace

// ── Zero zapisów ────────────────────────────────────────────────────────────────

TEST(probeLogicalGate, storage_without_a_write_reports_nothing) {
  rdb::probe::logicalWriteReset();
  const writableStorage stream("gate_no_write", oneInteger(), false);

  const auto l = rdb::probe::logicalWriteReport();
  EXPECT_EQ(l.publicAppends, 0u);
  EXPECT_EQ(l.publicBytes, 0u);
  EXPECT_EQ(l.substrateAppends, 0u);
  EXPECT_EQ(l.substrateBytes, 0u);
}

// ── Jeden zapis, obie role ──────────────────────────────────────────────────────

TEST(probeLogicalGate, single_public_append_costs_one_canonical_record) {
  rdb::probe::logicalWriteReset();
  writableStorage stream("gate_public_one", oneInteger(), false);
  const auto expected = stream.canonicalBytes();
  stream.write();

  const auto l = rdb::probe::logicalWriteReport();
  EXPECT_EQ(l.publicAppends, 1 * probeOn);
  EXPECT_EQ(l.publicBytes, expected * probeOn);
  EXPECT_EQ(l.substrateAppends, 0u);
  EXPECT_EQ(l.substrateBytes, 0u);
}

TEST(probeLogicalGate, single_substrate_append_lands_on_the_substrate_counter) {
  rdb::probe::logicalWriteReset();
  writableStorage substrate("gate_substrate_one", oneInteger(), true);
  const auto expected = substrate.canonicalBytes();
  substrate.write();

  const auto l = rdb::probe::logicalWriteReport();
  EXPECT_EQ(l.substrateAppends, 1 * probeOn);
  EXPECT_EQ(l.substrateBytes, expected * probeOn);
  EXPECT_EQ(l.publicAppends, 0u);
  EXPECT_EQ(l.publicBytes, 0u);
}

TEST(probeLogicalGate, roles_do_not_bleed_into_each_other) {
  // Mianownik metryki (rekordy publiczne) i licznik (bajty substratu) muszą pozostać
  // rozdzielne w jednym procesie — plan K23 ma zawsze oba rodzaje strumieni naraz.
  rdb::probe::logicalWriteReset();
  writableStorage substrate("gate_mixed_sub", oneInteger(), true);
  writableStorage stream("gate_mixed_pub", oneInteger(), false);
  const auto recordBytes = substrate.canonicalBytes();

  substrate.write();
  substrate.write();
  stream.write();

  const auto l = rdb::probe::logicalWriteReport();
  EXPECT_EQ(l.substrateAppends, 2 * probeOn);
  EXPECT_EQ(l.substrateBytes, 2 * recordBytes * probeOn);
  EXPECT_EQ(l.publicAppends, 1 * probeOn);
  EXPECT_EQ(l.publicBytes, recordBytes * probeOn);
}

// ── Q kopii tego samego podplanu ────────────────────────────────────────────────
//
// Geometria progu H9: bez współdzielenia Q fizycznych instancji zapisuje Q×b na slot,
// po scaleniu b. Próg 40% przy Q=8 odpowiada spadkowi 1−1/8 = 87,5%.
//

TEST(probeLogicalGate, two_instances_of_a_subplan_cost_twice) {
  rdb::probe::logicalWriteReset();
  writableStorage first("gate_q2_a", oneInteger(), true);
  writableStorage second("gate_q2_b", oneInteger(), true);
  const auto recordBytes = first.canonicalBytes();

  first.write();
  second.write();

  EXPECT_EQ(rdb::probe::logicalWriteReport().substrateBytes, 2 * recordBytes * probeOn);
}

TEST(probeLogicalGate, four_instances_of_a_subplan_cost_four_times) {
  rdb::probe::logicalWriteReset();
  const auto descriptor  = oneInteger();
  const auto recordBytes = rdb::probe::canonicalRecordBytes(descriptor);

  {
    writableStorage a("gate_q4_a", descriptor, true);
    writableStorage b("gate_q4_b", descriptor, true);
    writableStorage c("gate_q4_c", descriptor, true);
    writableStorage d("gate_q4_d", descriptor, true);
    a.write();
    b.write();
    c.write();
    d.write();
  }

  const auto l = rdb::probe::logicalWriteReport();
  EXPECT_EQ(l.substrateAppends, 4 * probeOn);
  EXPECT_EQ(l.substrateBytes, 4 * recordBytes * probeOn);
}

TEST(probeLogicalGate, eight_instances_match_the_decisive_q) {
  // Q=8 jest punktem rozstrzygającym progu H9, więc ma własny przypadek o znanej odpowiedzi.
  rdb::probe::logicalWriteReset();
  const auto descriptor  = oneInteger();
  const auto recordBytes = rdb::probe::canonicalRecordBytes(descriptor);

  {
    // Wskaźniki, bo `rdb::storage` jest niekopiowalne i nieprzenoszalne (trzyma
    // unique_ptr na accessor i payload), więc nie wejdzie do wektora po wartości.
    std::vector<std::unique_ptr<writableStorage>> instances;
    instances.reserve(8);
    for (int instance = 0; instance < 8; ++instance)
      instances.push_back(std::make_unique<writableStorage>("gate_q8_" + std::to_string(instance), descriptor, true));
    for (auto &instance : instances)
      instance->write();
  }

  const auto l = rdb::probe::logicalWriteReport();
  EXPECT_EQ(l.substrateAppends, 8 * probeOn);
  EXPECT_EQ(l.substrateBytes, 8 * recordBytes * probeOn);
}

TEST(probeLogicalGate, one_shared_instance_is_eight_times_cheaper_than_eight) {
  // Ten sam pomiar po scaleniu — to jest liczba, którą kampania porówna z powyższą.
  const auto descriptor  = oneInteger();
  const auto recordBytes = rdb::probe::canonicalRecordBytes(descriptor);

  rdb::probe::logicalWriteReset();
  {
    writableStorage shared("gate_q8_shared", descriptor, true);
    shared.write();
  }
  const auto shared = rdb::probe::logicalWriteReport();

  EXPECT_EQ(shared.substrateAppends, 1 * probeOn);
  EXPECT_EQ(shared.substrateBytes, recordBytes * probeOn);
}

// ── Różne szerokości rekordu ────────────────────────────────────────────────────

TEST(probeLogicalGate, integer_record_width_matches_the_serializer) {
  rdb::probe::logicalWriteReset();
  writableStorage stream("gate_width_int", rdb::Descriptor{{"a", 4, 1, rdb::INTEGER}}, true);
  stream.write();

  EXPECT_EQ(rdb::probe::logicalWriteReport().substrateBytes, (8u + 1u) * probeOn);
}

TEST(probeLogicalGate, double_record_width_matches_the_serializer) {
  rdb::probe::logicalWriteReset();
  writableStorage stream("gate_width_double", rdb::Descriptor{{"a", 8, 1, rdb::DOUBLE}}, true);
  stream.write();

  EXPECT_EQ(rdb::probe::logicalWriteReport().substrateBytes, (8u + 1u) * probeOn);
}

TEST(probeLogicalGate, string_record_width_matches_the_serializer) {
  rdb::probe::logicalWriteReset();
  writableStorage stream("gate_width_string", rdb::Descriptor{{"s", 16, 1, rdb::STRING}}, true);
  stream.write();

  EXPECT_EQ(rdb::probe::logicalWriteReport().substrateBytes, (16u + 1u) * probeOn);
}

TEST(probeLogicalGate, multi_field_record_width_matches_the_serializer) {
  rdb::probe::logicalWriteReset();
  writableStorage stream("gate_width_multi",
                         rdb::Descriptor{{"a", 4, 1, rdb::INTEGER}, {"b", 8, 1, rdb::DOUBLE}, {"s", 8, 1, rdb::STRING}}, true);
  const auto expected = stream.canonicalBytes();
  stream.write();

  EXPECT_EQ(expected, 8u + 8u + 8u + 1u);
  EXPECT_EQ(rdb::probe::logicalWriteReport().substrateBytes, expected * probeOn);
}

TEST(probeLogicalGate, wider_records_cost_strictly_more) {
  const auto narrow = rdb::probe::canonicalRecordBytes(rdb::Descriptor{{"a", 4, 1, rdb::INTEGER}});
  const auto wide   = rdb::probe::canonicalRecordBytes(rdb::Descriptor{{"a", 4, 1, rdb::INTEGER}, {"b", 8, 1, rdb::DOUBLE}});

  EXPECT_LT(narrow, wide) << "szersza materializacja musi być droższa, inaczej metryka nie mierzy objętości";
}

TEST(probeLogicalGate, two_records_of_the_same_stream_sum_up) {
  rdb::probe::logicalWriteReset();
  writableStorage stream("gate_two_records", oneInteger(), true);
  const auto recordBytes = stream.canonicalBytes();

  stream.write();
  stream.write();

  const auto l = rdb::probe::logicalWriteReport();
  EXPECT_EQ(l.substrateAppends, 2 * probeOn);
  EXPECT_EQ(l.substrateBytes, 2 * recordBytes * probeOn);
}

// ── NULL i luka ─────────────────────────────────────────────────────────────────

TEST(probeLogicalGate, null_values_do_not_change_the_canonical_width) {
  // Mapa NULL ma stałą szerokość, więc rekord z wartościami puste kosztuje tyle samo.
  // Metryka pierwotna ma być deterministyczna — inaczej iloraz zależałby od danych.
  const auto descriptor  = oneInteger();
  const auto recordBytes = rdb::probe::canonicalRecordBytes(descriptor);

  rdb::probe::logicalWriteReset();
  {
    writableStorage stream("gate_null_value", descriptor, true);
    stream.write(true);  // rekord all-null, ale BEZ detekcji gap — trafia do magazynu
  }

  const auto l = rdb::probe::logicalWriteReport();
  EXPECT_EQ(l.substrateAppends, 1 * probeOn);
  EXPECT_EQ(l.substrateBytes, recordBytes * probeOn);
}

TEST(probeLogicalGate, nullfill_records_are_real_writes) {
  // Faza nullfill: rekordy all-null PRZED oznaczeniem przerwy trafiają do magazynu,
  // więc są zapisami i muszą być policzone.
  const auto descriptor  = oneInteger();
  const auto recordBytes = rdb::probe::canonicalRecordBytes(descriptor);

  rdb::probe::logicalWriteReset();
  {
    writableStorage stream("gate_nullfill", descriptor, true);
    stream.configureGapDetection(2);
    stream.write(true);
    stream.write(true);
  }

  const auto l = rdb::probe::logicalWriteReport();
  EXPECT_EQ(l.substrateAppends, 2 * probeOn);
  EXPECT_EQ(l.substrateBytes, 2 * recordBytes * probeOn);
}

TEST(probeLogicalGate, records_absorbed_into_a_gap_are_not_writes) {
  // NAJWAŻNIEJSZY przypadek bramki. Rekord all-null pochłonięty przez maszynę gap nigdy
  // nie dociera do magazynu, więc nie jest materializacją. Policzenie go zawyżałoby
  // metrykę pierwotną tym bardziej, im więcej przerw ma źródło — czyli różnicowałoby
  // profile ablacyjne wielkością niezwiązaną ze współdzieleniem.
  const auto descriptor  = oneInteger();
  const auto recordBytes = rdb::probe::canonicalRecordBytes(descriptor);

  rdb::probe::logicalWriteReset();
  {
    writableStorage stream("gate_gap_absorbed", descriptor, true);
    stream.configureGapDetection(2);
    stream.write(true);  // nullfill 1 — zapis
    stream.write(true);  // nullfill 2 — zapis
    stream.write(true);  // pochłonięty do przerwy — NIE zapis
    stream.write(true);  // pochłonięty do przerwy — NIE zapis
  }

  const auto l = rdb::probe::logicalWriteReport();
  EXPECT_EQ(l.substrateAppends, 2 * probeOn) << "pochłonięte rekordy nie są zapisami";
  EXPECT_EQ(l.substrateBytes, 2 * recordBytes * probeOn);
}

// ── Nadpisanie ──────────────────────────────────────────────────────────────────

TEST(probeLogicalGate, overwrite_is_counted_with_its_bytes) {
  // To jest różnica wobec licznika natywnego, bez której substrat na buforze kołowym
  // raportowałby zero bajtów po zawinięciu bufora.
  const auto descriptor  = oneInteger();
  const auto recordBytes = rdb::probe::canonicalRecordBytes(descriptor);

  rdb::probe::logicalWriteReset();
  {
    writableStorage stream("gate_overwrite", descriptor, true);
    stream.write();       // rekord 0 — dopisanie
    stream.overwrite(0);  // ten sam rekord — nadpisanie
  }

  const auto l = rdb::probe::logicalWriteReport();
  EXPECT_EQ(l.substrateAppends, 1 * probeOn);
  EXPECT_EQ(l.substrateOverwrites, 1 * probeOn);
  EXPECT_EQ(l.substrateBytes, 2 * recordBytes * probeOn) << "nadpisanie jest zapisem rekordu";
}

TEST(probeLogicalGate, overwrite_keeps_the_role_of_its_stream) {
  const auto descriptor  = oneInteger();
  const auto recordBytes = rdb::probe::canonicalRecordBytes(descriptor);

  rdb::probe::logicalWriteReset();
  {
    writableStorage stream("gate_overwrite_public", descriptor, false);
    stream.write();
    stream.overwrite(0);
  }

  const auto l = rdb::probe::logicalWriteReport();
  EXPECT_EQ(l.publicOverwrites, 1 * probeOn);
  EXPECT_EQ(l.publicBytes, 2 * recordBytes * probeOn);
  EXPECT_EQ(l.substrateBytes, 0u);
}

// ── Niezależność od backendu magazynu ───────────────────────────────────────────

TEST(probeLogicalGate, memory_backed_storage_reports_the_same_canonical_bytes) {
  // Sedno metryki pierwotnej: §10 degraduje natywne `mat_bytes` właśnie dlatego, że
  // zależą od reprezentacji backendu. Ten sam rekord w magazynie trwałym i pamięciowym
  // musi dać tę samą liczbę kanoniczną, inaczej porównanie z Flinkiem jest bez sensu.
  rdb::probe::logicalWriteReset();
  {
    writableStorage persistent("gate_backend_disk", oneInteger(), true);
    persistent.write();
  }
  const auto onDisk = rdb::probe::logicalWriteReport().substrateBytes;

  rdb::probe::logicalWriteReset();
  {
    writableStorage inMemory("gate_backend_mem", memoryBackedInteger(), true);
    inMemory.write();
  }
  const auto inMemory = rdb::probe::logicalWriteReport().substrateBytes;

  EXPECT_EQ(onDisk, inMemory);
  EXPECT_EQ(onDisk, (8u + 1u) * probeOn);
}

// ── Higiena licznika w kampanii ─────────────────────────────────────────────────

TEST(probeLogicalGate, reset_between_cells_leaves_no_residue) {
  // Kampania zeruje licznik między komórkami. Przeciek zawyżałby każdą następną komórkę
  // w bloku, a objawem byłby „wynik zależny od kolejności".
  writableStorage stream("gate_reset_residue", oneInteger(), true);
  stream.write();

  rdb::probe::logicalWriteReset();

  const auto l = rdb::probe::logicalWriteReport();
  EXPECT_EQ(l.substrateAppends, 0u);
  EXPECT_EQ(l.substrateBytes, 0u);
  EXPECT_EQ(l.publicAppends, 0u);
  EXPECT_EQ(l.publicBytes, 0u);
}

TEST(probeLogicalGate, counters_accumulate_across_storage_lifetimes) {
  // Liczniki są procesowe: zamknięcie magazynu nie może gubić jego zapisów, bo kampania
  // odczytuje sumę po zakończeniu przebiegu.
  const auto descriptor  = oneInteger();
  const auto recordBytes = rdb::probe::canonicalRecordBytes(descriptor);

  rdb::probe::logicalWriteReset();
  {
    writableStorage first("gate_lifetime_a", descriptor, true);
    first.write();
  }
  {
    writableStorage second("gate_lifetime_b", descriptor, true);
    second.write();
  }

  EXPECT_EQ(rdb::probe::logicalWriteReport().substrateBytes, 2 * recordBytes * probeOn);
}
