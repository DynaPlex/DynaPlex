State class reference
=====================

.. cpp:class:: State

   The State class within the MDP class represents the state of a Markov Decision Process in the DynaPlex framework. It encapsulates all the necessary information about the current condition of the MDP, which is crucial for decision-making and state transitions.

   DynaPlex::StateCategory cat: Stores the category of the state, indicating what should happen next (event, action, or reaching a final state).

   **Functionality**

   - The `State` class is designed to track and manage the current status of the MDP.
   - It includes mechanisms to store and update various state attributes.
   - The class can be extended or modified to include additional state variables as needed for different MDP models.

   **Methods**

   - `ToVarGroup() const`: Converts the state to a `VarGroup` object for serialization or other purposes.
   - `operator==`: Compares two states for equality. This is optional and can be removed if not required by the solver.

   **Usage Example**

   .. code-block:: cpp

      // Example of using the State class
      DynaPlex::Models::lost_sales::MDP::State currentState;
      // Setting state properties
      currentState.total_inv = 50; // Example inventory level
      currentState.cat = DynaPlex::StateCategory::AwaitAction(); // Setting the state category

      // Using state in decision processes
      auto action = policy.GetAction(currentState); // Determine the action based on the current state
