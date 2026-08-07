# A brief introduction to MDPs

Welcome to the world of Markov Decision Processes (MDPs)! MDPs are a
fundamental concept in the field of artificial intelligence and reinforcement
learning. They are widely used to model decision-making problems where an
agent interacts with an environment to maximize some notion of cumulative
reward.

## What is an MDP?

A Markov Decision Process (MDP) is a mathematical framework used to model
decision-making problems in which an agent interacts with an environment over
a series of discrete time steps. At each time step, the agent observes the
current state of the environment and selects an action. The environment
responds by transitioning to a new state and providing a reward signal to the
agent. Importantly, the transition to the next state and the reward received
only depend on the current state and the action taken, satisfying the Markov
property.

## Key components of an MDP

1. **States (S):** The set of all possible situations or configurations in
   which the agent can find itself.

2. **Actions (A):** The set of all possible moves or decisions that the agent
   can make in a given state.

3. **Randomness / events:** In certain situations, the next state and reward
   do not depend deterministically on the state and the action, but are
   stochastic. In DynaPlex, the random events or stochastic transitions are
   represented by random variables that are explicitly sampled from some
   distribution.

4. **Transitions:** Transition functions specify how the state changes in
   response to actions and randomness/events.

5. **Costs:** The numerical values associated with state-action pairs,
   indicating the immediate cost of taking a particular action in a specific
   state. Note that a reward is a negative cost.

A key related concept is that of a policy, which specifies the action to take
in each state:

- **Policy (π):** A strategy that specifies which action to take in each
  state.

DynaPlex builds on the MDP-EI (MDP with exogenous inputs) framework, which is
illustrated below. Here, \(s_t\) represents the state at time \(t\), \(\pi\)
represents the policy, \(a_t\) the decision, and \(c_t\) the costs. We further
denote a random event by \(\xi_t\) and the transition function is \(f\).
For more information, we refer to:
[ArXiv paper](https://arxiv.org/abs/2011.15122),
[A unified framework for stochastic optimization](https://doi.org/10.1016/j.ejor.2018.07.014).
See also the published version of the Deep Controlled Learning paper:
[published version](https://www.sciencedirect.com/science/article/pii/S0377221725000463).

![MDP-EI illustration](../assets/images/mdpei.png)

## MDP models in DynaPlex

DynaPlex models are defined as an MDP dataclass. This dataclass has specific
methods which are required, and which manipulate objects of another
user-defined class that represents the state of the problem. The formalization
makes an explicit distinction between States that are awaiting an action
(pre-action state) and States that are awaiting an event (post-action state);
cf. "Reinforcement Learning and Stochastic Optimization" by Warren B. Powell,
Section 3.3.

We provide examples/tutorials in the form of the
[Airplane Ticket Selling MDP](../tutorials/airplane-mdp.md) and the
[Bin Packing MDP](../tutorials/binpacking-mdp.md). The Python code for these
MDPs is available on the
[Airplane MDP Python code](../tutorials/airplane-mdp-code.md) and
[Bin Packing MDP Python code](../tutorials/binpacking-mdp-code.md) pages.

Classes and methods in DynaPlex MDPs must adhere to certain structural and
semantic properties. For more information, see the
[language reference](../reference/language-reference.md).

## Recommended reference books

For further exploration of Markov Decision Processes, we recommend the
following books:

1. **"Markov Decision Processes: Discrete Stochastic Dynamic Programming"**
   by Martin L. Puterman — a thorough treatment of MDPs, including dynamic
   programming methods and their applications.

2. **"Dynamic Programming and Optimal Control"** by Dimitri P. Bertsekas — a
   comprehensive reference covering dynamic programming techniques, including
   their application in solving MDPs and optimal control problems.

3. **"Reinforcement Learning and Stochastic Optimization"** by Warren B.
   Powell — a valuable resource that explores the intersection of
   reinforcement learning and stochastic optimization, providing insights
   into advanced techniques.
