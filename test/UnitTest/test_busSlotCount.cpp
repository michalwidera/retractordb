// Tak jak test_busFallback: prywatny uklad sluzy tylko do podmiany naglowka
// przez drugie mapowanie. Nie dodajemy haka ani publicznego API do produkcji.
#include "retractor/lib/bus.cpp"

#include "busRepairSeqlockTests.hpp"
#include "busSlotCountTests.hpp"
