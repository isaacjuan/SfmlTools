#!/usr/bin/env bash
# Builds and runs the SfmlLuaEngine standalone demo. Requires Lua 5.4 dev
# headers (Debian/Ubuntu: liblua5.4-dev; Arch: lua). No ObjectARX, no
# AutoCAD, no Windows — this is the point: proves the engine is portable.
set -euo pipefail
cd "$(dirname "$0")"
g++ -std=c++17 -Wall -Wextra $(pkg-config --cflags lua5.4) \
    -o demo SfmlLuaEngine.cpp demo_main.cpp $(pkg-config --libs lua5.4)
./demo
