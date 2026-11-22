# What's new

Changes per release, newest first. Breaking changes are marked **breaking**.

## 2.0.0

### States on disk

A DynaPlex MDP model can now write its states to disk and read them back, from
compiled model code, at the speed of the simulation.

This was prepared over several releases. An MDP model is a constant object that
states refer to rather than copy: a job points at its task type, a vehicle at its
depot. A stored state keeps those references as *places in the MDP model*, so a file
holds only what is truly state, and reading it back restores every reference to the
MDP model's own objects. Objects shared between fields, and cycles, come back as they were.

Writing happens inside the engine, without returning to Python: a million states of a
few kilobytes each are written in a second or two, and compiled code and plain Python
write the same bytes.

For DCL this changes what a collection is: it stores the states it sampled instead of
their features, so one collection serves every featurizer and network you want to try
(next entry).

- [`RecordWriter`][dynaplex.RecordWriter] and [`RecordReader`][dynaplex.RecordReader]
  are opened in plain Python for one class and handed to model code, which calls
  `append_record(obj, writer)`, `read_record(obj, reader, k)` to read into an existing
  object, and `read_record_into(objs, reader, k)` to append a new object to a list. Any
  class that is not const can be a record: a state, statistics, or both.
- [`dynaplex.reserve(xs, n)`][dynaplex.reserve] makes room in a list for `n` elements
  ahead of a loop of appends. It never changes what a program computes.

### DCL: collect once, then try featurizers and networks (**breaking**)

- **`DCL(mdp, policy, ...)` no longer takes `features`, `network` or `train`.** A `DCL`
  now describes a collection; the featurizer, the network and the training settings are
  given when you train:

    ```python
    dcl = dp.DCL(mdp, policy, n=4000, m=200, h=40)
    dcl.collect_gen0()                                         # once
    agent = dcl.train_policy(features=F, network=dp.MLP(hidden=[128, 128]))
    agents = dcl.run(3, features=F, network=dp.MLP(hidden=[128, 128]))
    ```

    To port: move `features=`, `network=` and `train=` from the constructor to
    `run(generations, ...)`. `dcl.agent()`, `dcl.agents()` and `dcl.samples(g)` move to
    the object `dcl.experiment(features=..., network=..., train=...)` returns
    ([`DCLExperiment`][dynaplex.DCLExperiment]); `dcl.collect(g)` is replaced by
    `dcl.collect_gen0()`.
- **Generation 0 is collected without a featurizer and is shared.** It stores the state
  and the statistics of every sample on disk, so any number of featurizers and networks
  can be trained on it without collecting again. Each (featurizer, network, training
  settings) is an *experiment* on that collection, with its own line of agents from
  generation 1 on.
- **A collection writes no features**, in any generation.
  [`dynaplex.featurize(mdp, states, features)`][dynaplex.featurize] makes them from the
  stored states (100,000 lost-sales states in well under a second), and DCL does that
  when it trains. Nothing featurized is kept on disk.
- **The directory layout changed**, see the next entry. Run directories of earlier
  versions are not reused: delete `dynaplex_runs/` after upgrading.
- **`dynaplex.collect_samples` takes `records=` (required) and no featurizer for
  generation 0.** It writes `records/states` and `records/statistics` while it collects
  and returns a [`SampleSet`][dynaplex.SampleSet] without features;
  `SampleSet.with_features(...)` attaches the result of `featurize`. With a
  `rollout_agent`, `features=` is the agent's featurizer and is required.
  `dynaplex.dcl.collect_gen0_samples` is removed.
- `keep_samples` no longer removes generation 0, which every experiment shares.

See [Deep Controlled Learning](training/dcl.md).

### Run directories: readable names, labels, and when a run is reused (**breaking**)

- **A run directory says what is in it.**

    ```text
    dynaplex_runs/
        LostSalesMDP_dcl_3f9a1c2e/                  a DCL collection: dcl.json, gen0/
            LostSalesFeaturizer_mlp_128x128_a41b07/ an experiment on it: exp.json, agent_gen1/, gen1/, ...
        LostSalesMDP_ppo_3f9a1c2e/                  the MDP model, for PPO: mdp.json
            LostSalesFeaturizer_mlp_128x128_9c22d4/ one PPO run: ppo.json, best_agent/, final_agent/, ...
    ```

    The class of the MDP model, the featurizer and the network are written out; what is
    left is a hash of the settings. All hashes differ from 1.14, so no run directory of
    an earlier version is found again.
- **A label stands where a hash would be.** `DCL(..., gen0_label="small")` gives
  `LostSalesMDP_dcl_small/`; `training_label="lr3"` on `train_policy`, `run` or
  `experiment` gives `LostSalesFeaturizer_mlp_128x128_lr3/`. `PPO` takes `mdp_label`
  and `training_label` in the same way. Without a label every distinct setting has its
  own directory. With a label, a run whose settings differ from what the directory
  holds stops with [`WorkdirMismatchError`][dynaplex.WorkdirMismatchError], which lists
  the differences. Nothing on disk is ever overwritten.
- **A network factory names its networks.**
  [`@dp.net_factory(label=...)`][dynaplex.net_factory] on a factory gives the name
  used in the directory, as a fixed string or as a function of the factory's
  parameters. Without it the name is the factory's function name without a trailing
  `_factory`. `dp.MLP(hidden=[128, 128])` is called `mlp_128x128`.
- **The code of the MDP model and of the rollout policy is part of what a collection is.**
  In 1.14 a run was found by the parameter values alone: after an edit of
  `modify_state_with_event` the same script printed `agent_gen1 exists — skipping` and
  returned the agent of the old MDP model. Now an edited MDP model or policy is another
  collection, with its own directory (under a `gen0_label`: refused). The file the
  code is in does not count: an MDP model that moved to another module is the same
  MDP model.
- **The code of a featurizer or a network is NOT looked at.** An experiment is found by
  the featurizer's name, the network's name and parameters, and the training settings.
  After editing the body of a featurizer or of a network factory, the same script
  loads the agents that are there. Delete the experiment's directory, or use another
  `training_label`.
- **The machine is not part of what a run is.** In 1.14 the split into processes,
  engines, workers and slots counted, so a run moved to another computer, or started
  with another `machine=`, began again. It now finds its directory and goes on. When
  it uses samples that were collected under another split it prints one line saying
  so. The split decides which states are sampled, so two machines do not collect the
  same generation 0, but each is a valid collection of the same MDP model.
- **Where runs are stored.** `root=` when given, else the environment variable
  `DYNAPLEX_RUNS_DIR` when set, else `dynaplex_runs` in the directory Python was
  started from. `DCL` and `PPO` print the absolute path.
- **PPO: `{MDPClass}_ppo_{hash}/` holds the MDP model, a directory below it one run.** It
  was `{MDPClass}_{hash}/ppo_{hash}/`. `ppo.json` is now compared when a directory is
  found: a `workdir=` or label that holds a run with other settings is refused.

### One run collects generation 0 at a time

- Several runs on one collection (a sweep over networks or training seeds started at
  once, a second terminal) each found no generation 0 and collected into the same
  directory, and one removed the unfinished files of the other. A run that finds
  another run of the same machine collecting generation 0 now stops with
  [`CollectionInProgressError`][dynaplex.CollectionInProgressError]. Collect first
  (`dcl.collect_gen0()`), then start the runs that train: with generation 0 on disk
  they only read it. The claim of a run that was killed is taken over, so running a
  script again still resumes. Runs on different machines that share a disk are not
  checked.

### `VectorEnv` follows the Gymnasium 1.x interface (**breaking**)

- **`gymnasium>=1.1` is required** (`pip install "dynaplex[gym]"`), and
  `dynaplex.gym.VectorEnv` is a `gymnasium.vector.VectorEnv`.
- **`autoreset=` is replaced by `autoreset_mode=`**, one of Gymnasium's three
  `AutoresetMode`s (importable from `dynaplex.gym`), and the default changed:

    | 1.14 | 2.0 |
    | --- | --- |
    | `autoreset=True` (default) | `autoreset_mode=AutoresetMode.SAME_STEP` |
    | `autoreset=False` | `autoreset_mode=AutoresetMode.DISABLED` |
    | (did not exist) | `autoreset_mode=AutoresetMode.NEXT_STEP`, **the default** |

    Under `NEXT_STEP` the row of an episode that ended holds its end state, and the
    next `step()` on that env ignores its action and returns the first observation of
    the next episode, with reward 0, `infos["dt"]` 0 and both flags False. A training
    loop written for 1.14 keeps its behaviour with `AutoresetMode.SAME_STEP`.
- **`infos["final_observation"]` is now `infos["final_obs"]`** (and
  `infos["_final_observation"]` is `infos["_final_obs"]`), under `SAME_STEP`.
- **`env.spec` is Gymnasium's `EnvSpec` now.** The feature spec of the observation
  (`dict[str, TensorSpec]`) is `env.feature_spec`.
- In `DISABLED` mode, `reset(options={"reset_mask": mask})` restarts the envs where
  `mask` is True (Gymnasium's form); `reset(env_id=[...])` still works.
- The three modes run the same episodes, and so does the Python backend.

See [Vectorized gym environments](training/gym-environments.md).

### PPO discounts by the time a step took

- `step()` returns `infos["dt"]`: the time in the MDP model that each env's step took (1 for
  an MDP model with one decision per period, 0 for a decision that takes no time).
  [`PPO`][dynaplex.PPO] now discounts a step by `gamma ** dt`. Before, it discounted
  once per decision, which on an MDP model where the number of decisions per period depends
  on the policy optimises the cost per decision, not per period: a policy could look
  better by taking more decisions. On an MDP model with one decision per period nothing
  changes in the method. Trained weights are not expected to be the same as in 1.14.

### PPO runs on the compiled environment, or stops

- `PPOConfig.backend` defaults to `"engine"` (was `"auto"`). An MDP model or featurizer that
  does not compile is now an error under [`PPO`][dynaplex.PPO]; before, training went
  on in the Python environment at about a tenth of the speed. `backend="auto"` still falls back, and `VectorEnv` now warns every time it
  does; `backend="python"` asks for the Python environment.

### `PolicyComparer`: a trained agent runs on the engine

- `PolicyComparer(backend="python")` refuses an [`NNAgent`][dynaplex.NNAgent], as
  before; the error now names what to use instead (`backend="engine"`, the default, or
  `"auto"`), and the rule is on the [Comparing policies](training/policy-comparison.md)
  page.

### `resource_allocation` model: object references instead of type indices

- `dynaplex.models.resource_allocation` now points at its static data instead of
  indexing it: `Resource.resource_type` is a `ResourceType`, `Job.tasks` is a
  `list[TaskType]`, an `Assignment` holds the `ResourceOption` that created it
  (`option`, was `action_index`), and a `TaskType` holds its successor (`next_task`, or
  `None`). On the model, `resource_types` and `task_types` replace the parallel lists
  `resources_per_type`, `task_has_arrivals`, `interarrival_samplers`, `next_task` and
  `next_probability`. Trajectories are unchanged, bit for bit.

### A const list or array can no longer share storage with a modifiable one

- Compiled code never turns a modifiable list or ndarray into a const one (`s.view =
  s.hist` is a type error). A hand-written `__init__` could: handing ONE Python list or
  ndarray to a `Const()` field and to a non-const field made them one container in the
  engine, so a write through the non-const field changed the const one. The engine now
  refuses this when it takes the object in, naming the field: "the same Python list is
  held by a const field and by a non-const field". Give one of the two fields its own
  copy. One list in two non-const fields, or in two const fields, is unchanged.

### Apple silicon wheels need macOS 15 or newer

- GitHub retired the macOS 14 build runner on 2 November 2026, and the Homebrew LLVM
  the JIT is built against has no macOS 14 bottle any more, so the arm64 wheels of 2.0
  are built on macOS 15 and carry a `macosx_15_0` floor. On macOS 14 (Sonoma) `pip
  install dynaplex` keeps resolving to 1.14.1, whose arm64 wheels run on 14.0. The Intel
  Mac floor stays at 15.0.

### Apple silicon: every performance-class core tier counts (issue #11)

- An Apple M5 Pro reports its 15 cores as two tiers, `Super` (5) and `Performance` (10),
  and no efficiency tier. `dynaplex.machine.detect()` read the first tier as the
  performance cores and the second as the efficiency cores, so `machine="auto"` gave
  that chip 5 performance cores: a generation-0 shape with 3 workers and no
  two-engine `mps` shape, four times slower than the laptop rule over all cores at
  100 000 samples (the reporter's measurement). Every tier that is not the efficiency
  tier now counts as performance cores: 13 workers in generation 0 and two engines
  with 6 workers each on `mps` from generation 1. `python -m dynaplex.machine` prints
  the tiers by the names macOS gives them (`5 super + 10 performance cores`), and
  `Machine.core_tiers` carries them. Chips that name their tiers `Performance` and
  `Efficiency` detect as before.

### `python -m dynaplex.selftest` points a failed run at the right issue tracker

- The link printed after a failed self-test is now
  `https://github.com/DynaPlex/DynaPlex/issues`, where issues are filed; it used to
  name the documentation repository.

### `PolicyComparer` frees the garbage of models that allocate every period

- A model that creates a small array or distribution in every period, for example
  `np.zeros(16)`, `DiscreteDist.constant(d)` or `dist.add(other)` in
  `modify_state_with_event`, made `PolicyComparer` use more memory the more
  trajectories it ran and, for heuristic policies, the longer the horizon (issue #13).
  There were two causes.
    - The automatic sweep is triggered by the bytes allocated since the last sweep, and
      the data of arrays made by the numpy creators and of distributions was not
      included in that count, so the sweep was almost never triggered. These
      allocations are now counted, in every engine, not only in the comparer.
    - A heuristic policy was only given a chance to sweep between trajectories, so
      one long trajectory kept everything its periods had allocated until it ended.
      The comparer now also checks every 256 steps inside a trajectory.
- Results are unchanged. Memory now levels off at roughly 16 to 35 MiB of garbage per
  worker, whatever the number of trajectories and the horizon. The extra check costs
  a few percent on a model whose transitions do almost nothing and is not measurable
  otherwise.
- A `dynaplex.modelling.gc()` call added to a model or policy to work around this is
  no longer needed and can be removed.
- An object of a class without fields was not counted either, like the empty list
  below: a model that created such objects and nothing else was never swept. Every
  object now adds at least one word.

### DCL from generation 1: no walk over all objects in every round

- Every call that brought a numpy array into an engine was followed by a full walk
  over the objects the engine holds. From generation 1 on DCL makes such a call in
  every round, to hand over the actions of the network. On a GPU node this walk was
  about a quarter of the time of each process (measured on a full run on an H100
  node). The walk now happens only when enough has been allocated since the last
  one, like every other automatic sweep. Samples are unchanged.

### An empty list counts toward the garbage-collection threshold

- A model whose only per-period allocation was an empty list literal, such as
  `draws: list[float] = []` that stays empty on some instances, grew memory without
  bound during DCL sample collection (issue #12). The automatic sweep is triggered by
  the bytes allocated since the last sweep, and an empty list reserves no element
  storage, so it added nothing to that count: the lists were unreachable and
  collectable (`gc_every=1` reclaimed them), but no sweep was ever triggered, and
  their bookkeeping slots piled up at about 6 MiB per sample. Every list now adds a
  small fixed cost on creation, in the interpreter and the JIT alike, so a stream of
  empty lists trips the sweep like any other allocation. Lists with elements were
  already counted and are unchanged.

### Deep reads no longer break the read-only promises

- A `@policy`, featurizer or `write_action_validity` that only *reads* deep into
  the state, such as `state.stock_points[k].open_orders.front()` (state, a list of
  objects, a `FifoQueue`, its items), was rejected in 1.14 with
  `... is modified (escape depth ...) — breaks the policy promise 'state-read-only'`.
  The access analysis stopped following a field path after four steps and recorded
  anything deeper as "unknown", which the read-only promises treated as a write. A
  cut-off access now keeps its kind (a read stays a read, a write is still reported),
  and the limit is eight steps. Writes through such a path are rejected as before.

### Promise checks in fast mode see list element writes

- An engine built with `checks=False` lowers `xs[i]` and `xs[i] = v` to unchecked
  opcodes, and the access analysis behind the role promises (`state-read-only` and
  friends) did not recognise them: a `write_action_validity` or policy that assigned
  into a state list was accepted in fast mode and rejected in checked mode. The
  analysis now treats both spellings alike, so a promise violation is reported
  independently of the mode.

### JIT inspection knobs are development-build only

- `DYNAPLEX_JIT_DUMP_IR` and `DYNAPLEX_JIT_INLINE_THRESHOLD` are now ignored by
  installed wheels, like `DYNAPLEX_JIT_PERFMAP` already was: a wheel exposes neither
  the LLVM IR nor the addresses of the code it compiles, and takes no tuning
  overrides. All three remain available in a source build configured with
  `DYNAPLEX_PROFILING=ON`.

### `for x in a` over a one-dimensional ndarray

- `for x in a:` and `for i, x in enumerate(a):` now compile when `a` is a rank-1
  ndarray; `x` has the scalar type of the dtype. In 1.14 every array walk had to
  be an index loop (`For loops over lists require a list expression`). The loop
  lowers like the list form (length read once, element read per iteration);
  arrays of rank two or more are still rejected, with a message pointing at an
  index loop over the leading axis.

### Checked mode: view assignment is shape-checked; out-of-range errors name the index

- `g[i] = row` with a `row` of the wrong length is now an error in checked mode
  (the default for `Engine` and `Program`, and the rehearsal pass of DCL and the
  comparer). In 1.14 the copy silently ran past the row or past the value. Fast
  mode (`checks=False`) is unchanged: out of range is undefined behaviour there.
- Out-of-range list and ndarray accesses in checked mode now say what went wrong
  instead of naming an opcode, identically on the interpreter and the JIT:

      list index -5 out of range for a list of length 4
      list assignment index 5 out of range for a list of length 4
      ndarray index 9 is out of bounds for axis 1 with size 3
      ndarray view assignment: the value has size 2 along its axis 0 where the
        target view has size 3 (shapes must match exactly; DynaML does not broadcast)

  The index shown is the one the model used, before Python's negative wrap, as
  CPython and numpy report it. Code that matched on the old texts
  (`LIST_GET: list index out of range`, `ARRAY: ndarray index out of range`) must
  be updated.

### DCL trains on soft targets by default

- `train_network(loss=...)` and the DCL `train=` default changed from `"ce"` to
  `"soft_ce"`: the label mass goes to the final-round survivors of the rollout
  tournament, each weighted by `exp(-gap/tau)` with `tau` the median positive gap
  over the sample set, instead of a one-hot on the winner. On models whose decision
  consequences lie far out and whose rollout gaps are noise-sized (the spare-parts
  tutorial), hard labels trained a first-generation agent worse than its rollout
  policy and more rollouts made it worse; soft targets on the same samples beat the
  rollout policy everywhere and tied the best heuristic. `"ce"` and `"count_ce"`
  remain selectable. **Behaviour-changing** for every DCL run that did not set `loss`.

### Featurizer probe: 1000 events and a knob

- The probe state (the initial state walked to its first decision, used by the
  synthesized `spec()`, `check_mdp`, `rehearse` and DCL) is found within 1000
  events instead of 100, and `@featurizer(probe_steps=N)` raises the limit for one
  featurizer. The error for a model whose first decision lies further out names the
  knob and the alternative (an initial state closer to a decision) and says that the
  walk is seeded, so the failure is a property of the model.

### `rng.permutation(n)` and `rng.shuffle(xs)`

- Both generator families gain `permutation(n)`, a uniformly random order of
  `0..n-1` as a `list[int]`, and `shuffle(xs)`, which reorders a list of any
  element type in place. Policies that visit components in a random order
  (`for i in rng.permutation(n): ...`) now compile. In CPython the native
  `dynaplex.Generator` runs the same kernel, so plain and compiled runs agree
  bit-for-bit, as for `choice`. Both are DynaPlex's own stream: the NumPy family
  does not reproduce NumPy's `permutation` for the same seed, exactly as
  `choice(n)` does not. Shuffling an array in place is not offered.

### `DiscreteDist.custom` from a list in compiled code

- `DiscreteDist.custom(probs, offset)` compiles with `probs` a `list[float]`, not
  only a float64 array; the list is read in place, without a conversion to an array
  first. An integer list is a compile error that points at `DiscreteDist.constant`,
  since an integer PMF can only be a point mass. A state that holds a PMF estimate of varying length
  as a list is copied in place by `clone_into`, where an array of varying length is
  reallocated on every copy; on a lost-sales model with such a field this made
  generation-0 collection 1.5x faster. An empty list literal `[]` is a compile error
  (assign it to a typed variable first); a typed empty list is rejected by the PMF
  check like any other invalid PMF.

### Conditions on an int; `while True:`; unreachable code

- `if x:`, `elif x:`, `while x:`, `not x`, `1 if x else 0` and `x and y` on an `int`
  compile and mean `x != 0`, as the language reference always promised. 1.14 passed
  the int straight to the backend, which rejected it with
  `JUMP_IF_FALSE: condition temp must be Bool`; floats were already fine.
- A function that returns from inside a `while True:` (or `while 1:`) loop and never
  breaks is no longer rejected with "control can reach the end without returning".
  A loop that can `break` still needs a return after it.
- A statement that can never run (after a `return`, `break` or `continue`, after an
  `if`/`else` whose branches all return, or after a `while True:` without a break,
  including that loop's `else`) is now a clear compile error, `Unreachable code: ...`.
  Before, such a statement in a function with a return value failed deep in the
  backend with `FindNextNonEmptyBlock: no non-empty block found`.

### A wrong `Engine(...)` call is an error, not a crash

- `Engine(model, ...)` with a model *instance* where the root class goes now raises
  `TypeError: Engine takes the root CLASS and its constructor arguments ... write
  Engine(MyMDP, <constructor args>)`. 1.14 let the instance through to class
  discovery, where the development build raised an unhelpful `unhashable type` and
  the cp314 macOS wheel crashed. `python -m dynaplex.selftest` now probes this path
  on the installed wheel.
- The crash itself is fixed as well: it was a code-generation bug in Nuitka (the
  compiler the wheels use for the front end) that emits `dict.fromkeys(...)` of a
  runtime iterable without an error exit, so an unhashable element crashed instead of
  raising. Class discovery no longer uses `dict.fromkeys` and rejects a non-class root
  before anything hashes it, so the path raises a `TypeError` on every wheel, not only
  behind the `Engine` check.

### Machine notes only where they mean something

- The `machine:` block that `DCL.run()` prints no longer ends with the two
  provenance notes (where the allowed-CPU count came from) on a laptop; they print
  only on a machine that looks like a cluster node (Linux under a scheduler, with a
  cgroup or memory limit, with an allocation smaller than the hardware, or with 64
  or more CPUs). Notes that carry advice, such as a failed GPU probe or an affinity
  mismatch, print everywhere. All notes are still collected in `Machine.notes` and
  the run manifest.

## 1.14.1

Patch release for 1.14.0.

### numpy 2.2.x: `import dynaplex` failed

- With numpy 2.2.x installed, `import dynaplex` raised
  `TypeError: TrajectoryContext.valid: ... is not a DynaML type (Tuple/shape-based
  numpy.ndarray annotations are not supported ...)`. numpy 2.0, 2.1, 2.3 and 2.4 were
  unaffected ([issue #10](https://github.com/DynaPlex/DynaPlex/issues/10)).
- Cause: the annotation parser accepted `numpy.typing.NDArray[dtype]` only when numpy's
  internal shape alias was spelled `tuple[Any, ...]`; numpy 2.2.x alone spells it
  `tuple[int, ...]`, so the library's own array-typed fields were rejected as explicit
  tuple shapes before any user code ran. Any variadic `tuple[X, ...]` is now accepted as
  the alias. Explicit fixed shapes such as `tuple[int, int]` are still rejected: declare
  the rank with `Annotated[NDArray[dtype], Rank(n)]`.
- The release smoke matrix can now pin the numpy the wheel suite runs against, so each
  supported platform is checked against numpy 2.2 as well as the latest release.

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
