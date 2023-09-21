It looks like you have provided a markdown file (`adding_models.md`) detailing the process of adding a new model to some system called DynaPlex. Here's a typeset version of your content with some formatting for better clarity:

---

# Models

DynaPlex facilitates the extension with new native models (and policies). This document explains how to add an additional model, which will then be generally usable and retrievable (both in Python and for the generic C++ functionality).

---

## Understanding what is provided

To begin, review the implementations under `src/lib/models/models`, especially the one in `src/lib/models/models/lost_sales`.

The `lost_sales` model offers both a native MDP (in `../lost_sales/mdp.h` and `../lost_sales/mdp.cpp`) and a native policy (in `../lost_sales/policies.h` and `../lost_sales/policies.cpp`). It's possible to define an MDP without native policies by excluding the `policies.*` files. Standard policies (like "random") and learned policies will be available regardless. Additionally, some JSON files are provided, acting as default configuration files for unit testing. They also help in understanding the configuration parameters an MDP accepts.

---

## Adding a new model

Adding a model is easiest in debug mode, such as in WinDeb or LinDeb.

Start by copying the code of an existing MDP and registering it under a new name.

### Copy an MDP and register it under a new name:

1. **Duplicate Directory**: Copy the content of a directory associated with an existing MDP to a new directory. Name this new directory to match your new MDP's desired name (e.g., "mdp_id" in this example).
   - Choose an MDP with similar functionality if possible. Another option is `../models/models/empty_example/`, which is a basic MDP with limited functionality.
2. **Cleanup**: From the new directory, delete `policy.h`, `policy.cpp`, and all `.json` config files except for `mdp_config_0.json` (skip if using `/empty_example/`).
3. **Remove Policy References**: Delete any references to policies in these files. If using `/empty_example/` for step 1, you can skip this step.
4. **Namespace Update**: Modify namespaces in `mdp.h` and `mdp.cpp` in the newly created directory. For example, change from `lost_sales` or `empty_example` to the new name (`mdp_id`).
   - Refer to the section on namespaces for more details.
5. **Register Function**: Adjust the `MDP::Register` function in `../models/models/mdp_id/mdp.cpp` to register the new_id and include an appropriate description for your MDP.
6. **Registration Manager**: In `/models/models/registrationmanager.cpp`, forward-declare the Register function for the new model in the appropriate namespace. Call this function within the `RegisterAll` function.

Following these steps allows for creating a duplicate of another MDP registered under a different name. Ensure that the code compiles and runs without issues. If problems arise, refer to the troubleshooting section.

### Add test for the copied MDP

After successful compilation, set up a test for the new MDP:

1. Create a new file in `src/tests/mdp_unit_tests` named appropriately (e.g., `t_new_id.cpp`), where `_new_id_` matches the directory name and the registration name of the MDP.
2. Add a basic test content to this file. See the provided sample test content in your original document.
3. Modify the configuration file for testing the new MDP. Ensure the old `mdp_config_0.json` name is still valid but update the id.

---

## Change the logic so that the MDP matches the desired logic

Work on various components such as member variables, initial state creation, state-changing functions, custom events, and more.

---

## Key changes with respect to legacy DynaPlex

This section provides insights into the similarities and differences between the current API and the legacy DynaPlex library.

---

## Details on namespaces, directory names, ids, and registration

Maintain consistency when naming directories, files, classes, and namespaces for ease of reference and organization.

---

## Registering an MDP model

Make sure all DynaPlex MDP models are retrievable via `DynaPlex::DynaPlexProvider` and through the Python interface. The steps include updating the appropriate files and ensuring correct registration.

---

## Trouble-shooting

This section addresses common issues encountered during the process and offers solutions.

---

This typeset provides a clearer, more structured version of your README. Adjustments can be made as per your requirements or based on the platform where this content will be published.