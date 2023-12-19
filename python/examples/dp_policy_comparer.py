import os
import sys

parent_directory = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(parent_directory)
# noinspection PyUnresolvedReferences
from dp.loader import DynaPlex as dp
sys.path.remove(parent_directory)

mdp = dp.get_mdp(id="lost_sales",
                 p=9.0, h=1.0,
                 leadtime=3,
                 demand_dist={"type": "poisson", "mean": 3.0})


base_policy = mdp.get_policy("base_stock")
dcl = dp.get_dcl(mdp, None, M=500, num_gens=1)
dcl.train_policy()
trained_policies = dcl.get_policies()
comparer = dp.get_comparer(mdp, number_of_trajectories=100, rng_seed=12)

comparison = comparer.compare(trained_policies)
result = [(item['mean']) for item in comparison]
print(result)
