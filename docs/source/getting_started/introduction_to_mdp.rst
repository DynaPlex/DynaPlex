A brief introduction to MDPs
============================

Welcome to the world of Markov Decision Processes (MDPs)! MDPs are a fundamental concept in the field of artificial intelligence and reinforcement learning. They are widely used to model decision-making problems where an agent interacts with an environment to maximize some notion of cumulative reward.

What is an MDP?
---------------

A Markov Decision Process (MDP) is a mathematical framework used to model decision-making problems in which an agent interacts with an environment over a series of discrete time steps. At each time step, the agent observes the current state of the environment and selects an action. The environment responds by transitioning to a new state and providing a reward signal to the agent. Importantly, the transition to the next state and the reward received only depend on the current state and the action taken, satisfying the Markov property.

Key Components of an MDP
-------------------------

1. **States (S):** The set of all possible situations or configurations in which the agent can find itself.

2. **Actions (A):** The set of all possible moves or decisions that the agent can make in a given state.

3. **Transition Probabilities (P):** The probabilities that dictate the likelihood of transitioning from one state to another when a specific action is taken.

4. **Rewards (R):** The numerical values associated with state-action pairs, indicating the immediate benefit or cost of taking a particular action in a specific state.

5. **Policy (π):** A strategy that defines the agent's behavior, specifying which action to take in each state.

.. figure:: ../assets/images/mdp_illustration.png
   :alt: MDP illustration

DynaPlex builds on the MDP-EI (MDP with exogenous inputs) framework, which is illustrated below. Here, :math:`s_t` represents the state at time :math:`t`, :math:`\pi` represent the policy, :math:`a_t` the decision, and :math:`c_t` the costs, the exogenous event is denoted by :math:`\xi_t` and the transition function is :math:`f`.
For more information, we refer to: `ArXiv paper <https://arxiv.org/abs/2011.15122>`_ and `A unified framework for stochastic optimization <https://doi.org/10.1016/j.ejor.2018.07.014>`_

.. figure:: ../assets/images/mdpei.png
   :alt: MDP-EI illustration

Why MDPs are Important
-----------------------

MDPs provide a formal and systematic way to model and solve decision-making problems. They are used in various applications, including robotics, game playing, autonomous systems, and optimization tasks. By understanding and implementing MDPs, you can design intelligent agents capable of making optimal decisions in complex, uncertain environments.

Explore the rest of our documentation to learn how to get started, create your own MDPs, and leverage the full capabilities of our software tool. Happy coding!

.. note::
   For detailed guides and examples, please refer to the specific sections of this documentation site.

Recommended Reference Books
---------------------------

For further exploration of Markov Decision Processes, we recommend the following books:

1. "Markov Decision Processes: Discrete Stochastic Dynamic Programming" by Martin L. Puterman.
   - This book offers a thorough treatment of MDPs, including dynamic programming methods and their applications.

2. "Dynamic Programming and Optimal Control" by Dimitri P. Bertsekas.
   - A comprehensive reference covering dynamic programming techniques, including their application in solving MDPs and optimal control problems.

3. "Reinforcement Learning and Stochastic Optimization" by Warren B. Powell.
   - A valuable resource that explores the intersection of reinforcement learning and stochastic optimization, providing insights into advanced techniques.

4. "Reinforcement Learning: An Introduction" by Richard S. Sutton and Andrew G. Barto.
   - This comprehensive book provides an in-depth introduction to reinforcement learning, including MDPs and various algorithms used for solving them.