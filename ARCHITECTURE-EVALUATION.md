# SFML vs. ACML — Architecture Evaluation

Evaluates the SFML ADR (`~/Descargas/SFML-architecture-decision-record.md`) against
ACML **as it actually exists** in `../ArqaTools` — not against the ADR's own
description of ACML. Several conceptual references in the ADR (Axiom Zero, the
"six conversations between two Boundaries") could not be located anywhere in the
ArqaTools source or docs; they may live outside this repo. Everything below is
sourced from `AcmlAst.h`, `AcmlLexer.{h,cpp}`, `AcmlParser.{h,cpp}`,
`AcmlSemantic.{h,cpp}`, `AcmlGenerator.{h,cpp}`, `AcmlTools.{h,cpp}`.

## 1. What ACML actually is

A four-stage deterministic compiler, no AI anywhere in the pipeline:

```
.acml text → Lexer → Parser → AST → Semantic analyzer → ResolvedElement[] → Generator → AutoCAD entities
```

- **Lexer/Parser/AST** (`AcmlAst.h`): grammar has `Document`, `Import`,
  `ComponentDef` (parameterized, reusable), `ElementTypeDef` (single-parent
  `inherits`), `Element` (instance), `Assignment`, `ConstraintStmt` /
  `ConstraintExpr`. Expressions carry units natively on literals (`mm`, `cm`,
  `m`, `deg`, `rad`, `%`) and support dotted reference chains (`self`,
  `parent`, `root`, `previous`, `next`, user IDs).
- **Semantic analyzer** (`AcmlSemantic.h/.cpp`): pure C++20, zero ObjectARX
  dependency — testable headless. Builds an ID registry, component/type
  registries, resolves properties with cycle detection + caching, expands
  `Repeater`/`Model` for data-driven instantiation. Interactive input (point/
  distance/angle picks) is abstracted behind a `PickProvider` struct of
  `std::function` callbacks — real AutoCAD I/O is injected by `AcmlTools`, but
  the analyzer itself never touches ObjectARX. Passing an empty `PickProvider`
  makes pick expressions evaluate to `undefined` rather than crash, which is
  what makes headless/batch use possible at all.
- **Generator** (`AcmlGenerator.h/.cpp`): the only ObjectARX-dependent stage.
  Pure dispatch over `ResolvedElement` → `AcDbEntity` (Line/Circle/Arc/
  Polyline/Rectangle/Ellipse/Text, plus plan-view symbols Wall/Door/Window/
  Column). Carries no language logic of its own.
- **Commands** (`AcmlTools.h/.cpp`): three, staged by side effect —
  `ACMLLEX` (tokens only) → `ACMLCHECK` (lex+parse, report errors, **no
  draw**) → `ACML` (full pipeline, draws). `ACMLCHECK` is a genuine dry-run:
  validate without touching the document.

**Inheritance rule** (from `ResolvedElement` comments, §4.2 of ACML's own
spec): transforms (rotation, scale) accumulate down the element tree; visual
attributes (color, linetype, lineweight) explicitly do **not** — each element
defaults to ByLayer unless it sets its own value. Two different inheritance
models coexist by design, not by oversight.

## 2. Where the ADR's parallels hold

| ADR claim | Verdict | Evidence |
|---|---|---|
| Boundary/Area-style "primitive vs. emergent" split is a reasonable shape for a declarative CAD DSL | **Plausible pattern, not evidenced this way in ACML.** ACML's own primitive/emergent split is `ElementTypeDef` (primitive, inheritable) vs. `ComponentDef` (emergent composition of primitives + constraints). If SFML follows *this* shape, Member = `ElementTypeDef`-like, Connection = a `ComponentDef` composing two Members + constraints — closer to ACML's actual mechanism than a literal Boundary/Area port. | `AcmlAst.h` `ElementTypeDefNode` vs. `ComponentDefNode` |
| A staged, low-cost validation step before any expensive/irreversible write | **Directly reusable.** ACML's `ACMLCHECK` vs `ACML` split is exactly the "verify before commit" shape the ADR wants at the Revit-document level (§5). | `AcmlTools.h` |
| Property resolution needs cycle detection + caching for a tree with cross-references | **Reusable as-is.** ACML's `resolving_`/`cache_` pattern in `AcmlSemantic` solves the same problem SFML will hit once Member/Connection references become bidirectional (a Connection referencing two Members that may reference it back). | `AcmlSemantic.h:299-304` |
| Keep the interpreter/analyzer decoupled from the host CAD API so it's testable and reusable | **Directly reusable — and the strongest parallel in this whole comparison.** `PickProvider` is the template: an injectable seam that lets 100% of the semantic logic run without ObjectARX, AutoCAD, or a document open. SFML's C++ engine (ADR §4) should adopt the same seam for anything that currently assumes "Revit is running." | `AcmlSemantic.h:30-47` |

## 3. Where the ADR's parallels do NOT hold — corrections needed

1. **"AI mediates between symbolic architectural intent and executable form"
   (ADR §2) is not what ACML's pipeline does.** Grepping `AITools.cpp` (the
   AI-integration module) for any ACML reference returns nothing. ACML is
   compiled deterministically from human- or tool-authored `.acml` text
   straight through to entities — there is no AI step inside the pipeline
   itself. If AI-authored `.acml` happens, it happens *upstream*, outside
   this codebase (e.g. a chat UI writing the source text a human would
   otherwise write), which is a materially different claim than "AI mediates
   ACML → executable form." **Action: verify with whoever wrote the ADR
   whether an AI-authors-ACML-text step exists elsewhere before using it as
   precedent for SFML's AI-generates-Lua design (ADR §2).** If it doesn't
   exist, SFML's AI-in-the-loop step is a *new* architectural bet, not a
   proven pattern being reused — evaluate it on its own merits.

2. **ACML's verification/constraint system is the closest existing analog to
   SFML §3 — and it is unimplemented.** `ConstraintStmtNode`/
   `ConstraintExprNode` are fully lexed and parsed (`AcmlParser.cpp:358-451`,
   including a pretty-printer at line 937), but `AcmlSemantic.cpp` contains
   **zero** references to either node kind — no `case NodeKind::
   ConstraintStmt` anywhere in the resolver. Constraints parse successfully
   and are then silently dropped; nothing evaluates `coincident(...)` or
   `distance(...) >= 400`. **This means the ADR's guiding principle for SFML
   ("give the tools to AI, but control the output," §3) has no working
   reference implementation to borrow from inside this codebase — the exact
   problem SFML needs to solve (bounded, decidable predicates checked against
   a fixed rule library) is still open in ACML too.** Treat SFML's
   verification layer as greenfield work, not as "port ACML's constraint
   checker" — there isn't one to port yet.

3. **"Six conversations between two Boundaries" and "Axiom Zero" are not
   ACML source concepts** (not found in any `Acml*.{h,cpp}` or `*.md` in
   ArqaTools). The ADR's own §7 already flags this pattern-matching as a risk
   ("reached through pattern-matching to ACML's Axiom Zero rather than
   head-to-head comparison"). This evaluation independently confirms the
   flag: the concept isn't in the reference implementation to compare
   against, so the ADR's framing section (§1) may be reasoning from a
   conceptual document that lives outside this repo, or from an earlier/
   different ACML design than what's actually built. **Action: locate that
   source document (or confirm it's aspirational/from an earlier design pass)
   before treating "six conversations" as a settled precedent for SFML's
   Member/Connection relational model.**

## 4. Net assessment

- The ADR's **structural/process parallels** (staged validation, decoupled
  analyzer, cycle-safe resolution) are sound and have working reference code
  to copy.
- The ADR's **AI-mediation and verification-contract parallels** are framed
  as "mirrors ACML's existing principle," but no such principle currently
  exists in ACML's implementation — both are open problems in ACML too, not
  solved ones being reused. This changes SFML's risk profile: sections 2 and
  3 of the ADR are novel design work wearing ACML's name as precedent it
  hasn't yet earned. That's not a reason to reject them, but they should be
  evaluated as first-of-their-kind decisions (more scrutiny, more prototyping
  before committing) rather than "reuse a working pattern."
- The self-flagged bias in ADR §7 is worth taking at face value on exactly
  these two points.
