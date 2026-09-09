# What's new

Changes per release, newest first. Breaking changes are marked **breaking**.

## 1.14.0

### DCL: warm-up and restart, legacy defaults, sample provenance

- **`warmup_time`** (the legacy `L`): on an infinite-horizon MDP a trajectory first runs
  for `warmup_time` periods under the generation's own rollout policy before its decision
  states are labeled. The initial state is the modeller's artifact, not a state the system
  spends time in; warming up samples the stationary behaviour of the policy being
  improved, and the sampled states follow the policy as it improves generation by
  generation. Default 100 on an infinite horizon, forced to 0 on a finite one (there the
  initial state is the honest start of an episode); `warmup_time=0` turns it off.
- **`restart_after`** (the legacy `reinitiate_counter`): after that many labels a
  trajectory is thrown away and restarts from the initial state with a fresh warm-up, so
  that consecutive samples do not all come from one long trajectory sitting in one basin.
  Off by default. Both knobs are part of the experiment identity, on `DCL(...)` and
  `collect_samples(...)` alike.
- **Legacy defaults.** `n=5000` samples per generation, `m=1000` rollouts per candidate
  action, `h=40` on a finite-horizon MDP and `256` on an infinite-horizon one, and the
  legacy training regimen (5 % held out, validated every 5 epochs, best-validation weights
  restored, stop when the epochs since the last improvement exceed the patience). A legacy
  DCL configuration now ports one to one; pass any of them explicitly to override.
- **Sample provenance.** `DCL(sample_info=True)` / `collect_samples(sample_info=True)`
  records, per labeled sample, which controller and trajectory produced it, the
  trajectory it was warmed up from, the label's index along its trajectory and the time it
  was labeled at, as an `info` slab on the `SampleSet` (carried by save, load and merge).
  With it the origin of every training row can be reconstructed and replayed from its
  seeds, which is what `scripts/dcl_audit.py` does.
- **`Engine.view()`** opens the engine's root object for reading from Python: a live view
  whose fields, list elements and arrays reflect the engine's current state, valid while
  the engine lives and no call is in flight. The same views the test suite compares
  compiled results against, now available on any engine.
- **Generation 0 runs on all processes** of the plan, like the later generations; the
  result is byte-identical to the single-process collection.

### DCL on GPUs and cluster nodes: machine plans

- **`DCL(machine=...)`** selects an execution plan — processes (one per GPU), engines,
  workers, slots, the device the network runs on — engineered per machine: the Snellius
  H100 and A100 nodes in quarter/half/whole units, a whole Genoa node, and `laptop`, which
  is derived from what the machine has (Metal on Apple silicon, cuda elsewhere, else cpu).
  `machine="auto"`, **the default**, recognizes the allocation among those plans and
  refuses anything else — a machine without a preset fails up front rather than run on an
  untuned shape; pass `machine=None` to use the explicit knobs as before. Every plan is
  verified against the allocation before a collection starts. See *Training and
  evaluation → Machines and execution plans*.
- **Two shapes per plan, `dynaplex.Shape`.** Generation 0 rolls out the heuristic
  in-machine and has no network forward to place, so every plan carries a separate
  generation-0 shape — one engine per process, workers filling the cores, 512 slots — next
  to the trained policy's shape (two engines to drive an accelerator, cores reserved for
  torch threads on the cpu). Measured on Snellius: about 10 % faster generation-0
  collection on the Genoa and H100 units than the trained shape gives, and one thread per
  process too many halves it. `DCL(gen0=, trained=)` overlay the plan's shapes field by
  field with a `dynaplex.Shape(processes, engines, workers, slots)`: `Shape(workers=8)`
  changes the workers only; without a plan the base is 1 × 1 × 1 × 512. Both shapes are
  part of the experiment identity (`params.gen0` / `params.trained` in `dcl.json`). The
  generation-0 run header says that the rollout policy runs in-machine and the device is
  unused. **Breaking:** the explicit `controllers`, `workers`, `slots` arguments of
  `DCL(...)` are gone; pass `gen0=dp.Shape(...)` / `trained=dp.Shape(...)` instead
  (`collect_samples` keeps its scalar arguments).
- **`dynaplex.machine`**: hardware-vs-allocation detection (affinity mask, cgroup limits,
  visible GPUs, SLURM variables); `python -m dynaplex.machine` prints it.
- The CPU plan `genoa` is 32 processes of one engine with 4 workers each, 512 slots per
  worker and 2 torch threads per process. With the network on the same cores as the
  workers the rule is `processes × (workers + torch threads) ≤ cores`, because workers
  spin at the barrier rather than yielding their cores to the forward. Measured on 2000
  generation-1 samples: 15 s for lost sales and 38 s for lot sizing K=5, against 271 s
  and 1002 s for a single engine with 190 workers.
- **Generation ≥ 1 collection** now alternates engines so the network's forward for one
  engine overlaps the other engine's simulation, with asynchronous device bindings (pinned
  buffers; on cuda one CUDA-graph replay per round) and `processes=P` spawning one
  controller group per GPU. Results are byte-identical to the previous single-loop
  collection on the same device. Measured on a stochastic lot-sizing example (K=5, 150 000 samples
  per generation): 23 minutes per generation on one H100 node, where the previous loop
  needed hours.

### Exact solver

- **`dynaplex.ExactSolver`**: exact policy evaluation and optimization of a finite-state
  MDP. Construction discovers every decision state reachable under the model's own
  dynamics, running the model's transition methods compiled in the engine with each random
  draw enumerated instead of sampled; `evaluate(policy)` then computes a compiled policy's
  exact cost and `solve(warm_start=None)` the optimal policy by modified policy iteration.
  The result carries the cost, the value and action tables and `get_policy()`; the latter
  is a normal compiled policy (`ExactPolicy`, or `ExactTablePolicy` when the state space
  has several shapes) that runs in the comparer and as a DCL generation-0 policy. Models
  need discrete, enumerable events (the `DISCRETE_IDENTIFIABLE_EVENTS` promises) and an
  integer-valued state; guardrails `max_states` and `max_event_branches` bound discovery.
  Single-threaded and in memory in this version. See *Training and evaluation → Exact
  solver*.
- **Both horizons.** On an infinite-horizon MDP the cost is the long-run average cost per
  period and `res.values` the bias (`get_bias_function()`). On a finite-horizon MDP it is
  the expected total cost of an episode, from `get_initial_state` until the state becomes
  `FINAL`, and `res.values` the expected cost-to-go (`get_value_function()`);
  `res.stats["horizon"]` says which. Both forms of ending are covered — an internal clock
  such as the airplane's `remaining_days`, and stopping problems where reaching `FINAL`
  depends on the action — under the assumption, not checked, that every policy reaches
  `FINAL` with probability one (a model that does not shows as non-convergence). The
  initial distribution is enumerated like the events, so `get_initial_state` may draw
  discretely: the promise `discrete-initial-draws-only` replaces the deterministic-start
  rule. Finite-horizon sweeps update values in place and can follow an order the model
  supplies (an optional `sweep_rank(self, state) -> int`; the airplane model returns
  `remaining_days`, which makes the solve a single backward-induction pass).

### Bitwise and shift operators; `//` and `%` floor like Python

- **`&`, `|`, `^`, `<<`, `>>`, `~`** and the augmented forms `&=`, `|=`, `^=`,
  `<<=`, `>>=` now compile, on `int` and `bool` operands with Python's types
  (`bool & bool` is a `bool`, shifts are `int`; a `float` operand is a compile-time
  error, `~` takes an `int`). They are native instructions in both tiers, so bit
  packing that previously had to be a builtin (`combine_seeds`-style) can be written
  in DynaML. Integers are 64-bit: a left shift wraps modulo 2**64, a count of 64 or
  more gives `0` (`<<`) or the sign fill (`>>`), and a negative count raises in
  checked mode (fast mode saturates). Known values fold at compile time, so
  `static(self.cfg.mask & 4 == 4)` prunes like any other known condition. See
  *Language reference → Arithmetic*.
- **`//` and `%` on integers now follow Python** for negative operands
  (`-7 // 2 == -4`, `-7 % 2 == 1`); until now they truncated toward zero like C++
  (`-3`, `-1`). A model that divided or reduced negative integers changes result.
  Float `//` and `%` were already Python's.

### `dynaplex.equals`: value equality that includes arrays

- **`dynaplex.equals(a, b)`** compares two values of the same type as values,
  arrays included: `NDArray` fields (and arrays passed directly) are equal when shape,
  dtype and every element agree. On everything else it is exactly `==`, so a class's
  own `__eq__` and `dataclasses.field(compare=False)` behave as before. It compiles
  in DynaML like `==` does. Until now a state holding an array could only take
  part in equality by marking the array `compare=False`, which also dropped it
  from `deep_hash`; **`deep_hash` now hashes arrays** (axis lengths, then elements),
  consistent with `equals`, so such fields no longer need the opt-out. Plain `==`
  is unchanged and still refuses array fields, to align with CPython semantics.

### `@classmethod` and `@staticmethod` on dataclasses

- **`@classmethod` and `@staticmethod`** now compile on dataclasses, called on the
  class: `Point.origin()`, `Point.manhattan(a, b)`. Inside a classmethod `cls(...)`
  builds an instance and `cls.other(...)` calls another classmethod, so factory
  methods work as in Python. Calling either through an instance is not supported;
  use the class name. See *Language reference → Method signatures*.

### Weighted `rng.choice(p=...)` retired

- **breaking** — the weighted form `rng.choice(xs, p=probs)` no longer compiles:
  every draw rescanned and renormalized `probs`, and `DiscreteDist.custom(probs)`
  with an `alias_sampler()` (constant time per draw) or `cdf_sampler()` is the
  precomputed replacement. Passing `p=` is now a compile-time error naming that
  replacement, and the plain-Python `dynaplex.Generator.choice` rejects `p=` the
  same way. The unweighted forms `rng.choice(n)` and `rng.choice(xs)` are unchanged.
  `dynaplex._core.HAS_WEIGHTED_CHOICE` reports whether a build carries the weighted
  kernels (`False` on released wheels).

### Tianshou

- **`dynaplex.gym.TianshouVectorEnv`** runs a DynaPlex MDP under
  [Tianshou](https://tianshou.org)'s vector-env contract: per-environment observation
  and info dicts, list-valued spaces, subset `step(action, id)` / `reset(env_id)`, and
  the action-validity mask inside the observation under `"mask"`, where Tianshou's
  discrete algorithms read it. The MDP still steps batched in the compiled kernel; only
  the boundary layer is per environment. Install with `pip install "dynaplex[tianshou]"`;
  see *Training and evaluation → Vectorized gym environments*, which walks through a
  worked DQN script that scores its result against a heuristic with the comparer.
- It runs on a new **`VectorEnv(autoreset=False)`** park mode, also usable on its own: an
  ended episode leaves its terminal observation in the row and the environment stands
  parked — skipped by `step`, keeping its terminal flags — until `reset(env_id=...)`
  revives it. Episode streams are unaffected, so parking and reviving reproduces an
  autoreset run bit for bit.

### Documentation

- New [Discrete distributions](reference/discrete-distributions.md) page: a high-level
  inventory of `DiscreteDist` — supported families and the two-moment fit, inspection,
  convolution and mixing, the three ways to draw, exact enumeration, and the tail
  truncation that makes it unsuitable for rare-event probabilities.

### Runtime checks: `assert` follows Python, `dynaplex.fail`, fast mode and rehearsal

- **`assert` is a check, as in Python.** Compiled code now runs in one of two modes:
  *checked*, where every `assert`, `if __debug__:` block and list bounds check is live,
  and *fast*, where they are compiled out (Python `-O` semantics; an index out of range
  is then undefined behaviour, the C release-build trade). Running fast makes
  featurization and policy loops about 20 % faster, with bit-identical results.
- **`dynaplex.fail(msg)`** is the way to fail on purpose: it raises `DynaPlexError(msg)`
  in every mode (and in CPython) and never returns, so a method may end on it. Use it
  for the branch that must not be reached; keep `assert` for invariants.
- **`__debug__`** is the compile-time checks flag; `if __debug__:` compiles only the
  taken branch.
- **`checks=`** on `Engine`/`Program` (`True`/`False`, default checked) and on
  `PolicyComparer`, `DCL` and `VectorEnv` (`True`/`False`/`"rehearsal"`). The algorithms
  default to **`"rehearsal"`**: a short seeded pass through their own kernels compiled
  checked, then the fast compile. **`DYNAPLEX_CHECKS=0|1`** overrides any default from
  the environment (rerun a misbehaving script fully checked, no code change).
- **`dynaplex.rehearse(mdp, policy, features=..., trajectories=...)`** — the checked pass
  as a standalone model checker, plus a bytewise CPython-vs-compiled parity comparison.
- **`dynaplex.check_mdp(mdp, policy=None, seeds=64, periods=128)`** — the model check
  to run before compiling anything: plain Python, seeded trajectories under the policy
  (default: a random policy over the valid actions), and a failure names the method
  and the bug — a state category that never changes, an infinite-horizon model that
  never advances `time_elapsed` or reaches `FINAL`, a validity mask with unwritten
  entries or no valid action, a policy choosing an invalid action, a model that keeps
  state outside the `State` object (two runs of the same seed diverge), and a model
  whose every decision state has exactly one valid action — the case that used to
  stall DCL silently before its first round. With `features=` it also featurizes
  every decision state of the first trajectory and fails on a NaN or infinite
  feature value. Returns the counts (events, decisions, valid actions per decision,
  feature rows) when it passes. Opt-outs: `relax_program_flow`,
  `skip_no_decisions_check`. Run it first, `rehearse` second.

### Model validation: classes carry promises

- **`@mdp` and `@policy`** (importable from `dynaplex`; apply above `@const_dataclass`):
  the class promises the MDP / policy contract for its methods — in `get_initial_state` and the transitions, the cost accumulator is
  `+=`/`-=`-only, `time_elapsed` is `+=`-only, no generator is used in
  `modify_state_with_action`, the event step never draws from `policy_rng`, generators are
  never re-bound or stored, `write_action_validity` and `get_action` only read the state, a
  policy draws from `policy_rng` only, never re-seeds or stores it, and never reads the cost.
  Statistics and `Scratch` fields you add to the context are unconstrained in every role. Each rule also sees through
  helpers: handing the state or a stream to a method that mutates or draws counts. `@featurizer` promises that `write_features` only reads the state. The compiler
  checks every promise whenever the methods compile; a violation is a compile error naming
  the model's line and explaining the rule. Undecorated classes are unchanged. See
  *Language reference → Promises*.
- **`@mdp(promises=...)`** appends an algorithm's set; `dynaplex.validation.validate(engine,
  contract)` gives a non-raising verdict, and `DISCRETE_IDENTIFIABLE_EVENTS` is the set the
  exact solver requires.
- The shipped models (`dynaplex.models`) and the examples now carry `@mdp` / `@policy`.
- **breaking** — `PolicyComparer(backend=...)` and `gym.VectorEnv(backend=...)` default to
  `"engine"`: a model that fails to compile, or violates a promise, raises instead of
  silently running on the Python kernel. `"auto"` remains available as the explicit
  opt-in to that fallback and now warns when it falls back. Violations raise
  `dynaplex.validation.PromiseViolation` (a `ValueError`).

### Compile-time configuration: `static()` and value folding

- **`static(expr)`** (`dynaplex.modelling.static`): a compile-time branch decision.
  `if static(self.mdp.has_lead_time):` selects the branch from the configuration the
  engine was constructed with; the untaken branch is discarded *before* type-checking,
  so one model body can serve a whole MDP family whose variants have different fields.
  `expr` must be statically known (a chain of `Final`/`@const_dataclass` hops from the
  engine root, or from a world's known constructor arguments); otherwise compilation
  fails with a message naming the hop or the call/spawn site that broke the chain.
  See *Advanced → Compile-time configuration with `static()`*.
- **Specialization.** Methods called on a known configuration object, and
  `@separate_world_class` worlds created with known constructor arguments, are compiled
  once per distinct configuration. Known configuration values (`self.mdp.scale`,
  `self.horizon` in such a world) compile to constants, so the JIT can fold compares,
  dead branches and constant loop bounds. Results are bit-identical to unspecialized code.
- `DYNAPLEX_DISABLE_FOLDING=1` disables all of the above (models using `static()` then
  fail to compile, deliberately).

### Fixes

- **Integer division by zero and float-to-int overflow no longer depend on the
  hardware.** An integer `//` or `%` by zero was a silent `0` on arm64 and a process-killing
  SIGFPE on x86; `int()`, `math.floor()`, `math.ceil()` and `round()` of NaN, infinity or a
  float outside the 64-bit range saturated on arm64 and returned the minimum int on x86.
  Both are now checked-mode errors with Python's message (`ZeroDivisionError`,
  `ValueError`, `OverflowError` texts), emitted like the negative-shift check, and in fast
  mode compute the same value everywhere: `0` for the division, a saturated int for the
  conversion (NaN gives `0`). `-2**63 // -1` wraps instead of trapping. Documented under
  *Arithmetic* in the language reference.
- **64-bit integer overflow is a checked-mode error.** Python ints are unbounded; DynaML's
  are 64-bit and wrap. A result of `+`, `-`, `*`, `**`, `<<`, unary `-` or `abs` (or
  `-2**63 // -1`) that leaves the range now raises in checked mode with the offending
  expression (`integer overflow: 9223372036854775807 + 1 does not fit in a 64-bit int`),
  so a model whose counts could leave the range fails in rehearsal. Fast mode is unchanged
  and still wraps; a compile-time constant that would overflow is no longer folded, so the
  same rule applies to it.
- **`abs(-0.0)`, and `min`/`max` on floats with a tie or a NaN, now follow Python.**
  `abs(-0.0)` returned `-0.0` (now `0.0`); two-argument `min`/`max` picked the second
  argument on a tie and whenever a NaN was involved, where Python keeps the first unless
  the second is strictly smaller or larger (`max(0.0, -0.0) == 0.0`, `max(0.5, nan) == 0.5`,
  `max(nan, 0.5)` is `nan`). The three-or-more-argument and list forms already did.
- **`%` and `//` on floats, `**` on ints, now follow Python.** Float `%` was C's
  `fmod`, so the remainder took the sign of the dividend (`-7.0 % 2.0` gave `-1.0`, Python
  gives `1.0`); float `//` was the floor of the rounded quotient (`1.0 // 0.1` gave `10.0`,
  Python gives `9.0`); and int `**` went through a double, losing digits above 2**53
  (`3 ** 39` came back wrong by rounding). All three now compute CPython's value on both
  tiers, with int `**` an exact integer power that wraps modulo 2**64 like `*`. Seeded
  results that go through one of these operators on such operands change accordingly.
- The comparer's NN path no longer hands the network a feature row it never wrote. An
  episode that ends without ever awaiting an action is legitimate — events alone can
  finish a trajectory before the first decision — and it still counts in the average,
  since no policy could have influenced it; but it leaves its episode slot parked, and
  with `nn_batch` at or above the number of trajectories each slot hosts exactly one
  episode, so a single such episode used to put all-zero features under an all-empty
  action mask into the forward batch. Parked rows are now filled from a live slot before
  each forward, so the network only ever sees a complete featurization of a real decision
  state; results are unchanged, since a parked slot's action was already discarded. A
  comparison in which *no* episode ever awaited an action now raises instead of reporting
  numbers that measure nothing.
- Compile errors inside a method that was compiled because another method calls it now
  end with that chain of calls, nearest caller first, each with its file, line and the
  call itself — so an error raised in library code points back at your model.
- The one-shot draw methods on `DiscreteDist` (`poisson_sample`, `binomial_sample`,
  `negative_binomial_sample`, `geometric_sample`, `adan_eenige_resing_sample`) use a
  faster sampling scheme; draws remain exact, but the **draw sequence changes**, so
  seeded results that go through them differ from 1.13.
- The documentation links in compile-error and validation messages now point at the docs
  for the **installed** version (`.../DynaPlex/1.14/...`); a development build links to
  `latest`. Previously every such link was unversioned and 404ed on the versioned site.
- Enum-typed dataclass fields with a default (`act: Action = Action.UP`) can now be
  omitted in constructor calls inside compiled code; previously the default was rejected
  with "expected Enum(...), got Int".

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
