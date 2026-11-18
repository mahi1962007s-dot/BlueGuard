#!/bin/sh
# Builds the C engine, installs deps if needed and starts the server (Linux / macOS)
cd "$(dirname "$0")/.."
(cd c && make) || exit 1
[ -d node_modules ] || npm install
npm start
