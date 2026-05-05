# Wymagania programu RetractorDB supervisor

1. Program supervisora jest programem służącym do kontroli działania systemu RetractorDB w środowisku izolowanego kontenera.
2. Jego docelowym środowiskiem pracy jest wnętrze kontenera w którym zarządza działaniem serwera (xretractor), procesów odpytujących serwer (xqry) oraz narzędzia do analizy rejestrowanych danych binarnych (xtrdb).
3. Program supervisora jest opracowany w języku Go.

program rsupervisor powinien:
- zarządzać procesami xretractor, xqry i xtrdb,
- udostępniać interfejs API umożliwiający komunikację i zarządzanie procesami za pomocą protokołu REST,
- udostępniać interfejs API umożliwiający komunikację i zarządzanie procesami za pomocą protokołu gRPC,
- umożliwiać uruchomienie xretractor z plikiem zapytań (.rql) przesłanym przez API jak i zlokalizowanym w systemie plików,
- umożliwiać przeładowanie xretractor z nowym plikiem zapytań (.rql) poprzez zatrzymanie procesu przez xqry -k i uruchomienie go ponownie z nowym plikiem,
- umożliwiać dodanie tymczasowego zapytania adhoc (nie restartuje xretractor),
- umożliwiać wylistowanie skompilowanych i funkcjonujących zapytań w systemie przez wywołanie xqry,
- udostępnić drzewo zapytań za pomocą wywołania xretractor w trybie kompilacji (-c) z odpowiednimi parametrami,
- być punktem wejścia dla kontenera produkcyjnego,
- udostępniać dane przez API wygenerowane przez procesy xqry na bieżąco,
- uruchamiać i udostępniać dane generowane przez xqry przez TCP socket (kompatybilny z nc) umożliwiający ich odbiór z zewnątrz kontenera poza protokołami REST oraz gRPC,
- agregować i udostępniać logi ze wszystkich zarządzanych procesów,
- wykonywać graceful shutdown: przy odebraniu SIGTERM zatrzymać xretractor przez xqry -k, poczekać na zakończenie procesów i zakończyć działanie,
- umożliwić wywołanie przez API polecenia zakończenia procesu xretractor przez proces xqry z opcją -k,
- umożliwić wywołanie przez API zakończenia nadzorowanych procesów oraz zakończenia procesu supervisor
