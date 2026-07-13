# https://hub.docker.com/_/debian
# Runtime instaluje xretractor/xqry z opublikowanego pakietu .deb i uruchamia
# usługę xretractor przez systemd (PID 1) — tak jak na realnym systemie.

FROM debian:trixie-slim

# Wersja bez wartości domyślnej — jedyne źródło prawdy to plik VERSION w korzeniu
# repo. Przekazywana przy buildzie: --build-arg RDB_VERSION="$(cat ../../VERSION)".
ARG RDB_VERSION

LABEL com.retractordb.version="${RDB_VERSION}"
LABEL vendor1="Michal Widera"
LABEL com.retractordb.release-date="2026-06-21"
LABEL com.retractordb.description="RetractorDB production runtime"
LABEL com.retractordb.url="https://retractordb.com"

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# Użytkownik usługi tworzony przed instalacją pakietu: postinst .deb wykrywa
# istniejącego 'retractor' i pomija własne useradd (zachowujemy home /home/retractor,
# potrzebny jako WorkingDirectory usługi dla STORAGE 'temp').
RUN useradd -m -s /bin/bash retractor

# Pakiet retractordb (.deb) z GitHub Releases. apt-get install rozwiązuje zależności;
# binaria lądują w /usr/bin (prefiks /usr pakietu), jednostka w /usr/lib/systemd/system.
# systemd + systemd-sysv: init kontenera (PID 1). libstdc++6 to jedyna zależność
# runtime binariów (antlr linkowany statycznie). ca-certificates do pobrania .deb.
ADD https://github.com/michalwidera/retractordb/releases/download/v${RDB_VERSION}/retractordb-${RDB_VERSION}-Linux.deb /tmp/retractordb.deb

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    libstdc++6 \
    systemd \
    systemd-sysv \
    /tmp/retractordb.deb \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/retractordb.deb

# Instalacja usługi: włącz jednostkę (symlink w multi-user.target.wants — wykonywane
# offline, bez działającego systemd). Na boocie kontenera systemd ją wystartuje.
RUN systemctl enable xretractor.service

# Drop-in: usługa pisze magazyn 'temp' względem cwd; bez WorkingDirectory systemd
# uruchamia ExecStart z '/', gdzie retractor nie ma prawa zapisu → crash-loop.
RUN mkdir -p /etc/systemd/system/xretractor.service.d
RUN <<'EOF' cat > /etc/systemd/system/xretractor.service.d/override.conf
[Service]
WorkingDirectory=/home/retractor
EOF

# Magazyn danych usługi (STORAGE 'temp' z pliku zapytań).
RUN mkdir -p /home/retractor/temp && chown retractor:retractor /home/retractor/temp

# Plik zapytań usługi: ExecStart czyta /etc/retractor/startup.rql (postinst tworzy go
# pustym = tryb idle). Podmieniamy na przykładowy workload kontenera.
COPY --chown=retractor:retractor docker/supervisor/startup.rql /etc/retractor/startup.rql

# Konfiguracja usługi (czytana automatycznie z /etc/retractor/retractor.toml).
# lock_dir: serwis liczy ścieżkę lock-a z argv[0] → '/usr/bin/xretractor_service.lock',
# gdzie User=retractor nie ma prawa zapisu (crash-loop). Przenosimy lock do home usługi.
COPY <<'EOF' /etc/retractor/retractor.toml
[paths]
lock_dir = "/home/retractor"
EOF

ENV RDB_XRETRACTOR=/usr/bin/xretractor
ENV RDB_XQRY=/usr/bin/xqry

# Czysty shutdown systemd na 'docker stop'.
STOPSIGNAL SIGRTMIN+3

CMD ["/sbin/init"]

# How to build and push (run from docker/prod/):
#   docker build --build-arg RDB_VERSION="$(cat ../../VERSION)" -f Dockerfile -t retractordb:latest ../..
#   docker tag retractordb:latest micwide/retractordb:latest
#   docker push micwide/retractordb:latest

# How to run (systemd jako PID 1 wymaga dostępu do cgroup):
#   docker run -d --name retractordb \
#     --tmpfs /run --tmpfs /tmp \
#     -v /sys/fs/cgroup:/sys/fs/cgroup:rw \
#     --cgroupns=host \
#     retractordb:latest
#
# Logowanie do kontenera (shell):
#   docker exec -it retractordb bash              # jako root
#   docker exec -it -u retractor retractordb bash # jako użytkownik usługi
#
# Status / logi usługi w kontenerze:
#   docker exec retractordb systemctl status xretractor
#   docker exec retractordb journalctl -u xretractor -f
#
# Własny plik zapytań (zamiast przykładowego workloadu):
#   docker run -d \
#     -v /path/to/queries.rql:/etc/retractor/startup.rql:ro \
#     ... retractordb:latest
