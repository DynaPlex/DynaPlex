Changes from Legacy (v0.0)
==========================

.. note::
   It should not be necessary to make changes to any CMakeLists.txt. Newly added files should be automatically detected. Contact main developers if you find limitations in this.
   Set paths to dependencies in the CMakeUserPresets.txt, and not directly in the CMakeLists.txt. An exception happens when adding a new executable to the ``src/examples`` directory. 

The current version of the DynaPlex library uses a similar API to the legacy version. However, several changes have been made:

- **StateCategory**:
  - In Legacy DynaPlex, the flow of the program could be determined by the functions ``bool AwaitsAction(const State& state)``, ``bool IsFinal(const State& state)``, and for the case of multiple task types, also ``int TaskType(State& state)``. This was effective but a bit cumbersome, and the new version instead supports all functionality in  ``DynaPlex::StateCategory GetStateCategory(const state& state)``. StateCategory can be either ``IsAwaitEvent();``, ``IsAwaitAction();``, or ``IsFinal();``. Moreover, apart from having these three possible values, it also supports an index:
    
    .. code-block:: cpp

        auto cat = StateCategory::AwaitAction(int64_t index);

  - Index may be useful when you have a range of actions that must be performed sequentially after a single event, e.g. actions for various entities in your model. Indices will also support task types. 
  - It is recommended to simply have ``StateCategory`` as a member variable to ``State``, so that it can be kept up-to-date easily and returned from ``GetStateCategory``. 

- **Action Type**: Actions transitioned from ``size_t`` (unsigned) to ``int64_t`` (signed). This change was made for several reasons:
  - Improved compatibility with Torch, which uses ``int64_t``.
  - ``int64_t`` ensures consistency across platforms, streamlining interoperability.
  - The library can support negative actions in future DynaPlex versions.
  - Using ``int64_t`` for both input actions and internal logic simplifies implementation as an action can be subtracted from a number without casting. 

- **Static Info Communication**: Attributes like the number of allowed actions, number of features, and ``alpha`` (now ``discount_factor``) no longer come from member functions like ``NumFeatures()`` or ``NumValidActions()`` or from the ``alpha`` member variable. Instead, this information is packaged and provided when ``GetStaticInfo()`` is called, offering future flexibility and other benefits.

- **Namespaces**: Strict requirements for namespaces facilitate copying and pasting an MDP, changing identifiers, and adding new MDPs, as discussed extensively above.

- **Data Types for Modeling**:
  - State and MDP integer elements are preferably ``int64_t``.
  - For continuous variables, use ``double``.

- **Event Type**: By default, events are ``int64_t``, but manual overrides remain possible.

- **MDP Constructor**: Every MDP should have a constructor accepting a ``const VarGroup&``. The ``VarGroup`` behaves similarly to a JSON file or nested dictionary, albeit with some restrictions (lists must be homogeneous and root must be object/dict). This consistent construction method:
  - Enables the uniform creation of MDPs and type erasure. This means any MDP can be retrieved with a single function, and there's a singular type ``DynaPlex::MDP`` that can house every MDP.
  - Streamlines algorithm writing, compilation, and Python bindings.
  - Since the MDP is defined by these variables, a unique identifier can be generated from them, eliminating the need to manually implement and update ``GetIdentifier``.
  - Enables very flexible configuration of any MDP. 

- **VarGroup Usage**: The ``VarGroup`` is frequently used throughout the code. For its syntax, refer to ``tests/other_unit_tests/t_model_provider.cpp`` and ``t_initiateclasswithvargroup.cpp`` in the same directory. Apart from providing a constructor of the MDP based on ``VarGroup``, you'll need to:
  - Convert ``MDP::State`` to ``VarGroup``, facilitating state console printing and conversion to native Python objects, as shown in ``lost_sales.cpp``.
  - Provide a reverse conversion, as shown in ``lost_sales.cpp``.