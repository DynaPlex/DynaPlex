<div class="dp-hero" markdown>
![DynaPlex logo](assets/images/logo.png){ width="420" }
</div>

DynaPlex is an optimization library for solving Markov Decision Problems (MDPs). MDP models in DynaPlex are written in DynaML/Python and compiled into C++20 & LLVM kernels: you read and write canonical Python code, and get auto-vectorized C++ speed — greatly reducing simulation, computation and training times. DynaPlex supports deep reinforcement learning and classical parameterized policies; exact methods based on policy and value iteration are [planned](community/roadmap.md). 
DynaPlex focuses on solving problems arising in Operations Management: Supply Chain, Transportation and Logistics, Manufacturing, etc.

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

    Two worked examples — a finite-horizon ticket selling problem and an
    infinite-horizon bin packing problem — with complete, runnable code.

    [:octicons-arrow-right-24: Airplane MDP](tutorials/airplane-mdp.md)

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
