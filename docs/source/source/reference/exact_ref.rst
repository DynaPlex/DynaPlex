Exact policy reference
======================

.. cpp:function:: std::vector<std::tuple<MDP::Event, double>> MDP::EventProbabilities() const
.. cpp:function:: std::vector<std::tuple<MDP::Event, double>> MDP::EventProbabilities(const MDP::State&) const

   The `EventProbabilities` function in the MDP class provides a list of possible events along with their respective probabilities. This function is essential for exact optimization in the context of Markov Decision Processes.
   Note that this is typically only feasible if the state space if finite and not too big, i.e. at most a few million states.
   
   :returns: A vector of tuples, each containing an event and its probability.
   :rtype: std::vector<std::tuple<MDP::Event, double>>

   **Functionality**

   - The first variant of the function returns probabilities independent of the state.
   - The second variant takes a state as a parameter and returns probabilities that are specific to that state.

   **Usage Example**

   .. code-block:: cpp

      // Example of using state-independent EventProbabilities
      	std::vector<std::tuple<MDP::Event, double>> MDP::EventProbabilities() const {
			return demand_dist.QuantityProbabilities();
		}

