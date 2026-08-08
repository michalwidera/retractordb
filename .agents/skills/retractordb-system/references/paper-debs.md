# DEBS 2027 paper map

## Version basis and file roles

The paper is maintained in the sibling `paper-arXiv` repository. The indexed basis is branch `main`, commit
`5f8f28dec026ac2e64dc9a4ef6f662578a210803` (2026-08-08). Engine behavior still has to be verified in `retractordb`;
the manuscript and planning files are not implementation evidence.

| File | Role |
|---|---|
| `debs/main-debs.tex` | English submission source and claim authority |
| `debs/main-debs-pl.tex` | synchronized Polish working translation |
| `debs/research_plan.md` | detailed research ledger; later status sections override stale summary rows |
| `debs/harmonogram.md` | operational deadlines, gates, and abandonment rules |
| `debs/related_work_k8.md` | recorded novelty/related-work gate |
| `debs/references.bib` | bibliography and remaining metadata checks |
| `debs/done/` | completed protocols and repair records; historical, not the active task list |

Repository policy requires substantive edits in English first and synchronization of the Polish version in the same
session. ACM generative-AI disclosure belongs in the venue-designated statement; the current anonymous source keeps the
camera-ready acknowledgment as a commented block.

## Current manuscript snapshot

The current PDFs were rebuilt on 2026-08-08 with no unresolved references or citations. The English manuscript is 18
pages and the Polish working version is 20 pages. The working DEBS limit remains 12 pages, inherited from the 2026 CFP
until the 2027 CFP is published. The English submission therefore needs roughly six pages of reduction during K17; do
not mistake the Polish page count for the submission budget.

The manuscript's coupled thesis is:

1. RQL forms a declarative boundary for static, regular, fixed-schema monitoring: the compiler/runtime own scheduling,
   history, window state, startup boundaries, and sharing.
2. Algebra-aware plan normalization can expose common work, but a smaller plan does not automatically reduce slot cost.
3. The formal/operator boundary is positioned against SDF/CSDF without claiming that the studied computation is
   inexpressible in those models.

The reported optimizer result is deliberately negative: five profiles, 13 timed cells, and 780 runs yield 12 neutral
cells and one regression under the preregistered median criterion. The two best ratios occur in W9, the only family where
sharing halves in-memory materialization. This is an association and motivation for a controlled follow-up, not a causal
performance claim.

The semantic contribution reports a static logical-origin/startup-tail calculus over nine operator classes. At the
pinned campaign revision `34db1a2`, six classes are exact, three safely over-approximate by exactly one slot, none
under-approximate, and logical origin is exact for all nine in the measured corpus. Interleave uses an `O(p+q)` phase
scan with a safe `O(1)` fallback; the paper must not collapse this into a uniform closed-form claim.

## Prior-work boundary

The Beatty/Fraenkel bridge, the stream operators, and the shift-matching identity were published in regional
peer-reviewed work in 2005/2006. The current wording states that these publications had limited international reach,
were not internationally recognized, did not develop a significant citation record, and did not establish a visible
systems research line. Limited dissemination motivates a complete international account but does not restore priority.

The paper's novelty claim is the guarded query-plan realization, integration with structural sharing, and controlled
system evaluation. Do not describe the mathematical identity itself as new, and do not use low citation impact as a
novelty argument.

## Evidence and revision discipline

Paper claims mix several deliberately pinned evidence bases:

- K6c optimizer ablation and K18 performance/replay use their recorded engine and experiment revisions; they are not
  measurements of the current checkout merely because the prose says "current revision" inside that campaign context.
- K24d tail/origin numbers are pinned to engine `34db1a2`. Later commits add fail-closed compiler checks, regression
  armour, and workspace metadata; a semantic change to `computeLogicalOrigin()`, `computeStartupLatency()`,
  `SOperations.hpp`, or `dataModel.cpp` invalidates the campaign and requires rerunning it before changing the table.
- Current per-commit regressions include `ut_h10aGate`, `it_ecg_qrs-run`, and `it_replay_stability-run`; they strengthen
  maintenance evidence but do not retroactively change a preregistered campaign's population or statistics.

The article package must pin full, separate SHAs for `retractordb` and `rdb-experiment`. The VS Code workspace file
`retractordb.code-workspace` is navigation convenience only; it does not version the sibling repositories.

## Active path to submission

The critical remaining path in `research_plan.md`/`harmonogram.md` is:

1. **K9b**: artifact package with separate repository SHAs, cross-repository manifest, raw-data checksums,
   reproducibility instructions, and a citable release/DOI.
2. **K17**: page reduction, anonymous-repository audit, final reference metadata, figure regeneration, official DEBS
   2027 CFP check, and final submission build.
3. **K23/H9** is optional. If its compile-only pilot has not started by 2026-11-15, or has no verdict by 2027-01-10,
   the submission proceeds without H9.

The schedule assumes readiness by 2027-02-20 and an unconfirmed research-paper deadline near 2027-03-10. Replace these
dates when the official 2027 CFP appears. The conference dates recorded in the schedule are 2027-06-29 through
2027-07-01 in Galway.

Mandatory K17 debt already recorded in the schedule:

- reduce the English paper from 18 to at most 12 pages, following the cut order in `research_plan.md` section 13.2;
- regenerate `fig:qrs` from the repaired engine in both figure copies, or weaken the adjacent exact-replay sentence;
- verify five bibliography entries marked by the `TODO-REFS (K8)` block;
- preserve or appropriately place the ACM generative-AI disclosure under the final anonymous-submission instructions.

## Known paper and planning drift

Treat these as navigation warnings until corrected in `paper-arXiv`:

- `main-debs.tex` first derives `eq:interleave-tail` from the closed-form phase bound and calls it the startup tail, but
  later `eq:hash-tail` correctly states that the exact interleave tail is an `O(p+q)` period scan and the former form is
  only a safe fallback. The earlier derivation must be reframed to remove the internal contradiction.
- The compilation-pipeline prose says that history capacities are assigned before `W` is computed. Current code runs
  `computeLogicalOrigin()`, then `computeStartupLatency()`, then `computeRequiredCapacities()`.
- The header/status table of `research_plan.md` retains older engine/documentation revisions and a 172-test inventory.
  The live code checkout is versioned with this skill, documentation bases are in `references/provenance.md`, and current
  CTest inventory is 183. Later sections 14--16 are newer than the header and should be consulted first.
- `research_plan.md` section 1.3 still says the 2005/2006 result should remain framed merely as "in peer-reviewed form".
  The manuscript's newer regional/limited-impact wording is the active prior-work boundary.
- Some experimental paragraphs call an older pinned campaign revision "current". Read the explicit SHA and artifact
  manifest, not that adjective, when deciding which checkout supports a number.

## Fast workflow for paper changes

1. Run the skill freshness check and inspect any paper diff from the indexed SHA.
2. Verify every behavior or formula against current code and the tests map; preserve the campaign SHA for measured
   numbers.
3. Edit `main-debs.tex`, synchronize `main-debs-pl.tex`, and update the plan/schedule if status or ordering changed.
4. Build both PDFs with `latexmk -pdf -interaction=nonstopmode -halt-on-error`.
5. Check undefined references/citations, overfull boxes, page counts, citation-key parity, and the normalized numbers in
   both language versions.
6. Leave changes uncommitted unless the paper repository's human owner requests otherwise.
