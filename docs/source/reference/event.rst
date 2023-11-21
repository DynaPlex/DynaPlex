Event class reference
=====================

.. cpp:class:: Event

   The Event class, defined within the MDP class of the DynaPlex framework, is integral to representing various events that occur in a Markov Decision Process. It encapsulates the characteristics and data of an event, impacting the state transitions within the MDP.

   The Event class in this implementation is an alias for a primitive data type (e.g., int64_t), but it can be customized to a more complex struct or class if needed. This flexibility allows for the representation of a wide range of events, from simple occurrences to complex scenarios with multiple attributes.

   **Key Characteristics**

   - The Event class is used to model the occurrences that affect the state of the MDP.
   - It can be a primitive type or a more complex data structure, depending on the needs of the specific MDP model.
   - Events are used in conjunction with state and action classes to model the dynamics of the MDP.

   **Usage in MDP**

   In the `MDP` class, the `Event` type is used primarily in functions like `ModifyStateWithEvent` and `GetEvent`, where it plays a crucial role in defining how the state of the MDP changes in response to various occurrences.

   **Example Usage**

   .. code-block:: cpp

      // Example of defining an Event in MDP
      using Event = int64_t;  // Event defined as an alias for int64_t

