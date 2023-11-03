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