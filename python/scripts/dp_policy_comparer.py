from dp import dynaplex

mdp = dynaplex.get_mdp(id="lost_sales",
                       p=9.0, h=1.0,
                       leadtime=3,
                       demand_dist={"type": "poisson", "mean": 3.0})


base_policy = mdp.get_policy("base_stock")
dcl = dynaplex.get_dcl(mdp, None, M=500, num_gens=1)
dcl.train_policy()
trained_policies = dcl.get_policies()
comparer = dynaplex.get_comparer(mdp, number_of_trajectories=100, rng_seed=12)

comparison = comparer.compare(trained_policies)
result = [(item['mean']) for item in comparison]
print(result)
