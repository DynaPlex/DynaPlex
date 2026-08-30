# What's new

Changes per release, newest first. Breaking changes are marked **breaking**.

## 1.13.0

**Platform**

- **Python 3.11 and 3.12 supported** (previously 3.13+). Wheels ship for
  CPython 3.11–3.14. One documented semantic note: generated `__eq__` on
  model classes follows CPython 3.13 dataclass semantics (per-field `==`,
  no identity shortcut for a shared NaN or non-reflexive field object) on
  every Python version.

**Modelling**

- **`Featurizer` base class.** Featurizers now subclass `dynaplex.Featurizer`
  (`@featurizer @dataclass(slots=True) class Mine(Featurizer): ...`), mirroring
  `TrajectoryContext`. The base carries no fields and no behaviour; it exists so
  type checkers accept a featurizer where `features=` is expected
  (`DCL`, `PPO`, `VectorEnv`) and see the synthesized `install`/`reset`/
  `finish`/`spec`/`Holder`. Existing featurizers keep working at runtime
  without the base — only the static type changes. *Breaking for type
  checkers only.*

- **Trajectory-context statistics.** An MDP can declare its own trajectory
  context — a `@trajectory_context` dataclass carrying per-trajectory
  statistics (scalars and fixed-shape arrays) — and construct it in
  `make_context(self)`. `PolicyComparer` returns them as
  `assessment.stats`, one `[n, *shape]` array per statistic, rows aligned
  across policies under common random numbers. The holder is a dataclass
  you declare (`class MyStats(TrajectoryStats)`, named on the context as
  `Stats = MyStats`), so `PolicyComparer(mdp)` is a `PolicyComparer[MyStats]`
  and `assessment.stats` is typed in the editor; leave `Stats` out and a
  holder is derived (untyped).
  See the [tutorial](advanced/airplane-statistics.md) and the
  [language reference](reference/language-reference.md#trajectory-contexts-and-statistics).
- **Context scratch.** `Scratch[...]` marks a context scalar or array as
  per-trajectory scratch — carried by clones, but neither collected into
  `Stats` nor touched by `reset()`: it belongs to the context-driven policy
  that uses it (a plan computed once per period), which decides from the
  state when it is stale. Policy bookkeeping without putting it in the state.
- **Dataclass inheritance** (one base, one level): fields and methods are
  inherited as in CPython, overrides may call the base version
  (`super().m(...)` / `Base.m(self, ...)`), and a hand-written subclass
  `__init__` calls the base's. Custom trajectory contexts now inherit from
  `TrajectoryContext` instead of repeating its five members; `slots=True` is
  no longer required on a context. `@trajectory_context` rejects, at class
  definition, a subclass whose `@dataclass` generated an `__init__` over the
  base's hand-written one (the mix the compiler also rejects; in plain
  CPython it silently yields a positional-over-all-fields constructor).
  [Reference](reference/language-reference.md#inheritance).
- **Custom `__init__` and `__post_init__`** on DynaML dataclasses (states,
  contexts, helper objects): construct with your own signature, fields are
  checked for definite assignment at compile time.
  [Reference](reference/language-reference.md#custom-__init__-and-__post_init__).
- **For-loop targets are scoped to the loop**: using the loop variable after
  the loop (or shadowing an outer name) is rejected, matching what compiled
  code actually did.
- `GlobalStateWriter(h.v)` — the writer sizes itself from its array;
  hand-written `install` bodies no longer pass the width.

**Evaluation**

- `PolicyAssessment.stats` (see above). For the vanilla context this holds
  `cumulative_cost` and `time_elapsed` per trajectory.
- Infinite-horizon evaluation resets the context (not the streams) at the
  warmup boundary; window values restart from zero.

**Tooling**

- `dynaplex.set_jit_opt_level(level)` selects the LLVM optimisation level
  for compiled kernels (default 2; 0 compiles ~5x faster with bit-identical
  results — the test suite runs at 0).

**Breaking**

- `make_context(mdp, seed)` and `reseed_context(...)` are removed. Construct a
  seeded context with `dynaplex.modelling.new_context(mdp, seed)` — the MDP's
  own context when it declares `make_context()`, else a `TrajectoryContext`
  (`context.reseed(seed)` re-seeds an existing one).
- `TrajectoryContext` is no longer constructed positionally
  (`TrajectoryContext(rng, rng, 0.0, 0, valid)`); use `TrajectoryContext(mdp)`.
- A context-driven policy must annotate `context` with the MDP's context
  class (`TrajectoryContext` for MDPs without a custom context).

**Fixes**

- The airplane tutorial MDP now compiles on the engine (it silently ran on
  the Python backend before) and draws customer types from a
  `DiscreteDist` / `AliasSampler`.
- CPython `clone()` of an object whose class has a hand-written `__init__`.

## 1.12.3

- First public release on PyPI.
