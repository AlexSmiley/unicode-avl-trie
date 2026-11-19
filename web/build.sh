#!/usr/bin/env sh
set -eu
ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT_DIR"
if ! command -v emcc >/dev/null 2>&1; then
  echo "Emscripten (emcc) is required. Install and activate the Emscripten SDK first." >&2
  exit 1
fi
emcc web/wasm_bridge.c -O2 -std=c11 -Wall -Wextra \
  -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createDictionaryModule \
  -sEXPORTED_FUNCTIONS='["_wasm_init","_wasm_add","_wasm_search","_wasm_remove","_wasm_clear","_malloc","_free"]' \
  -sEXPORTED_RUNTIME_METHODS='["UTF8ToString"]' \
  -o web/dictionary.js
echo "Build complete. Serve web/ over HTTP (not file://)."
