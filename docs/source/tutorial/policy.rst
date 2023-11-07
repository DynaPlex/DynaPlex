[Optional] Custom policy
========================

It is possible to provide a custom policy in ``policy.h`` and ``policy.cpp``. We will implement the decision rule as defined in :ref:`label_airplane`.

First, we register the policy in mdp.cpp:

.. code-block:: cpp

		void MDP::RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>& registry) const
		{
			registry.Register<RuleBasedPolicy>("rule_based",
				"The heuristic rule");
		}

Next, we implement ``policy.h``, we implement the constructor and a function called ``GetAction()``, which takes the state as input and outputs an action.
Furthermore, we define several variables that are parameters for tuning the policy. 

.. code-block:: cpp

	namespace DynaPlex::Models {
		namespace airplane /*must be consistent everywhere for complete mdp defininition and associated policies.*/
		{
			// Forward declaration
			class MDP;

			class RuleBasedPolicy
			{
				//this is the MDP defined inside the current namespace!
				std::shared_ptr<const MDP> mdp;
				const VarGroup varGroup;
				double price_threshold_low;
				double price_threshold_high;
				int64_t seat_threshold_low;
				int64_t seat_threshold_high;
				int64_t remainingday_threshold;
			public:
				RuleBasedPolicy(std::shared_ptr<const MDP> mdp, const VarGroup& config);
				int64_t GetAction(const MDP::State& state) const;
			};

		}
	}

We can set the parameters in a seperate config file ``policy_config_0.json``:

.. code-block:: json

	{
	  "id": "rule_based",
	  "price_threshold_low": 1000.0,
	  "price_threshold_high": 2000.0
	}


Next, we implement the policy in ``policy.cpp``:

.. code-block:: cpp

		...
		//MDP and State refer to the specific ones defined in current namespace
		RuleBasedPolicy::RuleBasedPolicy(std::shared_ptr<const MDP> mdp, const VarGroup& config)
			:mdp{ mdp }
		{
			//provide default values and load parameters from json
			config.GetOrDefault("price_threshold_low", price_threshold_low, 0.0);
			config.GetOrDefault("price_threshold_high", price_threshold_high, 1.0);
			//we can also set parameters directly in the constructor
			seat_threshold_low = 1;
			seat_threshold_high = 5;
			remainingday_threshold = 9;
		}

		int64_t RuleBasedPolicy::GetAction(const MDP::State& state) const
		{
			if (state.RemainingSeats > seat_threshold_high)
			{
				return 1;//sell
			}
			if (state.PriceOfferedPerSeat > price_threshold_low)
			{//only sell to type 1 and 2 customers
				if (state.RemainingSeats <= seat_threshold_high && state.RemainingSeats >= seat_threshold_low)
				{
					if (state.RemainingDays <= remainingday_threshold)
					{
						return 1;//sell
					}

				}
			}
			if (state.PriceOfferedPerSeat > price_threshold_high)
			{//only sell to type 1 customers
				if (state.RemainingSeats <= seat_threshold_high && state.RemainingSeats >= seat_threshold_low)
				{
					if (state.RemainingDays > remainingday_threshold)
					{
						return 1;//sell
					}

				}
			}
			return 0;//no sales
		}

