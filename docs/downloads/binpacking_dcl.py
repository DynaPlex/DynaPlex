"""
Training a DCL agent on the bin packing MDP, and comparing it against the
FirstFit and LowestWeight heuristics on common random numbers.

Place this file next to binpacking.py (the model file from the tutorial) and
run it directly. Artifacts (samples, trained agents) land in dynaplex_runs/;
rerunning resumes rather than recomputes.
"""
import numpy as np

import dynaplex as dp
from dynaplex.modelling import DiscreteDist, assert_mdp, assert_policy_for_mdp

try:
    # If you author your own models, you can import them from another file 
    # as you would any other Python module:
    from binpacking import (  # pyright: ignore[reportMissingImports]  # local-file form; falls back to dynaplex.models below
        BinPackingMDP,
        BinPackingFeaturizer,
        FirstFitPolicy,
        LowestWeightPolicy,
    )
except ImportError:
    # Since binpacking is also one of the built-in models that ships with 
    # DynaPlex, we can also import it directly from the dynaplex.models.binpacking module.. 
    from dynaplex.models.binpacking import (
        BinPackingMDP,
        BinPackingFeaturizer,
        FirstFitPolicy,
        LowestWeightPolicy,
    )


def main() -> None:
    # Weights 10/20/30/40/50: a PMF over 10..50 with zero-probability gaps.
    weight_probs = np.zeros(41)
    weight_probs[[0, 10, 20, 30, 40]] = [0.2, 0.3, 0.25, 0.15, 0.1]
    mdp = BinPackingMDP(
        max_bin_size=100,
        number_of_bins=3,
        weight_dist=DiscreteDist.custom(weight_probs, offset=10),
    )

    lowest_weight = LowestWeightPolicy(mdp=mdp)
    first_fit = FirstFitPolicy(mdp=mdp)

    # No-ops at runtime; they make pyright statically verify that the MDP and
    # policies satisfy the interfaces DynaPlex expects.
    assert_mdp(mdp)
    assert_policy_for_mdp(mdp, lowest_weight)
    assert_policy_for_mdp(mdp, first_fit)

    d = dp.DCL(
        mdp, lowest_weight,                # generation-0 rollout policy
        features=BinPackingFeaturizer,
        n=8000,                            # labeled samples per generation
        m=200,                             # rollouts per candidate action
        h=100,                             # rollout horizon (periods)
        # NOTE: a plain MLP over the raw (unsorted) weight vector ignores the
        # symmetry between bins — see the warning in the tutorial.
        network=dp.MLP(hidden=[128, 128]),
        train=dict(loss="ce", epochs=50, batch_size=64, lr=1e-3,
                   patience=10, val_fraction=0.1),
    )
    agents = d.run(generations=3)

    # Evaluate all policies on common random numbers: each sees the same
    # weight streams, so the delta columns are paired differences.
    comparer = dp.PolicyComparer(
        mdp,
        number_of_trajectories=100,
        warmup_time=100,
        horizon=1000,
        seed=0,
    )
    to_compare = {
        "LowestWeight": lowest_weight,
        "FirstFit": first_fit,
    }
    for agent in agents:
        to_compare[f"DCL_gen{agent.info['generation']}"] = agent
    print(comparer.compare(to_compare))


if __name__ == "__main__":
    main()
