Custom policy reference
=======================

.. cpp:function:: void MDP::RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>& registry) const

   Registers custom policies for the Markov Decision Process (MDP). This function is essential for adding heuristic policies that can be used for decision-making within the MDP framework.

   :param DynaPlex::Erasure::PolicyRegistry<MDP>& registry: The registry where policies are stored and managed.
   :returns: None.
   :rtype: void

   **Examples**

   .. code-block:: cpp

      // Exact implementation of RegisterPolicies
      void MDP::RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>& registry) const
      {
          registry.Register<BaseStockPolicy>("base_stock",
              "Base-stock policy with parameter base_stock_level - default parameter is equal"
              " to the bound on system inventory discussed in Zipkin (2008)");
      }


.. cpp:function:: int64_t CustomPolicy::GetAction(const MDP::State& state) const

   The `GetAction` function in the CustomPolicy class calculates and returns the action to be taken for a given state in the MDP.
   This is only relevant for those that want to run their own custom policy, e.g., a heuristic baseline.

   :param const MDP::State& state: The current state of the MDP, from which the action is determined.
   :returns: The action determined to be most appropriate for the given state.
   :rtype: int64_t

   **Usage Example**

   .. code-block:: cpp

      int64_t BaseStockPolicy::GetAction(const MDP::State& state) const
		{
			int64_t action = base_stock_level - state.total_inv;
			//We maximize, so actually this is capped base-stock. 
			if (action > mdp->MaxOrderSize)
			{
				action = mdp->MaxOrderSize;
			}
			return action;
		}