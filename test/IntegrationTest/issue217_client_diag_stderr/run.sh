#!/bin/bash
# Diagnostyka xqry musi trafiać na STDERR (issue_217).
#
# Klient logował przez `setupLoggerMain(argv[0], dual=true)`, którego console
# sink to STDOUT, a ścieżki błędu w qryLauncher.cpp używały `std::println` —
# również stdout. Na stderr nie szło nic. Harness pomiarowy uruchamia klienta
# jako `xqry ... >/dev/null 2>xqry.err`, więc KAŻDA awaria klienta wyglądała
# jak zniknięcie bez komunikatu: `xqry.err` miał 0 bajtów także w przebiegach
# zakończonych sukcesem. Ta pozorna cichość zatrzymała dwie kampanie (K6b, K6c)
# i utrzymywała fałszywą hipotezę o crashu klienta pod obciążeniem.
set -e
rm -f out.txt err.txt

status=0
xqry -s nosuchstream_issue217 >out.txt 2>err.txt || status=$?

# Brak serwera (albo nieznany strumień) to awaria — klient nie może wyjść zerem.
[ "$status" -ne 0 ] || {
  echo "FAIL: xqry zakonczyl sie kodem 0 mimo braku serwera"
  exit 1
}

# Sedno regresji: komunikat musi być tam, gdzie szuka go operator i harness.
[ -s err.txt ] || {
  echo "FAIL: xqry nie napisal nic na stderr (kod $status); komunikat poszedl na stdout:"
  cat out.txt
  exit 1
}
