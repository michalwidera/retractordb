#!/bin/bash
# Atrapa systemctl/launchctl: test nie restartuje prawdziwej jednostki.
set -e
: > "$RDB_RESTART_MARKER"
