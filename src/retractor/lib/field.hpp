#pragma once

#include <list>

#include "rdb/descriptor.hpp"
#include "token.hpp"

/// Obiekt klasy field powinien:
/// - przechowywać definicję pola rField oraz program wyliczający wartość pola jako listę tokenów.
///   Test: `field.single_token_constructor_exposes_first_token`, `field.list_constructor_preserves_program_order`.
/// - przy konstrukcji z pojedynczym tokenem tworzyć program złożony z tego tokenu, a przy konstrukcji z listą tokenów przejmować listę bez zmiany kolejności.
///   Test: `field.single_token_constructor_exposes_first_token`, `field.list_constructor_preserves_program_order`.
/// - przez getFirstFieldToken zwracać pierwszy token programu, a wywołanie przy pustym programie traktować jako naruszenie niezmiennika (RDB_ASSERT).
///   Test: `fieldDeathTest.first_token_of_an_empty_program_is_an_invariant_failure`.
/// - wypisywać pole w postaci diagnostycznej FLD/name:<nazwa>,type:<kod typu>,prog:<tokeny rozdzielone przecinkami>.
///   Test: `field.stream_output_serializes_metadata_and_program`, `field.stream_output_handles_empty_program`.
struct field {
  std::list<token> lProgram;

  rdb::rField field_;

  explicit field(rdb::rField field_, const token &lToken);
  explicit field(rdb::rField field_, std::list<token> lProgram);

  token getFirstFieldToken();

  friend std::ostream &operator<<(std::ostream &os, const rdb::descFld &s);
  friend std::ostream &operator<<(std::ostream &os, const field &s);
};
