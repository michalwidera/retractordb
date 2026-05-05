# Wymagania programu RetractorDB supervisor

1. Program supervisora jest programem służącym do kontroli działania systemu RetractorDB w środowisku izolowanego kontenera.
2. Jego docelowym środowiskiem pracy jest wnętrze kontenera w którym zarządza działaniem serwera (xretractor), procesów odpytujących serwer (xqry) oraz narzędzia do analizy rejestrowanych danych binarnych (xtrdb).
3. Program supervisora jest opracowany w języku Go.

program rsupervisor powinien:
- zarządzać procesami xretractor, xqry i xtrdb,
- udostępniać interfejs API umożliwiający komunikację i zarządzanie procesami za pomocą protokołu REST,
- udostępniać interfejs API umożliwiający komunikację i zarządzanie procesami za pomocą protokołu gRPC,
- umożliwiać uruchomienie xretractor z plikiem zapytań (.rql) przesłanym przez API jak i zlokalizowanym w systemie plików,
- umożliwiać przeładowanie xretractor z nowym plikiem zapytań (.rql) poprzez zatrzymanie procesu przez xqry (kill) i uruchomienie xretractor ponownie z nowym plikiem,
- umożliwiać dodanie tymczasowego zapytania (adhoc),
- umożliwiać wylistowanie funkcjonujących zapytań wraz informacją o ilości zgromadzonych rekordów (diryaml),
- w przypadku /api/queries dokonać konwersji danych na format json,
- umożliwiać pobranie planów zapytań przez wywołanie xretractor (onlycompile,dot,fields,tags,streamprogs,rules),
- być punktem wejścia dla kontenera produkcyjnego,
- udostępniać dane przez API wygenerowane przez procesy xqry na bieżąco (select),
- uruchamiać i udostępniać dane generowane przez xqry przez TCP socket (kompatybilny z nc) umożliwiający ich odbiór z zewnątrz kontenera poza protokołami REST oraz gRPC,
- agregować i udostępniać ostatnie logi ze wszystkich zarządzanych procesów,
- wykonywać graceful shutdown: przy odebraniu SIGTERM zatrzymać xretractor przez xqry (kill), poczekać na zakończenie procesów i zakończyć działanie xretractor,
- umożliwić wywołanie przez API polecenia graceful shutdown oraz zakończenia procesu supervisor,
