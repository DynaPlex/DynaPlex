Implementing the Airplane MDP
=============================

We will take the following steps:

1. Define what a state is in ``mdp.h``.

2. Define what an event is in ``mdp.h``.

3. Provide the state in ``GetState()`` and ``ToVarGroup()`` functions.

4. Provide all needed static info in ``GetStaticInfo()``.

5. Provide the MDP initializer in ``MDP()``.

6. Provide all MDP logic.

7. Provide state feature information to the RL-policy


State and parameter definition
------------------------------

In ``mdp.h`` we can define all variables in a state. DynaPlex is flexible, in the sense that it allows all kinds of states and structure.

	The state consists the remaining days, remaining seats, and the price the current customer is willing to pay
	Aside from the state variables, we also define MDP parameters that are static, i.e., not dependent on events or actions. This is the price that the different types of customers are willing to pay for a seat, and the arrival distribution of customere types (to be defined later).
	Furthermore, we want to be able to be able to configure the initial values for the number of seats and days, later we will show how, but we already define both here.

.. note:
	We use ``DynaPlex::DiscreteDist`` for the arrival distribution. This contains various popular distributions, and allows for custom distributions.

.. hint::
	When using ``int`` datatypes, we advise to use ``int64_t``, since this is best compatible with pytorch.

.. code-block:: cpp

	class MDP
		{			
		public:	
			
			DynaPlex::DiscreteDist cust_dist;//the distribution of customer types
			std::vector<double> PricePerSeatPerCustType;

			int64_t InitialSeats;//get from json file
			int64_t InitialDays;//get from json file

			struct State {
				int64_t RemainingDays;
				int64_t RemainingSeats;
				double PriceOfferedPerSeat;

				//using this is recommended:
				DynaPlex::StateCategory cat;
				DynaPlex::VarGroup ToVarGroup() const;
				//Defaulting this does not always work. It can be removed as only the exact solver would benefit from this. 
				bool operator==(const State& other) const = default;
			};
	...

Event definition
----------------

In ``mdp.h``, we define what we consider a state. By default, events are ``int64_t`` datatypes. Although the events for this MDP could be defined as such, we will define them as ``struct`` to show how this works.

	Add the following in ``mdp.h``

.. code-block:: cpp
	
	//Events can take on any structure, here we use structs to showcase, but we may use int64_t if events are simpler
			struct Event {
				double PriceOfferedPerSeat;
			};
	...

GetState and ToVarGroup
-----------------------

Next, we add some needed info to get/set the state. In ``mdp.cpp``, we add the following two functions:

.. code-block:: cpp

	DynaPlex::VarGroup MDP::State::ToVarGroup() const
	{
		//add all state variables
		DynaPlex::VarGroup vars;
		vars.Add("cat", cat);
		vars.Add("RemainingDays", RemainingDays);
		vars.Add("RemainingSeats", RemainingSeats);
		vars.Add("PriceOfferedPerSeat", PriceOfferedPerSeat);
		return vars;
	}


.. code-block:: cpp

	MDP::State MDP::GetState(const VarGroup& vars) const
	{
		State state{};			
		vars.Get("cat", state.cat);
		vars.Get("RemainingDays", state.RemainingDays);
		vars.Get("RemainingSeats", state.RemainingSeats);
		vars.Get("PriceOfferedPerSeat", state.PriceOfferedPerSeat);
		return state;
	}

GetStaticInfo
-------------

Next, we can add the relevant MDP information. In this function, various MDP design choices cna be made. For now, we keep it simple:
We add the number of valid action, and the fact that our MDP is terminating, i.e. finite

.. code-block:: cpp

	VarGroup MDP::GetStaticInfo() const
	{
		VarGroup vars;
		//We can either accept or reject each arriving customer:
		vars.Add("valid_actions", 2);
		vars.Add("horizon_type", "finite");
		return vars;
	}

MDP initializer
---------------

Next, we need to provide a function that is called when initializing the MDP, this sets all relevant static info, as defined in ``mdp.h``
We also provide the probability distribution of different customer type arrivals.

.. code-block:: cpp

	MDP::MDP(const VarGroup& config)
	{
		config.Get("InitialDays", InitialDays);//get from json file
		config.Get("InitialSeats", InitialSeats);//get from json file

		PricePerSeatPerCustType = { 3000.0, 2000.0, 1000.0 };
		//probability distribution 0 (w. prob 0.4), 1 (w. prob 0.3), 2 (w. prob 0.3). 
		cust_dist = DiscreteDist::GetCustomDist( { 0.4,0.3,0.3 } );

		//Of course, any MDP property can be parameterized, but you can also
		//fix some things - configuration can always be expanded later.
	}

Next, we can provide the values for ``InitialDays`` and ``InitialSeats`` from the ``mdp_config_0.json`` file

	Open the ``mdp_config_0.json`` file and change it to:

.. code-block:: json

	{
	"id": "airplane",
	"InitialDays": 25,
	"InitialSeats": 12
	}

.. hint::
	Providing static info with ``.json`` files allows to easily test multiple configurations. It also helps in understanding the configuration parameters that an MDP accepts, as a form of documentation.

MDP logic
---------

Initial state
~~~~~~~~~~~~~

We start with the function that initializes all state variables at the start of the horizon.
For this MDP, we have three state categories: pre-action (post-event), post-action, and the final state, for this MDP, we start in a post-action state.

.. code-block:: cpp

	MDP::State MDP::GetInitialState() const
	{			
		State state{};
		state.cat = StateCategory::AwaitEvent();//or AwaitAction(), depending on logic
		//initiate other variables.
		state.PriceOfferedPerSeat = 0.0;
		state.RemainingDays = InitialDays;
		state.RemainingSeats = InitialSeats;
		return state;
	}

Event logic
~~~~~~~~~~~

We start with the event logic. First, we define a function that generates a new event.

.. code-block:: cpp

	MDP::Event MDP::GetEvent(RNG& rng) const {
		//generate an event using the custom discrete distribution (see MDP() initializer)
		int64_t custType = cust_dist.GetSample(rng);
		double pricePerSeat = PricePerSeatPerCustType.at(custType);
		return Event(pricePerSeat);//return the price related to the customer type
	}

Next, we can define what the event changes in the state. Note that this function can optionally return costs (negative rewards) in ``double`` type. In our case, we only have costs after an action.
Apart from changing the state, we check if the state we are in is the final state, i.e., if there are no remaining days or seats. In that case we change the ``state.cat`` to ``Final()``, the MDP episode will terminate.

.. code-block:: cpp

	double MDP::ModifyStateWithEvent(State& state, const Event& event) const
	{
		//after processing this event, we await an action.
		state.cat = StateCategory::AwaitAction();
		state.PriceOfferedPerSeat = event.PriceOfferedPerSeat;

		if (state.RemainingDays==0 || state.RemainingSeats == 0)
		{//here, we check if the MDP should terminate
			state.cat = StateCategory::Final();
		}
		return 0.0;//we only have costs after an action
	}

Action logic
~~~~~~~~~~~~

After modifying the state with the event, we can take an action and modify the state with it.

.. hint::
	It is a good habbit to build in some checks for logic errors, we provide some examples here.

.. code-block:: cpp

	double MDP::ModifyStateWithAction(MDP::State& state, int64_t action) const
	{
		if (state.RemainingDays == 0)
		{
			throw DynaPlex::Error("airplane::There should not be any sales in the last day.");
		}

		state.cat = StateCategory::AwaitEvent();//after processing this action, we await an event.

		if (action==0)
		{//reject offer
			state.RemainingDays--;//reduce the remaining days by 1
			return 0.0; //Note that flow ends when calling return, i.e.remainder of function is not carried out if action == 0
		}
		else
		{
			if (action == 1)
			{
				//Subtract the requested seats from the remaining seats
				state.RemainingSeats--;
				if(state.RemainingSeats<0)
					throw DynaPlex::Error("airplane:: Sold too many seats.");
				//One day passes.
				state.RemainingDays--;
				//Note that DynaPlex is by default cost-based, so we return negative reward here:
				double returnval = -state.PriceOfferedPerSeat;
				state.PriceOfferedPerSeat = 0.0;
				return returnval;
			}
			else
			{
				throw DynaPlex::Error("airplane:: Invalid action chosen.");
			}
		}
	}

As with most MDPs, the action space is constrained. We can define constraints in ``IsAllowedAction()``.  This function returns ``True`` if the action is allowed.

.. code-block:: cpp

	bool MDP::IsAllowedAction(const State& state, int64_t action) const {
		if (action == 0)
		{//rejection a customer is always allowed:
			return true;
		}
		//If we haven't returned, apparently action=1.
		//Selling a seat is allowed if there is at least one seat left:
		return state.RemainingSeats > 0;//alternative way to evaluate
	}

Provide state features
----------------------

Finally, we need to provide the RL-policy information about the state, claled ``features``. These features are always based on the state, as these vary per the event/action that occur.
You can engineer features, or rely on the neural network to do feature abstraction based on the literal state information. In this example, we only provide the literal state.

.. code-block:: cpp

	void MDP::GetFeatures(const State& state, DynaPlex::Features& features)const {
		//state features as supplied to learning algorithms:
		features.Add(state.RemainingDays);
		features.Add(state.RemainingSeats);
		features.Add(state.PriceOfferedPerSeat);
	}

