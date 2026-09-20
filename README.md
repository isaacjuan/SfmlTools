# SFML Tools — Planning & Evaluation

Sibling project to [ArqaTools](../ArqaTools) (which hosts the ACML and Lua
implementations). Mostly planning/evaluation — no SFML grammar exists yet,
per the source ADR's own status line ("draft, pipeline architecture only") —
plus one extracted, verified-working piece of scaffolding (`lua-engine/`)
for the one part of the ADR that's implementation-ready regardless of
grammar: the embedded scripting layer.

## Contents

- [`ARCHITECTURE-EVALUATION.md`](ARCHITECTURE-EVALUATION.md) — how SFML's ADR compares
  against what ACML *actually* is in ArqaTools (not what the ADR assumes about it).
  Several assumed parallels don't hold up against the source; this doc lists them.
- [`LUA-EMBEDDING-EVALUATION.md`](LUA-EMBEDDING-EVALUATION.md) — evaluates
  `ArqaTools/LuaTools.cpp` as the basis for SFML's embedded-Lua step, corrects
  a claim from the ACML evaluation above, and points at `lua-engine/`.
- [`lua-engine/`](lua-engine/) — `SfmlLuaEngine.{h,cpp}`: LuaTools' sandboxed
  embedding with all ObjectARX/AutoCAD coupling removed, compiled and run
  standalone as proof it's portable to SFML's C++ engine.
- [`ROADMAP.md`](ROADMAP.md) — phased plan for resolving the ADR's open questions,
  in dependency order, reusing patterns already proven in ACML's implementation.

## Source material

- ADR: `~/Descargas/SFML-architecture-decision-record.md`
- Reference implementation: `../ArqaTools/Acml{Ast,Lexer,Parser,Semantic,Generator,Tools}.{h,cpp}`,
  `../ArqaTools/LuaTools.{h,cpp}`
