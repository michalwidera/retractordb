# Wymagania programu uiRdbApp - User Interface Application

1. Program uiRdbApp jest programem służącym do komunikacji z programem supervisora,
2. Komunikacja jest realizowana za pomocą protokołów gRPC
3. Program służy do testowania i zaprezentowania sposobu komunikacj ze kontenerem zawierającym retractorDB i program supervisora,
4. Program powinien być przygotowany do działania poza kontenerem, ale powienien umożliwiać komunikację i prezentowanie danych dostarczanych przez supervisor

Program uiRdbApp powinien:
- komunikować się z systemem w którym działa supervisor za pomocą gRPC,
- umożliwiać załadowanie, przesłanie i uruchomienie wewnątrz kontenera zapytania w Języku RQL,
- umożliwiać zatrzymanie i zamknięcie wszystkich działających procesów wewnątrz kontenera, 
- prezentować swoje funkcje w postaci menu u góry programu oraz wyników w zakładkach,
- umożliwiać prezentację napływających danych na wykresach graficznych,
- umożliwiać prezentację napływających danych w tabelach,
- umożliwiać pobranie ze supervisora schematu prezentowanych danych (kolejności i typów kolumn dla formatów CSV, JSON i RAW),
- umożliwiać prezentację planu realizacji zapytań realizowanych w systemie w postaci graficznej (graphivz/dot/webview z embedded graphiz-js)
- prezentacje powinny być dostępne na różnych zakładkach i aktualizowane w tle,
