Modelling reference
===================

.. cpp:function:: VarGroup MDP::GetStaticInfo() const

   Retrieves static information about the Markov Decision Process (MDP) model, including valid actions, features, and diagnostic data. This information is not subject to change over the course of the MDP's execution and can be used for initializing or configuring other components that interact with the MDP.

   :returns: A VarGroup object containing static information about the MDP.
   :rtype: VarGroup

   **Examples**

   .. code-block:: cpp

      // Exact implementation of GetStaticInfo
      VarGroup MDP::GetStaticInfo() const
      {
          VarGroup vars;
          vars.Add("valid_actions", MaxOrderSize + 1);
          vars.Add("discount_factor", 0.99);//when not provided, this is assumed to be 1.0
          vars.Add("horizon_type", "infinite");
          
          VarGroup feats{};
          
          vars.Add("features", feats);
          vars.Add("discount_factor", discount_factor);
          
          // Potentially add any stuff that was computed for diagnostics purposes
          // not used by dynaplex framework itself.
          VarGroup diagnostics{};
          diagnostics.Add("MaxOrderSize", MaxOrderSize);
          diagnostics.Add("MaxSystemInv", MaxSystemInv);
          vars.Add("diagnostics", diagnostics);
          
          return vars;
      }

.. cpp:function:: DynaPlex::StateCategory MDP::GetStateCategory(const State& state) const

   Retrieves the category of the current state within the Markov Decision Process (MDP). This function is important for determining the state's classification in the decision process.

   :param const State& state: The state whose category is to be identified.
   :returns: The category of the given state.
   :rtype: DynaPlex::StateCategory

   **Examples**

   .. code-block:: cpp

      // Exact implementation of GetStateCategory
      DynaPlex::StateCategory MDP::GetStateCategory(const State& state) const
      {
          return state.cat;
      }

.. cpp:function:: MDP::State MDP::GetState(const DynaPlex::VarGroup& vars) const

   Reconstructs the state of the Markov Decision Process (MDP) from a set of variables. This function is typically used for deserializing a state or creating it based on external inputs.
   
   :param const DynaPlex::VarGroup& vars: Variables defining the state.
   :returns: The reconstructed state.
   :rtype: MDP::State

   **Examples**

   .. code-block:: cpp

      // Exact implementation of GetState
      MDP::State MDP::GetState(const DynaPlex::VarGroup& vars) const
      {
          State state{};
          vars.Get("cat", state.cat);
          vars.Get("state_vector", state.state_vector);
          vars.Get("total_inv", state.total_inv);
          return state;
      }

.. cpp:function:: MDP::State MDP::GetInitialState() const

   Initializes the state of the Markov Decision Process (MDP) to the starting conditions. This involves setting the initial state variable values and the starting state category.

   :returns: The initial state of the MDP.
   :rtype: MDP::State

   **Examples**

   .. code-block:: cpp

      // Exact implementation of GetInitialState
      MDP::State MDP::GetInitialState() const
      {
          auto queue = Queue<int64_t>{};
          queue.reserve(leadtime + 1);
          queue.push_back(MaxSystemInv); // Initial on-hand inventory
          for (size_t i = 0; i < leadtime - 1; i++)
          {
              queue.push_back(0);
          }
          State state{};
          state.cat = StateCategory::AwaitAction();
          state.state_vector = queue;
          state.total_inv = queue.sum();
          return state;
      }

.. cpp:function:: double MDP::ModifyStateWithAction(State& state, int64_t action) const

   Applies an action to the current state within the Markov Decision Process (MDP). It modifies the state's based on the action taken and returns the cost associated with this action, which is zero in the given example.

   :param State& state: The current state of the system to be modified.
   :param int64_t action: The action to be applied to the state.
   :returns: The immediate cost associated with the action, which is zero in this case.
   :rtype: double
   :raises DynaPlex::Error: If the action is not allowed in the current state.

   **Examples**

   .. code-block:: cpp

      // Exact implementation of ModifyStateWithAction
      double MDP::ModifyStateWithAction(State& state, int64_t action) const
      {
          if (!IsAllowedAction(state, action))
          {
              throw DynaPlex::Error("Lost Sales: action not allowed: state.total_inv: " + std::to_string(state.total_inv) + "  action: " + std::to_string(action) + "  MaxSystemInv: " + std::to_string(MaxSystemInv) + " MaxOrderSize " + std::to_string(MaxOrderSize));
          }
          state.state_vector.push_back(action);
          state.total_inv += action;
          state.cat = StateCategory::AwaitEvent();
          return 0.0;
      }

.. cpp:function:: double MDP::ModifyStateWithEvent(State& state, const MDP::Event& event) const

   Updates the state of the Markov Decision Process (MDP) model in response to an event, e.g., representing demand. This function adjusts the state calculates the incurred cost or profit from the event.

   :param State& state: The state to be updated, usually representing the current inventory levels.
   :param const MDP::Event& event: The event that affects the state, generally a demand event.
   :returns: The cost or profit resulting from the event.
   :rtype: double

   **Examples**

   .. code-block:: cpp

      // Exact implementation of ModifyStateWithEvent
      double MDP::ModifyStateWithEvent(State& state, const MDP::Event& event) const
      {
          state.cat = StateCategory::AwaitAction();
          
          auto onHand = state.state_vector.pop_front(); // Length is leadtime again.
          
          if (onHand > event)
          {
              // There is sufficient inventory. Satisfy order and incur holding costs.
              onHand -= event;
              state.total_inv -= event;
              state.state_vector.front() += onHand;
              return onHand * h;
          }
          else
          {
              state.total_inv -= onHand;
              return (event - onHand) * p;
          }
      }

.. cpp:function:: bool MDP::IsAllowedAction(const State& state, int64_t action) const

   Determines whether a given action is permissible in the current state within the Markov Decision Process (MDP). This function ensures the validity of actions in the state space.

   :param const State& state: The current state to be examined.
   :param int64_t action: The action to be validated.
   :returns: True if the action is allowed, false otherwise.
   :rtype: bool

   **Examples**

   .. code-block:: cpp

      // Exact implementation of IsAllowedAction
      bool MDP::IsAllowedAction(const State& state, int64_t action) const
      {
          return ((state.total_inv + action) <= MaxSystemInv && action <= MaxOrderSize) 
                     || action == 0;
      }

.. cpp:function:: MDP::Event MDP::GetEvent(RNG& rng) const

   Generates a random event based on the underlying probability distribution defined within the Markov Decision Process (MDP) model. The event represents a sample from the demand distribution which is a key component in the lost sales MDP.

   :param RNG& rng: A random number generator that aids in the sampling process.
   :returns: An event which is a sample from the demand distribution.
   :rtype: MDP::Event

   **Examples**

   .. code-block:: cpp

      // Exact implementation of GetEvent
      MDP::Event MDP::GetEvent(RNG& rng) const
      {
          return demand_dist.GetSample(rng);
      }

.. cpp:function:: void MDP::GetFeatures(const State& state, DynaPlex::Features& features) const

   Extracts features from the given state within the Markov Decision Process (MDP) model. These features are used for policy evaluation or decision-making purposes within the MDP framework.

   :param const State& state: The state from which to extract features.
   :param DynaPlex::Features& features: The features object to be populated with the extracted features.
   :returns: None.
   :rtype: void

   **Examples**

   .. code-block:: cpp

      // Exact implementation of GetFeatures
      void MDP::GetFeatures(const State& state, DynaPlex::Features& features) const
      {
          features.Add(state.state_vector);
      }