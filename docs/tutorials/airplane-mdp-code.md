# Airplane ticket selling MDP — Python code

You can download a complete Python implementation of this MDP example:
[airplane_mdp_example.py](../downloads/airplane_mdp_example.py).

The file contains, in order: the `State` and MDP classes with the transition
functions, a rule-based benchmark policy, a *featurizer* (the class that
defines the state representation used as neural-network input — declared
separately from the MDP itself), and driver code that first walks through a
single episode step by step (useful for validating a model), then evaluates
the policy over many episodes with the `PolicyComparer`, and optionally
trains a PPO agent and compares it against the rule-based policy on common
random numbers.

Below is the full code for reference:

```python title="airplane_mdp_example.py" linenums="1"
--8<-- "downloads/airplane_mdp_example.py"
```
