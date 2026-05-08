#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HOST="${1:-127.0.0.1}"
PORT="${2:-5050}"
CLIENT_BIN="$SCRIPT_DIR/client"
LIBS=""

case "$(uname -s 2>/dev/null || echo unknown)" in
  MINGW*|MSYS*|CYGWIN*)
    CLIENT_BIN="$SCRIPT_DIR/client.exe"
    LIBS="-lws2_32"
    ;;
esac

if [[ ! -x "$CLIENT_BIN" || "$SCRIPT_DIR/client.c" -nt "$CLIENT_BIN" ]]; then
  gcc -Wall -Wextra -std=c11 -o "$CLIENT_BIN" "$SCRIPT_DIR/client.c" $LIBS
fi

exec "$CLIENT_BIN" "$HOST" "$PORT"
