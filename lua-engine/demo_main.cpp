// demo_main.cpp - proves SfmlLuaEngine compiles and runs standalone, with
// zero ObjectARX/AutoCAD in the link, exposing a toy domain API instead of
// LuaTools' CAD-drawing `at` table. Not a unit test framework - a runnable
// sanity check with three cases:
//   1. happy path   - script calls the host API, host mutates its own model
//   2. sandboxing   - script tries os.execute(), must fail (stdlib absent)
//   3. runtime error - script errors mid-run, must surface via result.error
//
// Build/run: see README.md in this directory.

#include "SfmlLuaEngine.h"
#include <cstdio>
#include <vector>
#include <string>

// ── Toy host model - stands in for whatever SFML's C++ engine actually
// tracks (Members/Connections). Lives entirely outside the Lua engine file;
// the engine never needs to know this struct exists.
struct DemoModel
{
    struct Member { std::string name; double lengthMm; };
    std::vector<Member> members;
};

// ── Host API functions exposed to Lua as the `sfml` table.
// Each retrieves the DemoModel* via SfmlLua::HostData(L) - the engine
// forwards whatever opaque pointer runScript() was given.

static int api_defineMember(lua_State* L)
{
    const char* name   = luaL_checkstring(L, 1);
    double      length = luaL_checknumber(L, 2);

    auto* model = static_cast<DemoModel*>(SfmlLua::HostData(L));
    model->members.push_back({name, length});

    lua_pushinteger(L, static_cast<lua_Integer>(model->members.size() - 1));
    return 1;  // returns the new member's index
}

static int api_memberCount(lua_State* L)
{
    auto* model = static_cast<DemoModel*>(SfmlLua::HostData(L));
    lua_pushinteger(L, static_cast<lua_Integer>(model->members.size()));
    return 1;
}

static void runCase(const char* label, const std::string& code)
{
    DemoModel model;

    std::vector<SfmlLua::ApiFunction> fns = {
        { "defineMember", api_defineMember },
        { "memberCount",  api_memberCount  },
    };

    SfmlLua::Engine engine;
    SfmlLua::RunResult r = engine.runScript(code, "sfml", fns, &model);

    std::printf("=== %s ===\n", label);
    std::printf("ok: %s\n", r.ok ? "true" : "false");
    if (!r.output.empty())
        std::printf("output:\n%s", r.output.c_str());
    if (!r.error.empty())
        std::printf("error: %s\n", r.error.c_str());
    std::printf("host-side model after run: %zu member(s)\n", model.members.size());
    for (const auto& m : model.members)
        std::printf("  - %s: %.1f mm\n", m.name.c_str(), m.lengthMm);
    std::printf("\n");
}

int main()
{
    runCase("1. happy path",
        "local i1 = sfml.defineMember('beam_a', 3200)\n"
        "local i2 = sfml.defineMember('beam_b', 4100)\n"
        "print('defined members at indices', i1, i2)\n"
        "print('total members:', sfml.memberCount())\n");

    runCase("2. sandboxing (expect failure - os is not available)",
        "os.execute('echo should not run')\n");

    runCase("3. runtime error (expect failure - bad argument type)",
        "sfml.defineMember('beam_c', 'not-a-number')\n");

    return 0;
}
