# SfmlLuaEngine - extracted, host-agnostic embedding

`SfmlLuaEngine.{h,cpp}` is `ArqaTools/LuaTools.cpp`'s `runLuaScript` with every
ObjectARX/AutoCAD reference removed and the hard-coded `at` table replaced by
a caller-supplied table name + function list. Verified to compile and run
standalone on Linux with system Lua 5.4 - no Windows, no ObjectARX, no
AutoCAD anywhere in the link. That portability is the entire point: this is
meant to drop into SFML's C++ engine (ADR §2/§4's "embedded in the C++
engine" candidate), which also has no ObjectARX dependency.

## Build & run

```
./build.sh
```

Requires Lua 5.4 dev headers (`liblua5.4-dev` on Debian/Ubuntu, `lua` on
Arch). Runs `demo_main.cpp`'s three cases: a happy-path host-API round trip,
a sandboxing check (`os.execute` must fail - `os` isn't loaded), and a
runtime-error check (bad argument type must surface in `RunResult::error`
without crashing the process).

## Using it for SFML

`demo_main.cpp` is the template: define your own host model struct (SFML's
actual Member/Connection store, not `DemoModel`), write `lua_CFunction`s that
pull it via `SfmlLua::HostData(L)`, list them in an `ApiFunction` vector, and
call `Engine::runScript(code, "sfml", fns, &model)`. See
`../LUA-EMBEDDING-EVALUATION.md` for what this does and doesn't solve for
SFML specifically.
