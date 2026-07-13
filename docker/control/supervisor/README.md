# Wymagania programu RetractorDB supervisor

1. Program supervisora jest programem służącym do kontroli działania systemu RetractorDB w środowisku izolowanego kontenera.
2. Jego docelowym środowiskiem pracy jest wnętrze kontenera w którym zarządza działaniem serwera (xretractor), procesów odpytujących serwer (xqry).
3. Program supervisora jest opracowany w języku Go.

program rsupervisor powinien:
- zarządzać procesami xretractor, xqry oraz uruchomionym serisem xretractor.
- udostępniać interfejs API umożliwiający komunikację i zarządzanie procesami za pomocą protokołu REST API.
- udostępniać wszystkie funkcjonalności xretractor i xqry przez REST API.
- umożliwiać przesłanie i uruchomienie pliku zapytań (.rql) przez API (REST: POST /api/file).
- umożliwiać dodanie tymczasowego zapytania (adhoc).
- umożliwiać wylistowanie funkcjonujących zapytań wraz informacją o ilości zgromadzonych rekordów (diryaml)
- odpowiedzi systemu po protokole REST powinny być prezentowane w postaci json.
- umożliwiać pobranie planów zapytań przez wywołanie xretractor.
- być punktem wejścia dla kontenera produkcyjnego.
- uruchamiać i udostępniać udostępniać dane przez REST API wygenerowane przez procesy xqry na bieżąco (select) dla klientów.
- uruchamiać i udostępniać dane generowane przez xqry wysłany na TCP socket (xqry -s stream|nc -l port) umożliwiający ich odbiór z zewnątrz kontenera.
- wykonywać graceful shutdown po otrzymaniu żądania za pomocą REST API i ustawić kontener w trybie jak po pierwszym uruchomieniu instalacji z działającym serwisem xretractor bez działających klientów.

