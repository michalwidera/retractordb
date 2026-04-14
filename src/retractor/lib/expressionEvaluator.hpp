#pragma once

#include "fldType.hpp"
#include "rdb/payload.hpp"
#include "token.hpp"  // token, std::list

/// @brief Klasa opisująca proces wyznaczania wartości wyrażenia.
///
/// obiekt klasy expressionEvaluator powinien:
/// - przyjmować listę tokenów reprezentujących wyrażenie do wyznaczenia.
/// - przyjmować opcjonalny wskaźnik do obiektu payload, który może być używany do pobierania wartości zmiennych w wyrażeniu.
/// - obsługiwać różne operatory arytmetyczne, logiczne i porównania, takie jak dodawanie, odejmowanie, mnożenie, dzielenie, negację, porównania równości, nierówności, mniejszości, większości itp.
/// - obsługiwać różne typy danych, takie jak liczby całkowite, zmiennoprzecinkowe, łańcuchy znaków itp., zgodnie z typami obsługiwanymi przez payload.
/// - zarządzać pamięcią w sposób efektywny, aby uniknąć wycieków pamięci.
/// - zwracać wynik wyrażenia jako obiekt typu rdb::descFldVT, który może reprezentować różne typy danych.
/// - być zoptymalizowany pod kątem wydajności, aby nie wprowadzać nadmiernych opóźnień w przetwarzaniu wyrażeń, zwłaszcza w przypadku złożonych wyrażeń lub dużych ilości danych w payload.
/// - obsługiwać błędy, takie jak nieprawidłowe tokeny, niezgodne typy danych, dzielenie przez zero itp., i zwracać odpowiednie komunikaty błędów lub wyjątki w takich przypadkach.
/// - być projektowany z myślą o łatwej integracji z innymi komponentami systemu, takimi jak streamInstance, aby umożliwić efektywne wyznaczanie wartości wyrażeń w różnych kontekstach, takich jak warunki reguł, obliczenia pól wyjściowych itp.
///
/// @note Każde wyrażenie składa się z operatorów i operandów, reprezentowanych jako tokeny. Wykorzystuje stos do przechowywania pośrednich wyników obliczeń. Obsługuje operatory arytmetyczne, logiczne i porównania, a także negację. Umożliwia normalizację typów operandów przed wykonaniem operacji, zapewniając zgodność typów. Może być rozszerzona o dodatkowe operatory i funkcje w przyszłości.
/// @note W wyrażeniach mogą się pojawić wartości null, które powinny być obsługiwane zgodnie z zasadami logiki trójwartościowej (3VL). Porównanie wartości null z inną wartością zwraca null. Operacje logiczne z udziałem null są obsługiwane zgodnie z SQL 3VL: true OR null = true, false AND null = false, w pozostałych przypadkach wynik jest null (np. true AND null = null, false OR null = null).
/// @note Logika trójwartościowa (3VL) w SQL obsługuje wartość NULL jako trzeci stan – „nieznany” (unknown), obok prawdy (TRUE) i fałszu (FALSE). Występuje, gdy brakuje danych, a porównania z NULL (np. x = NULL) dają wynik nieznany, a nie fałsz.
class expressionEvaluator {
 private:
  /* data */
 public:
  expressionEvaluator(/* args */);
  ~expressionEvaluator();

  rdb::descFldVT eval(std::list<token> program, rdb::payload *payload = nullptr);
};
