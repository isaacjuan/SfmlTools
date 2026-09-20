#include "SfmlLuaEngine.h"

namespace SfmlLua {

// ─────────────────────────────────────────────────────────────────────────────
// Per-run context - the direct analog of LuaTools.cpp's anonymous-namespace
// LuaCtx, generalized with an opaque hostData pointer so this file has no
// knowledge of what the host actually does.
// ─────────────────────────────────────────────────────────────────────────────
namespace {

struct RunContext
{
    std::string output;
    void*       hostData = nullptr;
};

RunContext* ctxFrom(lua_State* L)
{
    return static_cast<RunContext*>(lua_touserdata(L, lua_upvalueindex(1)));
}

// Overridden global print(...) - captures into the run's output buffer
// instead of writing to a (nonexistent, in an embedded context) console.
// Uses luaL_tolstring so it matches stock print()'s __tostring-aware
// formatting, same as LuaTools::lua_print_override.
int luaPrintOverride(lua_State* L)
{
    RunContext* ctx = ctxFrom(L);
    int n = lua_gettop(L);
    for (int i = 1; i <= n; ++i)
    {
        size_t len = 0;
        const char* s = luaL_tolstring(L, i, &len);
        ctx->output.append(s, len);
        lua_pop(L, 1);
        if (i < n) ctx->output += "\t";
    }
    ctx->output += "\n";
    return 0;
}

void registerApiTable(lua_State* L, RunContext* ctx, const char* tableName,
                      const std::vector<ApiFunction>& functions)
{
    if (!tableName || !*tableName || functions.empty())
        return;

    lua_newtable(L);                             // <tableName> = {}
    for (const auto& f : functions)
    {
        lua_pushlightuserdata(L, ctx);           // upvalue #1 = RunContext*
        lua_pushcclosure(L, f.fn, 1);
        lua_setfield(L, -2, f.name);              // table[name] = closure
    }
    lua_setglobal(L, tableName);                  // _G[tableName] = table
}

} // namespace

void* HostData(lua_State* L)
{
    return ctxFrom(L)->hostData;
}

void AppendOutput(lua_State* L, const std::string& text)
{
    ctxFrom(L)->output += text;
}

RunResult Engine::runScript(const std::string&              code,
                            const char*                      tableName,
                            const std::vector<ApiFunction>& functions,
                            void*                            hostData)
{
    RunResult result;

    lua_State* L = luaL_newstate();
    if (!L) { result.error = "luaL_newstate failed."; return result; }

    RunContext ctx;
    ctx.hostData = hostData;

    // Restricted stdlib: base/table/string/math only. Deliberately NOT
    // io/os/package/debug, so a generated script cannot touch the
    // filesystem, shell out, or introspect/mutate the running engine.
    luaL_requiref(L, LUA_GNAME,       luaopen_base,   1); lua_pop(L, 1);
    luaL_requiref(L, LUA_TABLIBNAME,  luaopen_table,  1); lua_pop(L, 1);
    luaL_requiref(L, LUA_STRLIBNAME,  luaopen_string, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math,   1); lua_pop(L, 1);

    lua_pushlightuserdata(L, &ctx);
    lua_pushcclosure(L, luaPrintOverride, 1);
    lua_setglobal(L, "print");

    registerApiTable(L, &ctx, tableName, functions);

    int loadStatus = luaL_loadstring(L, code.c_str());
    if (loadStatus != LUA_OK)
    {
        const char* msg = lua_tostring(L, -1);
        result.error = msg ? msg : "Lua load error.";
        lua_close(L);
        return result;
    }

    int callStatus = lua_pcall(L, 0, 0, 0);
    result.output = ctx.output;
    if (callStatus != LUA_OK)
    {
        const char* msg = lua_tostring(L, -1);
        result.error = msg ? msg : "Lua runtime error.";
        result.ok = false;
    }
    else
    {
        result.ok = true;
    }

    lua_close(L);
    return result;
}

} // namespace SfmlLua
