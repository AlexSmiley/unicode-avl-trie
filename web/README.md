# Web frontend

This directory contains a browser UI for the Unicode AVL Trie. The frontend calls the repository's existing C functions compiled to WebAssembly; it does not reimplement the data structure in JavaScript.

## Requirements

- Emscripten SDK (emcc)
- A local static HTTP server

## Build

From the repository root:

    sh web/build.sh

This generates web/dictionary.js and web/dictionary.wasm. Generated build artifacts are intentionally not committed.

## Run

Serve the web/ directory over HTTP. For example:

    cd web
    python3 -m http.server 8000

Open http://localhost:8000.

## Features

- Search dictionary entries
- Add new entries or update existing translations
- Remove individual entries or clear the dictionary
- UTF-8 input and translations, including Macedonian Cyrillic
- Sample entries loaded at startup

## Notes

- Entries live in WebAssembly memory and are reset when the page is reloaded.
- The C implementation remains the source of truth for UTF-8 validation and dictionary behavior.
- The existing C API stores translations up to 511 UTF-8 bytes.
- The implementation is case-sensitive and does not perform Unicode normalization or locale-specific case folding.
