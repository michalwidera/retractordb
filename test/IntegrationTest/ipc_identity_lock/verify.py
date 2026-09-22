"""Tozsamosc IPC: zajeta blokada zatrzymuje start, zanim cokolwiek zostanie skasowane.

1. Blokade trzyma obcy proces: start odmawia i nie dotyka magazynu.
2. Dwa prawdziwe serwery o tej samej nazwie w roznych TMPDIR i przestrzeniach magistrali. Na
   Linuksie nazwy nie sa skracane, wiec to jedyna droga do wspolnej tozsamosci IPC; ani blokada
   instancji (inny TMPDIR), ani magistrala (inny segment) tej kolizji nie widza. Drugi serwer
   odmawia startu, pierwszy nadal odpowiada, a jego obiekty IPC zostaja nietkniete.

Test oblewal rzadko i tylko w rownoleglym przebiegu calego zestawu, a z samego komunikatu
asercji nie dalo sie odczytac, KTO skasowal obiekty ani czy serwer wtedy zyl. Dlatego kazda
kontrola idzie przez check(), ktore przy niepowodzeniu zrzuca na stdout komplet stanu maszyny
(ctest zapisuje stdout w LastTest.log): pelne /dev/shm z i-wezlami, blokady z /tmp razem
z wlascicielami odczytanymi z /proc/locks, tablice procesow, log serwera i os czasu zebrana
przez watek obserwatora. Bez tego kazde zlapane oblanie jest tak samo nieme jak pierwsze.
"""
import fcntl
import os
from pathlib import Path
import re
import subprocess
import sys
import threading
import time
import traceback

xretractor, xqry = sys.argv[1:3]
name = os.environ["RDB_NAMESPACE"]
QUERY = "DECLARE a BYTE STREAM src, 0.1 FILE '/dev/urandom'\nSELECT src[0] STREAM dst FROM src\n"
PAYLOAD = b"protected payload"

SHM_DIR = Path("/dev/shm")
LOCK_DIR = Path("/tmp")
# Nazwy nalezace do badanej tozsamosci: trzy obiekty globalne koncza sie kropka i nazwa
# instancji, kolejki odpowiedzi klientow maja ja w srodku (brcdbr.<nazwa>.<klient>).
OWN_OBJECT = re.compile(r"\." + re.escape(name) + "$")
OWN_RELATED = re.compile(r"(^|\.)" + re.escape(name) + r"(\.|$)")

# Stan biezacy dla zrzutu diagnostycznego - uzupelniany w miare postepu scenariusza.
CONTEXT = {"phase": "init", "server": None, "server_dir": None, "last_run": None}
START = time.monotonic()


def stamp():
    return f"{time.monotonic() - START:8.3f}s"


# --- odczyt stanu maszyny ---------------------------------------------------------------


def shm_entries():
    """Mapa nazwa -> i-wezel dla calego /dev/shm; None, gdy platforma go nie ma (Darwin)."""
    if not SHM_DIR.is_dir():
        return None
    retVal = {}
    for entry in os.scandir(SHM_DIR):
        try:
            retVal[entry.name] = entry.stat().st_ino
        except OSError:
            pass  # znikl miedzy listowaniem a stat() - dla osi czasu to to samo, co brak
    return retVal


def flock_owners():
    """i-wezel -> lista PID-ow trzymajacych na nim flock, z /proc/locks."""
    retVal = {}
    try:
        text = Path("/proc/locks").read_text(encoding="ascii", errors="replace")
    except OSError:
        return retVal
    for line in text.splitlines():
        fields = line.split()
        for index, field in enumerate(fields):
            if re.fullmatch(r"[0-9a-f]+:[0-9a-f]+:\d+", field) and index > 0:
                try:
                    pid = int(fields[index - 1])
                except ValueError:
                    break
                retVal.setdefault(int(field.rsplit(":", 1)[1]), []).append(pid)
                break
    return retVal


def lock_files():
    """Blokady maszynowe z /tmp (tozsamosci IPC i obecnosci na magistrali) z wlascicielami."""
    owners = flock_owners()
    retVal = []
    try:
        entries = sorted(os.scandir(LOCK_DIR), key=lambda e: e.name)
    except OSError as error:
        return [f"<nie da sie wylistowac {LOCK_DIR}: {error}>"]
    for entry in entries:
        if not (entry.name.startswith("xretractor_ipc.") or entry.name.startswith("xrdbbus")):
            continue
        try:
            info = entry.stat()
        except OSError as error:
            retVal.append(f"{entry.name}: <stat: {error}>")
            continue
        held = owners.get(info.st_ino, [])
        retVal.append(f"{entry.name}: ino={info.st_ino} mode={info.st_mode & 0o7777:04o} "
                      f"trzymana przez PID {held if held else '<nikogo>'}")
    return retVal


def process_table():
    try:
        out = subprocess.run(["ps", "-eo", "pid,ppid,pgid,etimes,stat,args"],
                             capture_output=True, text=True, timeout=10).stdout
    except (OSError, subprocess.SubprocessError) as error:
        return [f"<ps nieosiagalne: {error}>"]
    keep = re.compile(r"xretractor|xqry|verify\.py|ctest")
    return [line[:300] for line in out.splitlines() if keep.search(line)]


def shm_listing():
    """Pelne /dev/shm z i-wezlami, trybem, rozmiarem i wiekiem - posortowane, wlasne oznaczone."""
    if not SHM_DIR.is_dir():
        return ["<brak /dev/shm>"]
    retVal = []
    now = time.time()
    try:
        entries = sorted(os.scandir(SHM_DIR), key=lambda e: e.name)
    except OSError as error:
        return [f"<nie da sie wylistowac /dev/shm: {error}>"]
    for entry in entries:
        try:
            info = entry.stat()
        except OSError as error:
            retVal.append(f"    {entry.name}: <stat: {error}>")
            continue
        mark = "WLASNE " if OWN_RELATED.search(entry.name) else "       "
        retVal.append(f"    {mark}{entry.name}: ino={info.st_ino} mode={info.st_mode & 0o7777:04o} "
                      f"size={info.st_size} uid={info.st_uid} wiek={now - info.st_mtime:.3f}s")
    return retVal


# --- watek obserwatora ------------------------------------------------------------------
#
# Sama para "przed/po" nie mowi, KIEDY i przy jakim towarzystwie obiekty zniknely. Watek
# probkuje /dev/shm co kilka milisekund i zapisuje kazda zmiane razem z faza scenariusza;
# dla zmian dotyczacych badanej tozsamosci dokleja tablice procesow i wlascicieli blokad,
# bo tylko wtedy warto placic za czytanie /proc.

TIMELINE = []
TIMELINE_LIMIT = 4000
WATCHER_STOP = threading.Event()


def watcher():
    previous = shm_entries() or {}
    while not WATCHER_STOP.is_set():
        current = shm_entries()
        if current is None:
            return
        if current != previous:
            added = {k: v for k, v in current.items() if previous.get(k) != v}
            removed = {k: v for k, v in previous.items() if current.get(k) != v}
            own = any(OWN_RELATED.search(k) for k in list(added) + list(removed))
            if len(TIMELINE) < TIMELINE_LIMIT:
                event = {"t": stamp(), "phase": CONTEXT["phase"], "added": added, "removed": removed, "own": own}
                if own:
                    event["procesy"] = process_table()
                    event["blokady"] = lock_files()
                TIMELINE.append(event)
            previous = current
        WATCHER_STOP.wait(0.002)


def timeline_report(only_own):
    retVal = []
    for event in TIMELINE:
        if only_own and not event["own"]:
            continue
        retVal.append(f"  [{event['t']}] faza={event['phase']} "
                      f"doszlo={event['added']} zniknelo={event['removed']}")
        if not only_own:
            continue  # kontekst procesow i blokad stoi juz przy zdarzeniach wlasnych
        for line in event.get("procesy", []):
            retVal.append(f"        proces: {line}")
        for line in event.get("blokady", []):
            retVal.append(f"        blokada: {line}")
    if not retVal:
        retVal.append("  <brak zdarzen>")
    return retVal


# --- zrzut diagnostyczny ----------------------------------------------------------------


def dump(label, detail):
    server = CONTEXT["server"]
    print("", flush=True)
    print("=" * 100, flush=True)
    print(f"DIAGNOZA: kontrola '{label}' nie przeszla", flush=True)
    print(f"  powod: {detail}", flush=True)
    print("=" * 100, flush=True)
    print(f"czas od startu testu: {stamp()}   faza: {CONTEXT['phase']}", flush=True)
    print(f"nazwa instancji (RDB_NAMESPACE): {name}", flush=True)
    print(f"TMPDIR: {os.environ.get('TMPDIR', '<brak>')}", flush=True)
    print(f"katalog roboczy: {os.getcwd()}", flush=True)
    print(f"PID testu: {os.getpid()}", flush=True)

    print("\n--- serwer pierwszy ---", flush=True)
    if server is None:
        print("  <jeszcze nie wystartowal>", flush=True)
    else:
        code = server.poll()
        print(f"  pid={server.pid} stan={'zyje' if code is None else f'zakonczony kodem {code}'}", flush=True)
    log = Path(CONTEXT["server_dir"] or ".") / "server.log"
    print(f"  log {log}:", flush=True)
    try:
        text = log.read_text(encoding="utf-8", errors="replace")
        print("".join(f"    | {line}\n" for line in text.splitlines()) or "    <pusty>", flush=True)
    except OSError as error:
        print(f"    <nie da sie odczytac: {error}>", flush=True)

    last = CONTEXT["last_run"]
    print("\n--- ostatni uruchomiony podproces ---", flush=True)
    if last is None:
        print("  <zaden>", flush=True)
    else:
        print(f"  polecenie: {last['cmd']}", flush=True)
        print(f"  katalog: {last['cwd']}  kod wyjscia: {last['rc']}", flush=True)
        print(f"  stdout: {last['stdout']!r}", flush=True)
        print(f"  stderr: {last['stderr']!r}", flush=True)

    tmpdir = Path(os.environ.get("TMPDIR", "/tmp"))
    engine = tmpdir / "xretractor.log"
    print(f"\n--- ogon logu silnika {engine} (slot przestrzeni nazw dziela inne katalogi) ---", flush=True)
    try:
        lines = engine.read_text(encoding="utf-8", errors="replace").splitlines()
        for line in lines[-120:]:
            print(f"    | {line}", flush=True)
    except OSError as error:
        print(f"    <nie da sie odczytac: {error}>", flush=True)

    print("\n--- /dev/shm w chwili awarii ---", flush=True)
    for line in shm_listing():
        print(line, flush=True)

    print("\n--- blokady maszynowe w /tmp (z wlascicielami z /proc/locks) ---", flush=True)
    for line in lock_files():
        print(f"    {line}", flush=True)

    print("\n--- procesy ---", flush=True)
    for line in process_table():
        print(f"    {line}", flush=True)

    print("\n--- os czasu /dev/shm: zmiany dotyczace tej tozsamosci ---", flush=True)
    for line in timeline_report(only_own=True):
        print(line, flush=True)

    print("\n--- os czasu /dev/shm: wszystkie zmiany (kontekst sasiadow) ---", flush=True)
    for line in timeline_report(only_own=False):
        print(line, flush=True)
    print("=" * 100, flush=True)


class CheckFailed(Exception):
    """Kontrola nie przeszla; stan zostal juz zrzucony w chwili awarii."""


# Zrzut powstaje NATYCHMIAST po nieudanej kontroli, a nie w obsludze wyjatku na koncu:
# blok `finally` zabija pierwszy serwer i kasuje jego obiekty IPC, wiec zrzut zrobiony
# po nim opisywalby juz posprzatana maszyne, czyli dokladnie nie ten stan, o ktory chodzi.
DUMPED = False


def dump_once(label, detail):
    global DUMPED
    if DUMPED:
        return
    DUMPED = True
    dump(label, detail)


def check(label, condition, detail):
    if not condition:
        dump_once(label, detail)
        raise CheckFailed(f"{label}: {detail}")


# --- scenariusz -------------------------------------------------------------------------


def identity_lock(instance):
    return LOCK_DIR / ("xretractor_ipc.RetractorQueryQueue." + instance + ".lock")


def still_linked(handle, path):
    """Czy i-wezel spod blokady nadal lezy pod ta sciezka (lockfile::stillLinked)."""
    try:
        held = os.fstat(handle.fileno())
        named = os.stat(path)
    except OSError:
        return False
    return (held.st_dev, held.st_ino) == (named.st_dev, named.st_ino)


def hold_identity_lock(path):
    """Zajmuje blokade tozsamosci tym samym protokolem, ktorym robi to silnik.

    open() i flock() to DWA wywolania jadra, a miedzy nimi kazdy konczacy sie xretractor moze
    ten plik zajac i odlaczyc: sweepAbandonedResources przeglada /tmp GLOBALNIE i robi to na
    kazdym wyjsciu serwera, a nie tylko pod `--cleanup`. Blokada na odlaczonym i-wezle nie
    chroni juz niczego - kolejny uczestnik zaklada pod ta sciezka NOWY plik i STARTUJE zamiast
    odmowic, czyli test oblewa bez winy silnika. Zmierzone odpowiednikiem zamiatacza w petli:
    2636 z 20000 zajec (13,2%) konczylo sie na odlaczonym i-wezle.
    """
    for _ in range(100):  # tyle samo ponowien co kMaxRelinks w lockFile.cpp
        handle = path.open("a+")
        try:
            fcntl.flock(handle, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except OSError:
            handle.close()
            time.sleep(0.001)
            continue
        if still_linked(handle, path):
            return handle
        handle.close()
    raise RuntimeError(f"nie udalo sie zajac blokady tozsamosci {path} w 100 probach")


def ipc_objects(instance):
    """Obiekty IPC instancji z ich i-wezlami; None, gdy platforma ich nie wystawia (Darwin)."""
    entries = shm_entries()
    if entries is None:
        return None
    return {k: v for k, v in entries.items() if OWN_OBJECT.search(k)}


def prepare(directory):
    directory.mkdir(exist_ok=True)
    (directory / "query.rql").write_text(QUERY, encoding="ascii")
    (directory / "dst").write_bytes(PAYLOAD)
    return directory


def record(cmd, cwd, result):
    CONTEXT["last_run"] = {"cmd": cmd, "cwd": str(cwd), "rc": result.returncode,
                           "stdout": result.stdout, "stderr": result.stderr}
    return result


def assert_refused(step, directory, env):
    cmd = [xretractor, "query.rql", "--name", name, "-r", "-k", "-f", "-m", "1"]
    result = record(cmd, directory, subprocess.run(
        cmd, cwd=directory, env=env, capture_output=True, text=True, timeout=10))
    check(f"{step}:odmowa-kod-wyjscia", result.returncode != 0,
          f"start NIE zostal odrzucony (kod {result.returncode}); stdout+stderr: {result.stdout + result.stderr!r}")
    check(f"{step}:odmowa-komunikat", "cannot acquire IPC identity" in result.stderr,
          f"odmowa przyszla z innego powodu niz tozsamosc IPC; stderr: {result.stderr!r}")
    check(f"{step}:magazyn-nietkniety", (directory / "dst").read_bytes() == PAYLOAD,
          f"odrzucony start ruszyl magazyn: {(directory / 'dst').read_bytes()!r}")


def xqry_run(*args, env, timeout=10):
    cmd = [xqry, "--server", name, *args]
    return record(cmd, os.getcwd(), subprocess.run(cmd, env=env, capture_output=True, text=True, timeout=timeout))


def scenario():
    # 1. Obcy wlasciciel blokady.
    CONTEXT["phase"] = "1:obcy-wlasciciel-blokady"
    holder = prepare(Path("holder"))
    lock = identity_lock(name)
    with hold_identity_lock(lock) as guard:
        assert_refused("krok1", holder, dict(os.environ))
        if still_linked(guard, lock):
            lock.unlink()  # zgodnie z protokolem: kasuje ten, kto trzyma blokade, jeszcze pod nia
    print("PASS occupied IPC identity refuses startup without touching payload", flush=True)

    # 2. Dwa serwery, jedna tozsamosc IPC.
    CONTEXT["phase"] = "2:start-pierwszego-serwera"
    first, second = prepare(Path("first")), prepare(Path("second"))
    CONTEXT["server_dir"] = first
    second_tmp = Path("second_tmp").resolve()
    second_tmp.mkdir(exist_ok=True)
    env_first = dict(os.environ)
    env_second = dict(os.environ, TMPDIR=str(second_tmp), RDB_NAMESPACE=name + "b")
    with (first / "server.log").open("wb") as log:
        server = subprocess.Popen([xretractor, "query.rql", "--name", name, "-r", "-k"], cwd=first, env=env_first,
                                  stdin=subprocess.DEVNULL, stdout=log, stderr=log)
    CONTEXT["server"] = server
    try:
        hello = xqry_run("-w", "--hello", env=env_first, timeout=20)
        check("krok2:pierwszy-wstal", hello.returncode == 0,
              f"pierwszy serwer nie wstal (kod {hello.returncode}); stderr: {hello.stderr!r}")

        CONTEXT["phase"] = "2:migawka-before"
        before = ipc_objects(name)
        check("krok2:obiekty-istnieja", before is None or len(before) >= 3,
              f"pierwszy serwer nie ma kompletu obiektow IPC (oczekiwane >=3): {before}")

        CONTEXT["phase"] = "2:proba-drugiego-serwera"
        assert_refused("krok2", second, env_second)

        CONTEXT["phase"] = "2:kontrola-koncowa"
        check("krok2:pierwszy-zyje", server.poll() is None,
              f"pierwszy serwer zginal przy probie startu drugiego (kod {server.poll()})")
        hello = xqry_run("--hello", env=env_first)
        check("krok2:pierwszy-odpowiada", hello.returncode == 0,
              f"pierwszy serwer przestal odpowiadac (kod {hello.returncode}); stderr: {hello.stderr!r}")
        if before is None:
            print("SKIP IPC object identity check: no /dev/shm on this platform", flush=True)
        else:
            after = ipc_objects(name)
            check("krok2:obiekty-nietkniete", after == before,
                  f"obiekty IPC pierwszego serwera zmienily sie: {before} -> {after}; "
                  f"zniknely {sorted(set(before) - set(after))}, "
                  f"doszly {sorted(set(after) - set(before))}, "
                  f"zmieniony i-wezel {sorted(k for k in set(before) & set(after) if before[k] != after[k])}")
    except BaseException as error:  # noqa: BLE001 - zrzut przed sprzataniem, potem wyjatek leci dalej
        dump_once(type(error).__name__, str(error) or "<bez tresci>")
        raise
    finally:
        CONTEXT["phase"] = "3:sprzatanie"
        xqry_run("--kill", env=env_first)
        try:
            server.wait(timeout=10)
        except subprocess.TimeoutExpired:
            server.kill()
            server.wait()
    print("PASS second server with the same IPC identity refused; first one untouched and answering", flush=True)


observer = threading.Thread(target=watcher, daemon=True)
observer.start()
try:
    scenario()
except CheckFailed:
    sys.exit(1)
except BaseException as error:  # noqa: BLE001 - kazda awaria ma zostawic slad, nie sam traceback
    dump_once(type(error).__name__, str(error) or "<bez tresci>")
    print("\n--- slad wyjatku ---", flush=True)
    traceback.print_exc(file=sys.stdout)
    sys.exit(1)
finally:
    WATCHER_STOP.set()
    observer.join(timeout=2)

# Zielony przebieg tez ma mowic, czy ktokolwiek dotykal tych obiektow: cisza obserwatora jest
# czescia dowodu, a jej brak w logu zmusza do powtorzenia calego polowania.
own_events = [event for event in TIMELINE if event["own"]]
print(f"OBSERWATOR: zdarzen /dev/shm razem {len(TIMELINE)}, dotyczacych tej tozsamosci {len(own_events)}"
      f"{' (limit probek osiagniety)' if len(TIMELINE) >= TIMELINE_LIMIT else ''}", flush=True)
# Sama liczba nie wystarcza: zdarzen wlasnych bywa raz dwa, raz trzy, i z liczby nie widac,
# czy trzecie to podzial tworzenia na dwie probki, czy obcy proces. Os czasu zdarzen wlasnych
# niesie tablice procesow i wlascicieli blokad z tamtej chwili, wiec kazdy ZIELONY przebieg
# dowodzi wprost, kto tych obiektow dotykal - a to jest polowa dowodu przy zdarzeniu rzadkim.
for line in timeline_report(only_own=True):
    print(line, flush=True)
