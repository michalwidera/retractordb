#!/bin/bash

STREAM=${1:-str1}
QUERY=${2:-query.rql}
SIZE=${3:-50,200}
XQRY_EXTRA_FLAGS=${4:-}

# Kazde uruchomienie dostaje wlasna nazwe instancji, domyslnie z katalogu roboczego celu.
# Bez nazwy dwa cele uruchomione rownolegle (np. `ninja dsp` i `ninja simple`) walcza o ta sama
# blokade i te same obiekty IPC: drugi serwer w ogole nie wstaje, jego xqry trafia do cudzej
# instancji, a koncowe sprzatanie zabija cudzy serwer. Nazwa musi spelniac regule xretractor
# (mala litera na poczatku, dalej [a-z0-9_-], najwyzej 32 znaki), stad normalizacja.
NAME=${5:-$(basename "$PWD")}
NAME=$(printf '%s' "$NAME" | tr 'A-Z' 'a-z' | tr -c 'a-z0-9_-' '_' | cut -c1-32)
case "$NAME" in [a-z]*) ;; *) NAME=$(printf 'x%s' "$NAME" | cut -c1-32) ;; esac

if ! xretractor $QUERY -c -r ; then exit 1 ; fi

if ! which gnuplot > /dev/null ; then echo "install gnuplot!" ; exit 1 ; fi

# Odsiew PRZED czynnoscia niszczaca. `rm -rf temp` kasuje magazyn celu, wiec nie moze wykonac
# sie w sytuacji, w ktorej i tak nie wystartujemy: drugie uruchomienie tego samego celu zabralo by
# dane dzialajacej instancji, zanim xretractor zdazylby odmowic startu na blokadzie.
if xqry --server "$NAME" -l > /dev/null 2>&1 ; then
    echo "xplot: instance '$NAME' is already running; stop it (xqry --server $NAME -k) or pass a different name as the fifth argument" >&2
    exit 1
fi

\rm -rf temp && mkdir -p temp
\rm -f nohup.out

XRETRACTOR_PID=
XQRY_PID=
GNUPLOT_PID=
PLOT_DIR=
cleanup()
{
    # Sprzatamy tylko wlasne dzieci. Nazwa instancji mogla juz zostac przejeta
    # przez nowy serwer, wiec koncowe `xqry --server "$NAME" -k` jest niebezpieczne.
    trap - EXIT INT TERM
    local pid
    for pid in "$XQRY_PID" "$XRETRACTOR_PID"; do
        if [ -n "$pid" ]; then kill -TERM "$pid" 2>/dev/null || true; fi
    done
    exec 4>&-
    # Po zamknieciu producenta gnuplot dostaje EOF i zamyka takze okno Qt.
    # SIGTERM do gnuplot omija to sprzatanie i zostawia osobny proces gnuplot_qt.
    for pid in "$XQRY_PID" "$GNUPLOT_PID" "$XRETRACTOR_PID"; do
        if [ -n "$pid" ]; then wait "$pid" 2>/dev/null || true; fi
    done
    if [ -n "$PLOT_DIR" ]; then rm -rf "$PLOT_DIR"; fi
    if [ -t 0 ]; then stty sane; fi
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

nohup xretractor $QUERY --name "$NAME" -k -r &
XRETRACTOR_PID=$!

# Czekamy na gotowosc WLASNEJ instancji zamiast na staly `sleep 1`. Petla pilnuje takze, czy
# proces serwera jeszcze zyje: odmowa startu (zajeta nazwa) konczy go od razu, a wtedy nie wolno
# isc dalej — klient poszedlby do cudzej instancji i to ja zabiloby sprzatanie.
READY=
for _ in $(seq 100); do
    if ! kill -0 "$XRETRACTOR_PID" 2>/dev/null ; then break ; fi
    if xqry --server "$NAME" -l > /dev/null 2>&1 ; then READY=1 ; break ; fi
    sleep 0.1
done
if [ -z "$READY" ]; then
    echo "xplot: instance '$NAME' failed to start; see nohup.out and the xretractor log" >&2
    exit 1
fi

if [ -z "$DISPLAY" ]
then
export DISPLAY=:0
fi

# Bez --warmup xqry rysuje od pierwszego rekordu i okno pojawia sie od razu — tak dziala
# wiekszosc celow, bo ich strumienie nie maja czego odcinac. Z --warmup xqry odrzuca podana
# liczbe rekordow i czeka na pelny kadr (patrz Formatter::gnuplot_warmup_), wiec przez ten
# czas gnuplot nie dostaje zadnego `plot` i nie tworzy okna. Bez ostrzezenia wyglada to na
# zawieszenie, dlatego mowimy o tym tylko wtedy, gdy rozbieg jest faktycznie wlaczony.
case "$XQRY_EXTRA_FLAGS" in
  *--warmup*) echo "xplot: skipping the stream warm-up period; the plot window will appear shortly..." >&2 ;;
esac
# FIFO pozwala zachowac PID obu stron potoku jako bezposrednich dzieci skryptu.
# Zwykly potok czekal na odrysowanie wszystkich zaleglych probek po smierci serwera.
PLOT_DIR=$(mktemp -d "${TMPDIR:-/tmp}/xplot.XXXXXXXX") || exit 1
mkfifo "$PLOT_DIR/data" || exit 1
exec 3<&0
# Uchwyt otwarty w obie strony pozwala wystartowac czytnik takze wtedy, gdy
# sygnal przerwie skrypt przed startem producenta. Sprzatanie zamyka ten uchwyt.
exec 4<> "$PLOT_DIR/data"
gnuplot < "$PLOT_DIR/data" 3<&- 4>&- &
GNUPLOT_PID=$!
{ printf 'bind "Close" "exit gnuplot"\n'; exec xqry --server "$NAME" -s "$STREAM" -p "$SIZE" $XQRY_EXTRA_FLAGS; } \
    <&3 3<&- > "$PLOT_DIR/data" 4>&- &
XQRY_PID=$!
exec 3<&- 4>&-

# Koniec dowolnego dziecka zamyka caly podglad, takze przy zapchanym potoku.
# Sprawdzamy PID-y, bo `wait -n` moze pominac dziecko zakonczone przed jego wywolaniem.
while kill -0 "$XRETRACTOR_PID" 2>/dev/null && kill -0 "$XQRY_PID" 2>/dev/null && kill -0 "$GNUPLOT_PID" 2>/dev/null; do
    sleep 0.1
done
