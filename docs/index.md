<div class="dp-hero" markdown>
![DynaPlex logo](assets/images/logo.png){ width="420" }

<p class="dp-tagline">One intuitive modelling interface.<br>Three ways to solve.</p>

<p class="dp-sub">Write a Markov decision process once, in <strong>DynaML</strong> — canonical
Python, compiled to vectorized C++ speed using LLVM JIT — and deploy and compare a range of methods.</p>
</div>

<div class="grid cards dp-features" markdown>

-   :material-pencil-ruler:{ .lg .middle } **Intuitive & correct MDP models**

    ---

    Write the state, the random events that move it, the actions that shape
    it and the costs they incur in plain Python that mirrors the MDP
    definition — and is checked against it. One model feeds all three
    methods.

    [:octicons-arrow-right-24: Modelling an MDP](getting-started/introduction-to-mdps.md)

-   :material-rocket-launch:{ .lg .middle } **Vectorized reinforcement learning**

    ---

    Compiled, vectorized environments drive
    [Deep Controlled Learning](training/dcl.md), designed for the highly
    stochastic problems of operations management, and DRL algorithms ([PPO](training/ppo.md), etc) through a vectorized
    [gym interface](training/gym-environments.md).

    [:octicons-arrow-right-24: Deep Controlled Learning](training/dcl.md)

-   :material-scale-balance:{ .lg .middle } **Compare & optimize policies**

    ---

    Evaluate heuristics and parameterized policies against each other on
    common random numbers, with statistical error bars, and optimize their
    parameters — all at compiled speed.

    [:octicons-arrow-right-24: Comparing policies](training/policy-comparison.md)

-   :material-function-variant:{ .lg .middle } **Exact dynamic programming**

    ---

    Where the state space can be enumerated and events are discrete: exact optimal policies, for finite and infinite horizons — a powerfull yardstick to benchmark heuristic and trained agents.

    [:octicons-arrow-right-24: Exact solver](training/exact-solver.md)

</div>

```bash
pip install dynaplex
```

DynaPlex is an optimization library for formulating and solving Markov Decision
Processes, i.e. for identifying well-performing policies that select actions
based on complete or partial observations of the state space. Its design
enables clean modelling and efficient solving of problems arising in operations
management (OM) and related fields — supply chain management, transportation,
manufacturing, warehousing, maintenance and process optimization. Models are
written in **DynaML**, a purpose-built modelling language whose syntax is
canonical Python, extended with modelling primitives such as discrete
distributions, and executed by a multi-threaded engine with a bundled LLVM JIT:
you read and write plain Python, and get auto-vectorized C++ speed.

!!! tip "New to MDPs?"
    Start with the [introduction to MDPs](getting-started/introduction-to-mdps.md),
    then work through the step-by-step tutorial, beginning with the
    [airplane ticket selling MDP](tutorials/airplane-mdp.md).

## Where to go next

<div class="grid cards" markdown>

-   :material-download:{ .lg .middle } **Getting started**

    ---

    Install the package and verify that it works on your platform, JIT
    included.

    [:octicons-arrow-right-24: Installation](getting-started/installation.md)

-   :material-school:{ .lg .middle } **Tutorials**

    ---

    Two worked examples — a finite-horizon
    [ticket selling problem](tutorials/airplane-mdp.md) and an
    infinite-horizon [bin packing problem](tutorials/binpacking-mdp.md) —
    with complete, runnable code.

-   :material-book-open-variant:{ .lg .middle } **Language reference**

    ---

    The complete reference for DynaML, the Python subset in which DynaPlex
    models are written — including all built-in functions.

    [:octicons-arrow-right-24: Language reference](reference/language-reference.md)

-   :material-account-group:{ .lg .middle } **Community**

    ---

    [Questions and discussion](https://github.com/DynaPlex/DynaPlex/discussions),
    [bug reports and feature requests](https://github.com/DynaPlex/DynaPlex/issues).

    [:octicons-arrow-right-24: Getting help](community/getting-help.md)

</div>
