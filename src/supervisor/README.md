# Wymagania programu RetractorDB supervisor

1. Program supervisora jest programem służącym do kontroli działania systemu RetractorDB w środowisku izolowanego kontenera.
2. Jego docelowym środowiskiem pracy jest wnętrze kontenera w którym zarządza działaniem serwera (xretractor), procesów odpytujących serwer (xqry) oraz narzędzia do analizy rejestrowanych danych binarnych (xtrdb).
3. Program supervisora jest opracowany w języku Go.

program rsupervisor powinien:
- zarządzać procesami xretractor, xqry i xtrdb w systemie,
- udostępniać interfejs API umożliwiający komunikację po protokole REST,
- udostępniać interfejs użytkownika po protokole WWW,
- umożliwiać uruchomienie xretractor z plikiem zapytań (.rql),
- umożliwiać przeładowanie xretractor z nowym plikiem zapytań (.rql) poprzez zatrzymanie procesu przez IPC i uruchomienie go ponownie z nowym plikiem,
- umożliwiać dodanie tymczasowego zapytania adhoc (nie restartuje xretractor),
- umożliwiać podgląd funkcjonujących zapytań w systemie w interfejsie użytkownika WWW,
- umożliwiać podgląd drzewa skompilowanych i funkcjonujących zapytań w systemie w interfejsie użytkownika WWW,
- utworzyć drzewo zapytań za pomocą wywołania xretractor w trybie kompilacji (-c) możliwiając wybór przez użytkownika czy na diagramie pojawią się pola, tagi, streamprogs lub rules.
- być punktem wejścia dla kontenera produkcyjnego,
- udostępniać dane (podgląd) wygenerowane przez xtrdb,
- zapewniać możliwość podglądu ciągłych zapytań prezentowanych przez xqry w interfejsie użytkownika jak i po protokole REST,
- udostępniać dane generowane przez xqry przez TCP socket (kompatybilny z nc) umożliwiający ich odbiór z zewnątrz kontenera,
- umożliwiać prezentację graficzną (wykresy JS charts) i tekstową ciągłych danych przez interfejs użytkownika WWW,
- agregować i udostępniać logi ze wszystkich zarządzanych procesów w interfejsie użytkownika WWW,
- udostępniać endpoint health check (/health) umożliwiający monitorowanie stanu kontenera,
- obsługiwać konfigurację startową przez zmienne środowiskowe (m.in. ścieżka do pliku .rql, port HTTP),
- wykonywać graceful shutdown: przy odebraniu SIGTERM zatrzymać xretractor przez IPC, poczekać na zakończenie procesów i zakończyć działanie,
- umożliwić z interfejsu użytkownika wywołania polecenia zakończenia procesu xretractor prze prorces xqry z opcją -k
- umożliwić z interfejsu użytkownika zakończenia nadzorowanych procesów oraz zakończenia procesu supervisor
- umożliwić z interfejsu użytkownika edycji plików .rql
- 
