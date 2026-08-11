<div class="dp-hero" markdown>
![DynaPlex logo](assets/images/logo.png){ width="420" }
</div>

DynaPlex is an optimization library for solving Markov Decision Processes and
related sequential decision making problems (POMDP, HMM). 

The DynaPlex design enables clean modelling and efficient solving of problems arising in operations management (OM) and related fields, e.g. in supply chain management, transportation, manufacturing, warehousing, maintenance optimization, process optimization, etc. Models are written in
**DynaML**: a purpose-built modelling language whose syntax is canonical Python, extended with modelling primitives such as discrete distributions. Models are executed by a multi-threaded engine with a bundled LLVM JIT, allowing the user to read and write canonical Python code, and get auto-vectorized C++ speed. 

Just as importantly, the library bundles algorithms such as [Deep Controlled Learning (DCL)](training/dcl.md), specifically designed for the highly stochastic problems that arise in typical OM applications, as well as canonical implementations of DRL algorithms such as [Proximal Policy Optimization (PPO)](training/ppo.md), which benefit from accelerated training due to the compiled vectorized environments. There is also first-class support for [comparing](training/policy-comparison.md) and optimizing classical parameterized policies, as well as [planned](community/roadmap.md) support for exact algorithms. 

```bash
pip install dynaplex
```

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
