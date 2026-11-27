#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# NodeDispatch - demo de la CLI (S3.11)
#
# Ejecuta el guion scripts/cli_demo.txt sobre nodedispatch_cli y deja un
# transcript reproducible. No modifica codigo: solo compila el target si falta
# y ejecuta.
#
# Uso:
#   bash scripts/demo.sh              # transcript a stdout
#   bash scripts/demo.sh > demo.txt   # transcript a un archivo
# -----------------------------------------------------------------------------

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT" || exit 1

CMAKE="${CMAKE:-cmake}"
BUILD_DIR="${BUILD_DIR:-build}"
CLI="$BUILD_DIR/nodedispatch_cli"
SCRIPT="scripts/cli_demo.txt"

if [ ! -x "$CLI" ]; then
    echo "Compilando nodedispatch_cli..." >&2
    "$CMAKE" -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug >/dev/null || exit 1
    "$CMAKE" --build "$BUILD_DIR" --target nodedispatch_cli --parallel 4 >/dev/null || exit 1
fi

if [ ! -f "$SCRIPT" ]; then
    echo "Falta el guion $SCRIPT" >&2
    exit 1
fi

echo "=== NodeDispatch - demo de la CLI (S3.11) ==="
echo "guion: $SCRIPT"
echo

"$CLI" --script "$SCRIPT"
status=$?

echo
if [ "$status" -eq 0 ]; then
    echo "=== fin de la demo: OK ==="
else
    echo "=== fin de la demo: FALLO (exit $status) ==="
fi

exit "$status"
