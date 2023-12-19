from dp import dynaplex

gridsize = 6
n_pickers = 2
max_orders_per_event = 1

mdp = dynaplex.get_mdp(id="order_picking",
                       grid_size=gridsize,
                       n_pickers=n_pickers,
                       max_orders_per_event=max_orders_per_event)


def dcl_policy(gen):
    dcl_filename = f'dcl_python_{gen}'
    dcl_load_path = dynaplex.filepath(mdp.identifier(), "dcl_python", dcl_filename)
    return dynaplex.load_policy(mdp, dcl_load_path)


def ppo_policy():
    ppo_filename = 'ppo'
    ppo_load_path = dynaplex.filepath(mdp.identifier(), "ppo", ppo_filename)
    return dynaplex.load_policy(mdp, ppo_load_path)


base_policy = mdp.get_policy("random")
heur_gd = mdp.get_policy(id="greedy_heuristic", coordinated=False, cost_based=False)
heur_cd = mdp.get_policy(id="greedy_heuristic", coordinated=True, cost_based=False)
heur_gc = mdp.get_policy(id="greedy_heuristic", coordinated=False, cost_based=True)
heur_cc = mdp.get_policy(id="greedy_heuristic", coordinated=True, cost_based=True)

policies = [base_policy, heur_gd, heur_cd, heur_gc, heur_cc, ppo_policy(), dcl_policy(1)]

comparer = dynaplex.get_comparer(mdp, number_of_trajectories=100, periods_per_trajectory=100, warmup_periods=0)
comparison = comparer.compare(policies)
result = [(item['policy']['id'], item['mean'], item['error']) for item in comparison]

print(result)
