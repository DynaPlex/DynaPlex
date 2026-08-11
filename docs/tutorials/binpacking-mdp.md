# Bin packing MDP formulation

The bin packing problem is an example of an infinite horizon MDP. Weights
arrive sequentially and must be assigned to one of several bins. Each bin has
a maximum capacity (`max_bin_size`). When a bin's total weight exceeds this
capacity, the overflow weight incurs a cost, and the bin is emptied. The goal
is to minimize the total overflow cost over an infinite horizon.

In this problem, weights are revealed one by one according to a probability
distribution. Each weight must be immediately assigned to one of the
available bins. When a bin exceeds its maximum capacity, any overflow weight
incurs a cost proportional to the overflow amount, and the bin is reset to
empty. The process continues indefinitely.

!!! note
    This is an **infinite horizon** MDP, meaning the process continues
    indefinitely rather than terminating after a fixed number of steps. This
    contrasts with finite horizon MDPs like the
    [airplane MDP](airplane-mdp.md), which has a fixed termination time
    (day 26).

    In infinite horizon MDPs, it is especially important to explicitly track
    time progression. DynaPlex requires you to manually update
    `context.time_elapsed` to track the passage of time. In this bin packing
    problem, time progresses by one unit each time a new random weight
    arrives (in the `modify_state_with_event` function).

## The components of the MDP

1. **States (S):** The state consists of:
    - `weight_vector`: a list holding the current weight in each bin
    - `upcoming_weight`: the weight that has just arrived and must be
      assigned
    - `category`: the state category (`AWAIT_EVENT` or `AWAIT_ACTION`)

2. **Actions (A):** The action is to select which bin (0 to
   `number_of_bins - 1`) to assign the upcoming weight to. All bins are
   always valid actions. (This is unlike the
   [airplane ticket selling MDP](airplane-mdp.md), where the action space
   was constrained.)

3. **Randomness / events:** A new item arrives that will need to be added to
   a bin. The weight of the item is sampled from a distribution — in the
   implementation, a `DiscreteDist` held by the MDP, drawn via an
   `AliasSampler` for fast repeated sampling.

4. **Transitions:**
    - When a new weight arrives (event), it is sampled from the weight
      distribution and stored in `upcoming_weight`. The state transitions to
      `AWAIT_ACTION`, and `context.time_elapsed` is incremented.
    - When an action is taken, the weight is added to the selected bin. If
      the bin overflows, the overflow cost is added to
      `context.cumulative_cost`, and the bin is reset to empty. The state
      transitions back to `AWAIT_EVENT`.

5. **Costs (C):** The cost is the overflow amount when a bin exceeds its
   capacity.

!!! note "Time tracking in infinite horizon MDPs"
    Unlike finite horizon MDPs where time is naturally bounded, infinite
    horizon MDPs require explicit time tracking. In DynaPlex, you must
    manually increment `context.time_elapsed` to track the progression of
    time. This is essential for:

    - algorithms that need to know how many steps have elapsed;
    - stopping criteria for training or evaluation.

    In this bin packing problem, time advances by one unit each time a new
    weight arrives, which happens in the `modify_state_with_event` function:
    `context.time_elapsed += 1`.

## Policy

This MDP example includes two simple heuristic policies:

1. **LowestWeightPolicy:** always assigns the incoming weight to the bin
   with the lowest current weight. This is a greedy strategy that tries to
   balance the load across bins.

2. **FirstFitPolicy:** assigns the weight to the first bin that can
   accommodate it without overflow. If no bin can accommodate it without
   overflow, it assigns to the first bin. This is a common heuristic in bin
   packing problems.

## Python code

You can download a complete Python implementation of this MDP example:
[binpacking.py](../downloads/binpacking.py).

Below is the full code for reference; note that other files could import from
this and use the MDP, e.g. for training policies:

```python title="binpacking.py" linenums="1"
--8<-- "downloads/binpacking.py"
```

## Training a DCL agent

With the model and the two heuristic policies in place, we can train a neural
network policy with [Deep Controlled Learning](../training/dcl.md) and see
whether it beats the heuristics. Download
[binpacking_dcl.py](../downloads/binpacking_dcl.py), place it next to
`binpacking.py`, and run it:

```python title="binpacking_dcl.py" linenums="1"
--8<-- "downloads/binpacking_dcl.py"
```

The script calls `assert_mdp` and `assert_policy_for_mdp` — no-ops at
runtime that make `pyright binpacking_dcl.py` statically verify that the MDP
and policies satisfy the interfaces DynaPlex expects (see the
[airplane tutorial](airplane-mdp.md) for details).

The script trains three DCL generations, starting from `LowestWeightPolicy`
as the generation-0 rollout policy, and then evaluates everything with the
[`PolicyComparer`](../training/policy-comparison.md) on common random
numbers.

!!! warning "Unexploited symmetry may hamper performance"
    This model is deliberately kept simple, and that leaves performance on
    the table. The bins are interchangeable — any permutation of
    `weight_vector` describes an equivalent state — but the model does not
    sort the weights after adding each weight, and the featurizer feeds the
    raw vector to a plain `MLP`. Equivalent states therefore look different
    to the network, which must spend training data and capacity learning the
    symmetry instead of having it built in. A canonical state — e.g. keeping
    `weight_vector` sorted after every assignment, so that action `i` means
    "assign to the i-th fullest bin" — exploits the symmetry and typically
    learns faster and reaches better policies. Network architectures that
    are permutation-invariant by construction are on the
    [roadmap](../community/roadmap.md#beyond-mlp-neural-networks-in-dcl). It takes a few minutes on a laptop; training artifacts land in
`dynaplex_runs/`, and rerunning the script resumes from what is already
there instead of recomputing.

On our machine the comparison comes out as follows (network training is
seeded but follows torch's determinism caveats, so your numbers may differ
slightly — the ordering should not):

```text
policy                  mean       error    delta_mean  delta_error
LowestWeight *        2.6381     0.01583             0            0
FirstFit              2.6298     0.01543       -0.0083     0.004839
DCL_gen1              2.5504      0.0164       -0.0877      0.02135
DCL_gen2              2.3791     0.01584        -0.259      0.01681
DCL_gen3              2.0206     0.01277       -0.6175      0.01875
(* = benchmark; delta = policy - benchmark, paired on common random numbers)
```

The two heuristics are nearly tied at about 2.63 overflow cost per period.
Each DCL generation improves on the previous one, and the generation-3 agent
reaches 2.02 — beating the heuristics by roughly 23%, with the paired delta
column showing the improvement is far outside the noise.
