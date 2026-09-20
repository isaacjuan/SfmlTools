# SFML - Roadmap

Resolves the ADR's open questions (§6) in dependency order. Each phase names
what it unblocks and what from ACML it can reuse vs. must build fresh (see
`ARCHITECTURE-EVALUATION.md` §2-3 for which is which).

## Phase 0 - Close the precedent gaps (prerequisite, not implementation)

Before any SFML grammar work, resolve the two items `ARCHITECTURE-EVALUATION.md`
flags as unverified precedent:

1. Confirm whether an AI-authors-source-text step exists anywhere upstream of
   ACML's compiler (outside ArqaTools), or whether ADR §2's "AI mediates"
   claim is aspirational. Changes how much design weight SFML's AI→Lua step
   should carry.
2. Locate the "Axiom Zero" / "six conversations" source document, or confirm
   it's from an earlier/parallel design pass not reflected in the current
   ACML implementation.

Neither blocks grammar design mechanically, but both determine how much of
the ADR's framing (§1, §2) is load-bearing vs. inherited assumption.

## Phase 1 - Grammar & primitives (ADR's stated "next phase")

- Decide Member-as-primitive vs. Connection-as-emergent using ACML's actual
  primitive/emergent split as the template: `ElementTypeDef` (inheritable,
  primitive) + `ComponentDef` (composition + constraints, emergent) - not a
  literal Boundary/Area port (see evaluation §2, row 1).
- Resolve whether framing relationships need a relational-conversation model
  at all, given they're governed by load-path type (axial/shear/moment/
  bearing), not spatial adjacency. This may mean SFML's Connection concept is
  simpler than ACML's Area, not an analog of it.
- Deliverable: `SfmlAst.h`-equivalent grammar spec (doc, not code - still
  planning phase) covering Member, Connection, load-path typing, and where
  verification clauses attach syntactically.

## Phase 2 - Verification layer (greenfield - do not treat as "port ACML's checker")

Per evaluation §3.2, ACML has no working constraint evaluator to copy.
This phase is original design work:

- Define the bounded/decidable predicate set (span ≤ limit, load ≤ capacity,
  valid member references) as a closed grammar, not open-ended expressions.
- Design the fixed, human-maintained standards library format and its
  versioning/pinning story (ADR §3's open question - e.g. pinning
  `AISC-360-22 §D2`). This is a new problem; ACML's `ElementTypeDef.inherits`
  single-parent model doesn't address code-edition drift and shouldn't be
  assumed to.
- Decide checker mechanism: direct evaluation vs. SMT-class solver, per
  predicate complexity - ACML gives no guidance here since it never built
  this stage.
- Note who owns/updates the standards library itself (flagged in prior
  review of the ADR as an unaddressed versioning question one level up from
  the clause-pinning question).

## Phase 3 - Generation flow (Lua placement + AI step)

- Prototype the AI→Lua step small and cheap before committing - this is now
  known to be novel, not reused, per Phase 0 finding. A prototype answers
  "does non-deterministic Lua generation over a still-unstable grammar
  produce usable output" before the grammar is fully locked.
- Decide Lua's placement (embedded in C++ engine vs. broker vs. both) once
  Phase 1's grammar stabilizes - placement is cheap to decide, expensive to
  reverse only if broker-level wire-protocol coupling (ADR §4's SFML-as-wire-
  protocol question) is chosen prematurely. Default to embedded-only until
  a concrete cross-process need appears.

## Phase 4 - Execution layer & state ownership

- Reuse ACML's decoupling pattern directly: everything that isn't drawing
  entities should be able to run without Revit open, the same way
  `AcmlSemantic` runs without ObjectARX (evaluation §2, row 4). This is the
  cleanest, most directly transferable piece of precedent in the whole ADR.
- Resolve state-ownership/sync question (ADR §4) only after Phase 2's
  verification contracts exist - sync strategy depends on what "valid state"
  means, which is defined by the verification layer, not the transport.

## Phase 5 - Human checkpoint

- Borrow ACML's staged-command pattern (`ACMLLEX` → `ACMLCHECK` → `ACML`) as
  the template for the Generative-Design-style candidate review (ADR §5):
  cheap non-committing preview stages before the one expensive Revit write.
- Resolve review cadence (per-generation vs. milestone) only after Phase 2
  defines what "verified" means per element - cadence is a consequence of
  verification granularity, not an independent choice.

## Sequencing note

Phases 1 and 2 can run in parallel once Phase 0 is closed - grammar and
verification-predicate design inform each other but don't block each other.
Phases 3-5 each depend on Phase 1 (stable-enough grammar) and, for anything
touching "verified," on Phase 2.
