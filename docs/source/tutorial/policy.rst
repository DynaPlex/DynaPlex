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
				int64_t seat_threshold_low;
				int64_t seat_threshold_high;
				int64_t remainingday_threshold;
				double price_threshold_low;
				double price_threshold_high;
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
	  "seat_threshold_low": 1,
	  "seat_threshold_high": 5,
	  "remainingday_threshold": 9,
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
			//define the policy parameters and provide default values
			config.GetOrDefault("seat_threshold_low", seat_threshold_low, 1);
			config.GetOrDefault("seat_threshold_high", seat_threshold_high, 5);
			config.GetOrDefault("remainingday_threshold", remainingday_threshold, 9);
			config.GetOrDefault("price_threshold_low", price_threshold_low, 1000.0);
			config.GetOrDefault("price_threshold_high", price_threshold_high, 2000.0);
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

