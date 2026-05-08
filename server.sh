#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT="${1:-5050}"
SERVER_BIN="$SCRIPT_DIR/server"
LIBS=""

case "$(uname -s 2>/dev/null || echo unknown)" in
  MINGW*|MSYS*|CYGWIN*)
    SERVER_BIN="$SCRIPT_DIR/server.exe"
    LIBS="-lws2_32"
    ;;
esac

if [[ ! -x "$SERVER_BIN" || "$SCRIPT_DIR/server.c" -nt "$SERVER_BIN" ]]; then
  gcc -Wall -Wextra -std=c11 -o "$SERVER_BIN" "$SCRIPT_DIR/server.c" $LIBS
fi

exec "$SERVER_BIN" "$PORT"
