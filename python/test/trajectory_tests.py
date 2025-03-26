import os
import sys
import numpy as np
import torch
import pytest
from dp import dynaplex


@pytest.fixture
def mdp():
    return dynaplex.get_mdp(id="lost_sales", p=9.0, h=1.0, leadtime=3, demand_dist={"type": "poisson", "mean": 3.0})


def test_rollout(mdp):
    other_traj = dynaplex.get_trajectory(123)
    other_mdp = dynaplex.get_mdp(id="lost_sales", p=19.0, h=1.0, leadtime=3,
                                 demand_dist={"type": "poisson", "mean": 3.0})
    traj = dynaplex.get_trajectory(12)
    traj.seed_rngprovider(123)
    mdp.initiate_state(traj)

    policy = other_mdp.get_policy("random")

    other_traj.seed_rngprovider(100)
    other_mdp.initiate_state(other_traj, traj.state_as_dict())
    other_mdp.run_until_period_count(policy, other_traj, other_traj.period_count+10)
    print()
    print(other_traj.cumulative_return)
    print(other_traj.period_count)
    #this is a much smaller dictionary , that can get you the current agent who is acting:
    print(mdp.get_state_category(other_traj))
    other_mdp.run_until_period_count(policy, other_traj, other_traj.period_count+10)
    print(other_traj.cumulative_return)
    print(other_traj.period_count)


def test_reset(mdp):
    traj = dynaplex.get_trajectory(12)
    #shallow copy:
    other = traj
    traj.next_action = 100
    assert other.next_action == 100, "next_action should be 100"
    assert other.external_index == 12, "The external_index should be set to 12"
    additional_traj = dynaplex.get_trajectory()
    traj.seed_rngprovider(123)
    mdp.initiate_state(traj)
    #copies the state, but reinitiates all "counters" and running averages:
    mdp.initiate_state(additional_traj,traj)
    deepcopy = mdp.deep_copy_and_reinitiate(additional_traj)

    other_traj = dynaplex.get_trajectory(123)
    other_mdp = dynaplex.get_mdp(id="lost_sales", p=19.0, h=1.0, leadtime=3, demand_dist={"type": "poisson", "mean": 3.0})


    # this won't work, as traj contains a c++ native state created by mdp, that cannot be loaded
    # in a different mdp.
   # other_mdp.initiate_state(other_traj,traj)
    # Here, we pay the price of converting the state first to a vargroup/dict
    #after which the other_mdp can interpret it as a state native to that mdp.
    #this will be relatively slower, and should only be used as a workaround when traj is created using a _different_ mdp
    #compared to other_mdp. 
    other_mdp.initiate_state(other_traj, traj.state_as_dict())



    print()
    print(other_traj.state_as_dict())
    print(traj.state_as_dict())
    print(additional_traj.state_as_dict())
    print(deepcopy.state_as_dict())
    max_period_count = 10
    policy = mdp.get_policy("random")

    while traj.period_count < max_period_count:
        mdp.incorporate_until_nontrivial_action(traj,max_period_count)
        mdp.incorporate_action(traj,policy)
        #print(traj.state_as_dict())
    while traj.period_count < 2*max_period_count:
        mdp.incorporate_until_nontrivial_action(traj,max_period_count*2)
        policy.set_action(traj)
        
        features = torch.from_numpy(mdp.get_features(traj))
        print(features)
        print(torch.from_numpy(mdp.get_mask(traj)))
        #note that features is a numpy array
        print(type(features))
        print(traj.state_as_dict())

        #OR simply:
        #traj.next_action = 0
        mdp.incorporate_action(traj)




