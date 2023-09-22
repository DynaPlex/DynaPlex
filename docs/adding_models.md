# Models

DynaPlex facilitates the extension with new native models (e.g. Markov Decision Processes (MDPs) and  policies). This document explains how to add an additional model, which will then be generally usable and retrievable (both in Python and for the generic C++ functionality).

---

## Understanding what needs to be provided

To begin, review the implementations under `src/lib/models/models`, especially the one in `src/lib/models/models/lost_sales`.

The `lost_sales` model offers both a native MDP (in `../lost_sales/mdp.h` and `../lost_sales/mdp.cpp`) and a native policy (in `../lost_sales/policies.h` and `../lost_sales/policies.cpp`). It's possible to define an MDP without native policies by excluding the `policies.*` files. Standard policies (like "random") and learned policies will be available for every MDP, so it is not strictly needed to define policies and when defining your first MDP you could do well without it. Additionally, some JSON files are provided, acting as default configuration files for unit testing. They also help in understanding the configuration parameters that an MDP accepts, as a form of documentation.

---

## Adding a new model

Adding a model is easiest in debug mode, such as in WinDeb or LinDeb.

Start by copying the code of an existing MDP and registering it under a new name.

### Copy an MDP and register it under a new name:

1. **Duplicate Directory**: Copy the content of a directory associated with an existing MDP to a new directory. Name this new directory to match your new MDP's desired name (e.g., "mdp_id" in this example).
   - Choose an MDP with similar functionality if possible. Another option is `../models/models/empty_example/`, which is an MDP that throws NotImplementedExceptions in most functions, meaning that those can be reimplemented later with the desired functionality.
2. **Cleanup**: From the new directory, delete `policy.h`, `policy.cpp`, and all `.json` config files except for `mdp_config_0.json` (skip step if using `/empty_example/`).
3. **Remove Policy References**: Delete any references to policies in these files. (skip step if using `/empty_example/`).
4. **Namespace Update**: Modify namespaces in `mdp.h` and `mdp.cpp` in the newly created directory. For example, change from `lost_sales` or `empty_example` to the new name (`mdp_id`).
   - Refer to the [section on namespaces](#namespaces) for more details.
5. **Register Function**: Adjust the `MDP::Register` function in `../models/models/mdp_id/mdp.cpp` to register the new mdp under an appropriate "mdp_id" (matching the directory name)  and include an appropriate description for your MDP.
6. **Registration Manager**: In `/models/models/registrationmanager.cpp`, forward-declare the Register function for the new model in the appropriate namespace. Call this function within the `RegisterAll` function.
  - Refer to the [section on namespaces](#namespaces) for more details.
 
Following these steps allows for creating a duplicate of an MDP registered under a new name. Ensure that the code compiles and runs without issues. If problems arise, refer to the troubleshooting section.

---

### Add test for the copied MDP

(DynaPlex includes googletest, a well-known and well-documented test framework. Using tests to check whether code is following intended logic is a good way to verify that an MDP implements  all required logic, and to quickly identify oversights.) 

If compilation in the previous step was succesfull, then it is time to set up a googletest for the new MDP:

1. Create a new file in `src/tests/mdp_unit_tests` named appropriately (e.g., `t_mdp_id.cpp`), where `mdp_id` matches the directory name and the registration name of the newly added MDP.

2. Add some content that tests the new MDP. The following is a good start (note mdp_id should be changed twice to the id of the mdp you are testing for!). You could add other tests later:

```cpp
// file: src/tests/mdp_unit_tests/t_mdp_id.cpp:
#include <gtest/gtest.h>
#include "testutils.h" // for ExecuteTest

namespace DynaPlex::Tests {    
    TEST(mdp_id, mdp_config_0) {
        std::string model_name = "mdp_id"; // this should match the id and namespace name discussed earlier
        std::string mdp_config_name = "mdp_config_0.json";
        Tester tester{};
		tester.ExecuteTest(model_name, config_name);
    }
}
//other tests can be added later. 
```

3. Modify the configuration file for testing the new MDP. Ensure the old `mdp_config_0.json` name is still valid but update the id:
```json
{
  "id": "mdp_id",
  // other variables should be left unchanged, as we didn't make any changes to the old mdp except the id under which it is registered. 
}
```

Compiling and running mdp_unit_tests.exe should give some results. If starting from a working mdp (like lost_sales), all tests should pass since all we did
was register existing logic under a new name. If starting from empty_example, test will fail, since many functions in that mdp throw exceptions, but this can be solved by inputting appropriate logic everywere. 

This default test works as follows. During compilation, the json files in models/models get copied to a subdirectory of in DYNAPLEX_IO_ROOT_DIR/IO_DynaPlex/defaults. 

Now, when the test executes, ExecuteTest does the following:
- Loads json from `IORootDir/IO_DynaPlex/defaults/model_name/mdp_config_name`
- Attempts to configure MDP from that json file
- Sets a random policy
- Performs range of tests using the MDP and the Policy, i.e. checks whether a number of simulation steps can be run, and whether all things work appropriately. 

**Note:** A .json file is one way to configure an MDP, but mdps may also be directly configured from c++ code (via a VarGroup, see `../src/tests/other_unit_tests/t_model_provider.cpp`), or via a dict object in python.

---

## Modify the MDP Logic to Match Desired Functionality

Iteratively combine the following to adjust the MDP to your desired logic:

1. **Modify Member Variables**: Adjust member variables in both the `mdp` and nested `state` struct located in `mdp.h`.
    - Make sure to update the functions providing serialization and initiation within the class body.
2. **Initial State Logic**: Provide logic for creating an initial state.
3. **State Transition Logic**: Update the logic in functions that determine state transitions, i.e. ModifyStateWith...
4. **Custom Event Logic**: default events are int64_t, but often this is not appropriate. 
5. **Modify the test configuration**: while adapting the logic, make sure to keep the `mdp_config_0' up to date to ensure that the mdp can be tested. 

Running `dp_mdp_unit_tests` is a must to avoid oversights. 

To verify that your code logic is in order, you might next want to visualize a sequence of states when following some policy. For an example, see e.g. the test `TEST(bin_packing, trace)` in `src/tests/mdp_unit_tests/t_bin_packing.cpp`. 



---

## Notable Changes from Legacy DynaPlex

NOTE: It should not be neccesary to make changes to any CMakeLists.txt. Newly added files should be automatically detected. Contact main developers if you find limitatations in this.
Set paths to dependencies in the CMakeUserPresets.txt, and not directly in the CMakeLists.txt! 

The current version of the DynaPlex library uses a similar API to the legacy version. However, several changes have been made:

- **StateCategory**:
  - In Legacy DynaPlex, the flow of the program could be determined by the function `bool AwaitsAction(const State& state)`, and `bool IsFinal(const State& state)`, and for the case of 
  multiple tasktypes, also `int TaskType(State& state)`. This was effective but a bit cumbersome, and the new version instead supports all functionality in  `DynaPlex::StateCategory GetStateCategory(const state& state)`. StateCategory can be 
  either `IsAwaitEvent();`, `IsAwaitAction();`, or `IsFinal();`. Moreover, apart from having these three possible values, it also supports an index:
```cpp
auto cat = StateCategory::AwaitAction(int64_t index);
```
  - Index may be usefull when you have a range of actions that must be performed sequentially after a single event, e.g. actions for various entities in your model. Indices will also support tasktypes. 

  - It is recommended to simply have `StateCategory` as a member variable to `State`, so that it can be kept up-to-date easily and returned from `GetStateCategory`. 

- **Action Type**: Actions transitioned from `size_t` (unsigned) to `int64_t` (signed). This change was made for several reasons:
  - Improved compatibility with Torch, which uses `int64_t`.
  - `int64_t` ensures consistency across platforms, streamlining interoperability.
  - The library can support negative actions in future DynaPlex versions.
  - Using `int64_t` for both input actions, and internal logic, simplifies implementation as an action can be substracted from a number without casting. 

- **Static Info Communication**: Attributes like the number of allowed actions, number of features, and `alpha` (now `discount_factor`) no longer come from member functions like `NumFeatures()` or `NumValidActions()` or from the `alpha` member variable. Instead, this information is packaged and provided when `GetStaticInfo()` is called, offering future flexibility and other benefits.

- **Namespaces**: Strict requirements for namespaces facilitate copying and pasting an MDP, changing identifiers, and adding new MDPs, as discussed extensively above.
  
- **Data Types for Modeling**:
  - State and MDP integer elements are preferably `int64_t`.
  - For continuous variables, use `double`.

- **Event Type**: By default, events are `int64_t`, but manual overrides remain possible.



- **MDP Constructor**: Every MDP should have a constructor accepting a `const VarGroup&`. The `VarGroup` behaves similarly to a JSON file or nested dictionary, albeit with some limitations (e.g., only supporting homogeneous lists and requiring the root to be an object). This consistent construction method:
  - Enables the uniform creation of MDPs and type erasure. This means any MDP can be retrieved with a single function, and there's a singular type `DynaPlex::MDP` that can house every MDP.
  - Streamlines algorithm writing, compilation, and python bindings.
  - Since the MDP is defined by these variables, a unique identifier can be generated from them, eliminating the need to manually implement and update `GetIdentifier`.
  - enables very flexible configuration of any MDP. 

- **VarGroup Usage**: The `VarGroup` is frequently used throughout the code. For its syntax, refer to `tests/other_unit_tests/t_model_provider.cpp` and `t_initiateclasswithvargroup.cpp` in the same directory. You'll need to:
  - Convert `MDP::State` to `VarGroup`, facilitating state console printing and conversion to native python objects.
  - Provide a reverse conversion, as shown in `lost_sales.mdp`.

---

<a name="namespaces"></a>
## Details on Namespaces, Directory Names, IDs, Registration

When adding a new MDP, consistency in naming directories, files, classes, and namespaces is essential. In the examples below, the placeholder `mdp_id` represents the name identifier for the MDP. Ensure you replace each mention of `mdp_id` with the appropriate name for the new MDP you're adding.

### Naming the MDP Class

It's recommended to name the MDP class simply as `MDP`. To differentiate it from other MDPs, it should be enclosed within a specific namespace:

```cpp
// File: ../models/models/mdp_id/mdp.h
#pragma once

namespace DynaPlex::Models {
    // Convention: MDP with id "mdp_id" is defined in namespace DynaPlex::Models::mdp_id 
    namespace mdp_id {
        class MDP {
            // Class definition goes here.    
        };
    }
}
```

### Function Definitions

Ensure that corresponding function definitions align with the above declaration by defining them within the same namespace:

```cpp
// File: ../models/models/_mdp_id_/mdp.cpp
#include "mdp.h"
#include "dynaplex/erasure/mdpregistrar.h"
#include "policies.h"

namespace DynaPlex::Models {
    namespace mdp_id {
         VarGroup MDP::GetStaticInfo() const {
              // Implementation details go here.          
         }

         // Other function definitions follow.
    }
}
```

### Incorporating Custom Policies

If you decide to introduce custom policies in the future, make sure they are declared and defined within the consistent namespace, i.e., `DynaPlex::Models::mdp_id`.

Preferred filenames for policy files are `../models/models/mdp_id/policies.h` and `../models/models/mdp_id/policies.cpp`. For a working example, refer to the `lost_sales` model.


## Registering an MDP Model

To make a DynaPlex MDP model retrievable via `DynaPlex::DynaPlexProvider` (refer to `src/tests/other_unit_tests/t_model_provider.cpp` for the syntax to retrieve an MDP) and the Python interface, it's imperative to register each new MDP. This registration process can be distilled into the following steps:

### 1. Registering the MDP in Its Definition File

In the definition file of the MDP (to be located at `src/lib/models/models/<mdp_id>/mdp.cpp`), update the function:

```cpp
void Register(DynaPlex::Registry& registry)
```

to ensure that it registers the MDP under an appropriate ID. For exact syntax, refer to existing models.

### 2. Forward Declaration in the Registration Manager

Navigate to `src/lib/models/models/registrationmanager.cpp` and add a forward declaration for the `Register` function:

```cpp
namespace <mdp_id> {
    void Register(DynaPlex::Registry&);
}
```

Replace `<mdp_id>` with your specific MDP's identifier.

### 3. Calling the Register Function in `RegisterAll`

Still within `src/lib/models/models/registrationmanager.cpp`, the `Register` function for the new MDP must be invoked inside the `RegisterAll` method:

```cpp
void RegistrationManager::RegisterAll(DynaPlex::Registry& registry) {
    lost_sales::Register(registry);
    bin_packing::Register(registry);
    // Other MDP registrations go here.

    // Add your new MDP's registration:
    <mdp_id>::Register(registry);
}
```

Replace `<mdp_id>` with your specific MDP's identifier.

---

With these steps, your MDP model should be properly registered and made retrievable both through the specified provider and the Python interface.
___

## Trouble-shooting:

If you encounter the following errors while working with DynaPlex, follow the associated solutions:

### 1. Missing MDP Identifier Error:
```
mdp_unit_tests\testutils.cpp(28): error: Expected: mdp = dp.GetMDP(mdp_vars_from_json); doesn't throw an exception.
Actual: it throws DynaPlex::Error with description "DynaPlex: No MDP available with identifier "<some_identifier>". Use ListMDPs() / list_mdps() to obtain available MDPs.".
```

**Solutions:**
- Verify if `<some_identifier>` is the correct identifier for the MDP you added.
- Ensure that you registered the MDP correctly using the `MDP::Register(DynaPlex::Registry& registry)` method in `models/models/<...>/mdp.cpp`.
- Update `models/models/registrationmanager.cpp` with the new MDP if you haven't done so.

### 2. Invalid Discount Factor Error:
```
Actual: it throws DynaPlex::Error with description "DynaPlex: MDP, id "<some_identifier>" : discount_factor is invalid: -6277438562204192487878988888393020692503707483087375482269988814848.000000. Must be in (0.0,1.0].".
```

**Solutions:**
- Make sure you've read the `discount_factor` correctly from the provided `VarGroup` in the `MDP::MDP(const VarGroup& config)` constructor.
