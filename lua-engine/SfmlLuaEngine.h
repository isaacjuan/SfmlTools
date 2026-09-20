// SfmlLuaEngine.h — host-agnostic sandboxed Lua embedding for SFML
//
// Extracted/ported from ArqaTools/LuaTools.cpp's runLuaScript, with every
// ObjectARX/AutoCAD dependency removed. Where LuaTools hard-codes a single
// `at` table of CAD-drawing functions, this engine takes the table name and
// function list as parameters — so SFML's C++ engine (or anything else) can
// expose its own domain API (e.g. a `sfml` table with defineMember/
// defineConnection/checkConstraint) without touching this file.
//
// Carried over unchanged from LuaTools:
//   - fresh lua_State per call, no shared/static state → trivially safe to
//     call concurrently from multiple threads as long as each call gets its
//     own Engine::runScript() invocation (matches the broker/WebSocket
//     concurrency model in the SFML ADR §4 — no locking needed here because
//     there is nothing shared to lock).
//   - restricted stdlib: base/table/string/math only — no io/os/package/
//     debug, so a generated script cannot touch the filesystem or shell out.
//   - print() is overridden to capture into the run's output buffer instead
//     of writing to a nonexistent console (this project has no console
//     either — same reasoning LuaTools used for a plugin process).
//
// Deliberately NOT carried over (see LUA-EMBEDDING-EVALUATION.md):
//   - No structured value marshalling back to C++. Host functions can only
//     return values Lua understands (numbers/strings/bools) and the run as
//     a whole only returns captured print() text + ok/error. If SFML needs
//     computed structured results back (not just text), that is an
//     additional layer to design on top of this, not something ACML/LuaTools
//     needed because AutoCAD entities were the result, addressed via handle
//     strings.
//   - No persistent lua_State across calls — each runScript() is one-shot.
//     Fine for stateless generation; revisit if SFML needs a script to hold
//     state across multiple C++ round-trips.

#pragma once

#include <string>
#include <vector>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

namespace SfmlLua {

// Result of one script run — always a real success/failure signal (never a
// coarse return-code check), same contract as LuaTools::LuaRunResult.
struct RunResult
{
    bool        ok = false;
    std::string output;   // everything written via print() during the run
    std::string error;    // Lua compile/runtime error message when ok == false
};

// One function to expose on the host's API table. `fn` is a plain
// lua_CFunction — inside it, use HostData(L) / AppendOutput(L, text) below
// instead of touching lua_upvalueindex directly.
struct ApiFunction
{
    const char*    name;
    lua_CFunction  fn;
};

// Call from inside an ApiFunction implementation to get back the opaque
// pointer passed as `hostData` to runScript() — e.g. a pointer to whatever
// in-memory model the host function should read/mutate.
void* HostData(lua_State* L);

// Call from inside an ApiFunction implementation to append text to the
// run's captured output buffer — the same buffer the overridden print()
// writes to, so host-function output and script print() output interleave
// in one ordered log.
void AppendOutput(lua_State* L, const std::string& text);

class Engine
{
public:
    // Runs `code` synchronously in a fresh, sandboxed lua_State.
    //   tableName — global table name the API functions are exposed under
    //               (e.g. "sfml"); pass nullptr/"" to skip exposing a table
    //               at all (script gets base/table/string/math only).
    //   functions — host functions to register on that table.
    //   hostData  — opaque pointer forwarded to every call via HostData(L).
    RunResult runScript(const std::string&            code,
                        const char*                    tableName,
                        const std::vector<ApiFunction>& functions,
                        void*                           hostData);
};

} // namespace SfmlLua
