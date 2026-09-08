#pragma once

#include <cstddef>
#include <iosfwd>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

/// Wejscie do parsera RQL (RQLParser.cpp). Naglowek jest JEDYNYM miejscem, w ktorym te
/// symbole sa deklarowane: do 2026-09-07 kazdy konsument robil to u siebie wlasnym `extern`,
/// a przy trzech przeciazeniach `parserRQLString` taka deklaracja nie tylko moze rozjechac
/// sie z definicja — moze po cichu wskazac INNE przeciazenie, bo przeciazenia nie daja
/// bledu linkowania, tylko wybor. RQLParser.cpp takze wlacza ten naglowek, wiec definicje
/// sa sprawdzane wzgledem deklaracji.

class qTree;

/// Parsuje JEDNA porcje tekstu RQL. `firstLine` to numer wiersza, na ktorym ta porcja stoi
/// w pliku zrodlowym — wolajacy, ktory tnie plik na instrukcje (parsePlanText,
/// parserRQLFile_4Test), podaje tu pozycje instrukcji, reszta zostawia 1.
///
/// Zwraca {status, pierwsze slowo kluczowe, nazwa strumienia}. Status "OK" albo tresc bledu;
/// `statementKeywords` dostaje slowa kluczowe wszystkich instrukcji porcji.
std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &inlet,
                                                                  std::vector<std::string> &statementKeywords, size_t firstLine);

/// Jak wyzej, z `firstLine` rownym 1.
std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &inlet,
                                                                  std::vector<std::string> &statementKeywords);

/// Jak wyzej, bez zbierania slow kluczowych.
std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &inlet);

/// Wiersze logiczne pliku RQL: komentarze `#` usuniete, kontynuacje `\` sklejone. Z kazda
/// instrukcja wraca numer wiersza PLIKU, na ktorym sie ona zaczyna — powody obu regul stoja
/// przy definicji.
std::vector<std::pair<std::string, size_t>> readLogicalLines(std::istream &file);

/// Wczytuje i parsuje CALY plik RQL. Istnieje wylacznie dla testow jednostkowych: droga
/// produkcyjna prowadzi przez parsePlanText (planSource.hpp), ktora dodatkowo zapamietuje
/// wiersze zrodlowe planu.
std::string parserRQLFile_4Test(qTree &coreInstance, const std::string &sInputFile);
