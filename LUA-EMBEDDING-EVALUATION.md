# Lua as SFML's embedded scripting layer - evaluation

Evaluates `ArqaTools/LuaTools.{h,cpp}` (the plugin's existing Lua 5.4
embedding) as the basis for SFML's ADR §2/§4 "AI's response is a Lua script,
embedded in the C++ engine" design. Produces `lua-engine/SfmlLuaEngine.{h,cpp}`
- a host-agnostic extraction, compiled and run standalone (no ObjectARX, no
AutoCAD, no Windows) to verify it actually works outside ArqaTools before
recommending it as a base for SFML.

## Correction to `ARCHITECTURE-EVALUATION.md`

That earlier doc found no AI→Lua/ACML connection in `AITools.cpp` and
concluded the ADR's "AI mediates... executable form" framing had no working
precedent in this codebase. That conclusion is only half right: **the
precedent exists, it's just decoupled from ACML.** `LuaTools::aiLuaCommand`
(`ATAILUA`) is a working, shipped implementation of exactly the SFML §2
pattern - natural-language intent in, AI-generated Lua out, run directly,
`{ok, output, error}` back - via `AITools::SendToGitHubCopilotWithHistory`.
`AGENTS.md` even states the decoupling explicitly: *"Fully decoupled from
ACML - no cross-references either direction."* So the ADR's claim is
accurate about the *AI→script* half of the pipeline (that pattern is proven
and shipping) and wrong about it being ACML's pattern specifically (it isn't
wired to ACML at all, and doesn't need to be to work). For SFML, treat
`ATAILUA` - not ACML - as the actual reference implementation for the
generation-flow step.

## What's directly reusable (carried into `SfmlLuaEngine`)

| LuaTools mechanism | Why it transfers as-is |
|---|---|
| Fresh `lua_State` per call, no shared/static state | Trivially thread-safe under concurrent load - matches the ADR §4 broker model (multiple requests in flight) with zero locking needed, because there's nothing shared to lock. Confirmed: `LuaTools.cpp`'s own comment notes this project has no background threads either, but the *design*, not just the current single-threaded environment, is what makes it safe. |
| Restricted stdlib (`base`/`table`/`string`/`math` only) | This *is* the runtime half of the ADR §3 principle "give the tools to AI, but control the output" - already solved, not something SFML needs to reinvent. No filesystem/process access reachable from generated code regardless of what the AI writes. |
| `print()` override capturing into a run-scoped buffer | Directly reusable pattern for surfacing script-side diagnostics back to whatever logs SFML's generation runs. |
| C-closure + upvalue-per-run-context idiom (`registerAtTable`) | The clean way to expose a mutable per-run API without global state; generalized in `SfmlLuaEngine::registerApiTable` to take any table name/function list instead of hard-coding `at`. |
| `luaL_loadstring` + `lua_pcall` real error capture | Same honest ok/output/error contract LuaTools already prefers over `AITools::ExecuteLispCode`'s coarse `acedInvoke` check - keep it, don't regress to a weaker signal. |

## Gaps - what LuaTools does NOT solve for SFML

1. **No structured data back to C++.** LuaTools' `at.*` functions return
   AutoCAD entity *handle strings* - because the result the caller cares
   about (an entity) already lives in the document once drawn; there's
   nothing else to marshal back. SFML likely needs the reverse: a script
   computes values (say, resolved member forces) that C++ then feeds into
   verification (Phase 2 of `ROADMAP.md`) *before* anything is committed to
   a document. `RunResult` today only carries free-text `output` plus
   individual return values from whatever functions were called - there is
   no generic "give me the whole resulting table" channel. **This is new
   work**, not something to port: e.g. a host function like
   `sfml.submitResult(table)` that walks the Lua table via `lua_next` and
   fills a C++ struct, or restrict generated scripts to *only* call defined
   API functions (already true here) and treat each function's C++-side
   effect as the "structured result," skipping table marshalling
   entirely - the demo's `api_defineMember` does exactly this and is
   probably the right default: don't add generic marshalling until a
   concrete case proves the function-call model insufficient.

2. **One-shot execution, no persistent state.** Each `runScript()" is a
   fresh interpreter; nothing survives between calls except whatever the
   host wrote into its own `hostData` model. Fine if SFML's generation is
   "one script produces one candidate, verified whole" (matches ADR §5's
   Generative-Design-style candidate pattern). Would need rework if SFML
   ever wants a script to be re-entered across multiple C++ round-trips
   (e.g. an interactive session) - not indicated anywhere in the ADR, so
   not solving for it now; noted so it isn't silently assumed away later.

3. **Lua's sandbox controls *capability*, not *correctness*.** `pcall`
   catches crashes and type errors (see `demo_main.cpp` case 3 - bad
   argument type surfaces cleanly). It says nothing about whether a
   structurally valid-looking script produced a structurally *sound*
   result (span within limit, load within capacity). That check is
   `ROADMAP.md` Phase 2's verification layer, which must run on the
   *result* of a script, in C++, against the fixed standards library -
   entirely separate from and after the Lua run. Don't let "the script ran
   without error" be mistaken for "the output is verified"; `RunResult.ok`
   only ever means the former.

4. **Threading model needs confirming for the real broker, not just
   demoed.** The demo proves single-call thread-safety by construction
   (no shared globals). If SFML's WebSocket broker (ADR §4) hands off
   requests to a thread pool, confirm no two threads ever touch the *same*
   `lua_State` concurrently - true here by construction since each
   `runScript()` call owns its own state start-to-finish, but worth stating
   as an explicit constraint in the broker's design, not an implicit one.

## Recommendation

Use `SfmlLuaEngine` (or a close descendant of it) as SFML's embedded
scripting layer. It inherits LuaTools' proven sandboxing and error-handling
design, drops the ObjectARX coupling (verified by compiling and running it
without ObjectARX present at all), and its API-table mechanism is exactly
the seam SFML needs to expose Member/Connection operations to AI-generated
Lua once Phase 1's grammar exists. Do not treat it as solving verification -
that stays a separate, C++-side, post-execution phase per `ROADMAP.md`.

## Artifacts

- `lua-engine/SfmlLuaEngine.h` / `.cpp` - the extracted engine.
- `lua-engine/demo_main.cpp` - proof it runs standalone; three cases
  (happy path, sandbox violation, runtime error), all verified passing.
- `lua-engine/build.sh` - compiles with system Lua 5.4 and runs the demo.
