.. _label_troubleshooting:

Troubleshooting
===============

If you encounter the following errors while working with DynaPlex, follow the associated solutions:


Missing CMakeLists.txt in googletest directory:
-----------------------------------------------

    .. code-block:: text

        CMake Error at C:\Users\<username>\source\repos\DynaPlexPrivate\CMakeLists.txt:20 (add_subdirectory):
          The source directory
            C:/Users/<username>/source/repos/DynaPlexPrivate/src/extern/googletest
          does not contain a CMakeLists.txt file.

    Solutions:

    This error usually occurs if the repository was cloned without the `--recurse-submodules` option, which results in missing submodule contents.

    To resolve this:

    1. Navigate to your cloned repository's root directory.
    2. Run the following commands:

    .. code-block:: bash

        git submodule init
        git submodule update

    This will fetch and checkout the appropriate content for the `googletest` submodule. After executing these commands, try running CMake again.


Missing MDP Identifier Error:
-----------------------------

   .. code-block:: text

      mdp_unit_tests\testutils.cpp(28): error: Expected: mdp = dp.GetMDP(mdp_vars_from_json); doesn't throw an exception.
      Actual: it throws DynaPlex::Error with description "DynaPlex: No MDP available with identifier "<some_identifier>". Use ListMDPs() / list_mdps() to obtain available MDPs.".

   Solutions:

   - Verify if ``<some_identifier>`` is the correct identifier for the MDP you added.
   - Ensure that you registered the MDP correctly using the ``MDP::Register(DynaPlex::Registry& registry)`` method in ``models/models/<...>/mdp.cpp``.
   - Update ``models/models/registrationmanager.cpp`` with the new MDP if you haven't done so.

Invalid Discount Factor Error:
------------------------------

   .. code-block:: text

      Actual: it throws DynaPlex::Error with description "DynaPlex: MDP, id "<some_identifier>" : discount_factor is invalid: -6277438562204192487878988888393020692503707483087375482269988814848.000000. Must be in (0.0,1.0]".

   Solutions:

   - Make sure you've read the ``discount_factor`` correctly from the provided ``VarGroup`` in the ``MDP::MDP(const VarGroup& config)`` constructor.

Linking Errors on Linux:
------------------------

   .. code-block:: text

      You may encounter an error during building similar to this: '/sw/arch/RHEL8/EB_production/2022/software/binutils/2.38-GCCcore-11.3.0/bin/ld: demandclass.cpp:(.text+0x65d): undefined reference to `DynaPlex::VarGroup::Add(std::string, double)''

   Solutions:

   - Make sure you have the correct LibTorch version, this error might be caused by linking to legacy c++ libraries like the preCXX11 libtorch library.

Warning Data Skew:
------------------

    You may get a warning: `WARNING possible data skew: sampling collection did not reach the final state even once for this finite horizon MDP`. 

    Explanation:

    Each processor on your machine will start collecting samples, beginning at the "start" of your model (initial state), and then gradually moving towards the end (final_state). A sample is collected at each step.

    Suppose you collect 5000 samples and you have 200 paralel processes. That is 25 samples per node. Then it has only collected a sample for the first 25 steps after calling GetInitialState. So then you get a possible "data skew", since it only collects samples for the start of the episode, and never towards the end.
    
    That is risky because then it might not learn how to make decisions towards the end of the episode. To learn whether things would become better, you could try to collect more samples, i.e. set N above 200 * [number of actions required to reach end of episode from initial state]. This should/would make the warning disappear. You may also opt to reduce the sampling_probability (option for DCL algorithm). This sample probability is the probability that the sampling algorithm takes a sample at each step. Normally, it is 1.0 (i.e. 100%), implying that you need a high value of N to always reach the end of episode. If you reduce the sampling probability, then maybe you reach the end more easily. But reducing that may also be detrimental to performance.
