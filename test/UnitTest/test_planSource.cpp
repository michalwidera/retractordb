#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "retractor/lib/planSource.hpp"
#include "retractor/lib/qTree.hpp"

/// @brief Wczytywanie zestawu RQL z tekstu — wspolne zrodlo startu z pliku i przeladowania
/// w locie (`xqry --reset`).
///
/// Przedmiotem badania jest jedna rzecz, ktora rozni sie tu od dawnego kodu launchera: zestaw
/// PUSTY nie jest bledem. Do 2026-09-05 pusty plik zapytan konczyl proces bledem parsowania
/// ("Empty file."), przez co udokumentowana w jednostce systemd sciezka "pusty plik = tryb
/// bezczynny" nie dzialala wcale, a Restart=on-failure zapetlal start uslugi.

TEST(PlanSource, empty_text_is_an_empty_plan_not_an_error) {
  qTree plan;
  const PlanSource loaded = parsePlanText(plan, "");

  EXPECT_EQ(loaded.status, "OK");
  EXPECT_TRUE(loaded.lines.empty());
  EXPECT_TRUE(plan.empty());
}

TEST(PlanSource, comments_and_blank_lines_alone_are_an_empty_plan) {
  qTree plan;
  const PlanSource loaded = parsePlanText(plan, "# tylko komentarz\n\n   \n# i jeszcze jeden\n");

  EXPECT_EQ(loaded.status, "OK");
  EXPECT_TRUE(loaded.lines.empty());
  EXPECT_TRUE(plan.empty());
}

TEST(PlanSource, statements_land_in_the_plan_and_in_the_line_list) {
  qTree plan;
  const PlanSource loaded = parsePlanText(plan,
                                          "DECLARE a INTEGER STREAM src, 1 FILE 'data.txt'\n"
                                          "SELECT a+1 STREAM dst FROM src\n");

  ASSERT_EQ(loaded.status, "OK");
  ASSERT_EQ(loaded.lines.size(), 2U);
  EXPECT_EQ(loaded.lines[0].first, "src");
  EXPECT_EQ(loaded.lines[1].first, "dst");
  EXPECT_TRUE(plan.exists("src"));
  EXPECT_TRUE(plan.exists("dst"));
}

TEST(PlanSource, a_broken_statement_stops_the_load_and_reports_the_reason) {
  qTree plan;
  const PlanSource loaded = parsePlanText(plan, "SELECT ((( STREAM dst FROM src\n");

  EXPECT_NE(loaded.status, "OK");
  EXPECT_TRUE(loaded.lines.empty());
}

/// Instrukcje po pierwszej bledna nie sa juz czytane. Zestaw czesciowo wczytany bylby gorszy
/// od zadnego: skompilowalby sie i policzyl COS INNEGO, niz operator zapisal w pliku.
TEST(PlanSource, load_stops_at_the_first_bad_statement) {
  qTree plan;
  const PlanSource loaded = parsePlanText(plan,
                                          "DECLARE a INTEGER STREAM src, 1 FILE 'data.txt'\n"
                                          "SELECT ((( STREAM broken FROM src\n"
                                          "SELECT a+2 STREAM never FROM src\n");

  EXPECT_NE(loaded.status, "OK");
  ASSERT_EQ(loaded.lines.size(), 1U);
  EXPECT_EQ(loaded.lines[0].first, "src");
  EXPECT_FALSE(plan.exists("never"));
}

/// Nazwy roszczone na magistrali to wszystkie wezly poza dyrektywami: dyrektywa nie jest
/// strumieniem i nie ma czego roscic, a nazwanie jej tak konczyloby sie kolizja kazdych
/// dwoch planow uzywajacych STORAGE.
TEST(PlanSource, directives_do_not_claim_stream_names) {
  qTree plan;
  const PlanSource loaded = parsePlanText(plan,
                                          "STORAGE 'temp'\n"
                                          "DECLARE a INTEGER STREAM src, 1 FILE 'data.txt'\n");
  ASSERT_EQ(loaded.status, "OK");

  const std::vector<std::string> claimed = planStreamNames(plan);
  EXPECT_EQ(claimed.size(), 1U);
  EXPECT_EQ(claimed.front(), "src");
  EXPECT_TRUE(planCounterPath(plan).empty());
}
