## Models

DynaPlex facilitates extension with new native models (and policies). This document explains how to add an additional model, that
will then be generally usable and retrievable (both in python and for the generic c++ functionality.) 


# Understanding what is provided 
For starters, review the implementations under src/lib/models/models, in particular the one in src/lib/models/models/lost_sales.

The lost_sales model provides both a native mdp (in ../lost_sales/mdp.h and ../lost_sales/mdp.cpp) and a native policy (in ../lost_sales/policies.h and ../lost_sales/policies.cpp). One may define an MDP without native policies, i.e. leaving out the policies.* files. Standard policies (like "random") and 
learned policies will be available anyhow. Also some json files are provded, those are default configuration files for unit testing purposes. They also help in getting a quick idea what configuration parameters an MDP allows.

# Adding a new model

Adding a model is easiest in debug mode, e.g. in WinDeb or LinDeb. 

It is easiest to first copy the code of some existing MDP, and registering it under a new name. 

# Copy an MDP and register it under a new name

1. Copy the content of an entire directory associated with an existing mdp to a newly created directory. The name of the new directory should match the name that you would want
to give to your new mdp (which is chosen to equal "mdp_id" in below example). 
       - If you find an MDP with functionality somewhat similar to what you want, take that. Another option is ../models/models/empty_example/, which is an MDP that doesn't really do anything and throws NotImplementedException on most function calls. 
2. Delete policy.h and policy.cpp from the newly created folder. Also delete all .json config files, except mdp_config_0.json. (can be skipped if using /empty_example/.) 
3. Delete references to policies (if any) in these files: MDP::RegisterPolicies will simply not do anything. (this step can be skipped if using /empty_example/ in step 1. )
4. Change namespaces names in the files mdp.h and mdp.cpp in the _newly created_ directory (../models/models/mdp_id/) (e.g. from lost_sales / empty_example to the name that matches the folder name, i.e. mdp_id in this instruction). 
       - see under # Details on namespaces, directory names, ids for details. 
5. Change the MDP::Register function in ../models/models/mdp_id/mdp.cpp , to register this mdp under the new_id (which by convention should again equal the namespace name), and add
an appropriate description for your mdp. 
6. In /models/models/registrationmanager.cpp, forward declare the Register function of the new model in the appropriate namespace, and call that function in the body of the RegisterAll function.

Now you have made a copy of another MDP, and registered it under another name. The resulting code should compile and run without trouble. If compilation give problems,
you maybe forgot to change a namespace name or delete a reference to some policy. If it compiles, but running the tests fails with the following error:

     C++ exception with description "DynaPlex: Error during construction of MDP Registry. You attempted to register two MDPs with the same identifier: "lost_sales". Each MDP must be registered under a unique name." thrown in the test body.

please review step 5.

# add test for the copied MDP. 

If you can compile and run without trouble, it is probably a good idea to set up a test for your newly created MDP. 

To this end, add a new file to src/tests/mdp_unit_tests. The file should be named something like t_new_id.cpp, with _new_id_ equal to the newly created folder and the name under which we registered the mdp.  

For a simple test, add the following content to this file:

//file src/tests/mdp_unit_tests/t_new.cpp:
#include <gtest/gtest.h>
#include "testutils.h" // for ExecuteTest
namespace DynaPlex::Tests {	
	TEST(bin_packing, mdp_config_0) {

		//Note: models/model_name/mdp_config_name is valid json config file for mdp "model_name". 
		std::string model_name = "new_id"; //_this should match the id and namespace name discussed earlier. _ 
		std::string config_name = "mdp_config_0.json";
		ExecuteTest(model_name, config_name);
	}
}

Also, you need to adapt the configuration file, since we want to test the new MDP and not the old once. Since we didn't make any actual changes to the mdp code, 
the old mdp_config_0.json name should still be valid, except that we need to change the id:

{
  "id": "new_id", //or whatever is the id of your new mdp.  
  //"id": "lost_sales", //old id should be removed. 
  //other variables should remain unchanged for now. 
}

After these changes, the entire thing should still compile. Running DP_mdp_unit_tests.exe should be possible. However, if you copied the empty_example, tests will fail since many crucial functions throw NotImplementedError.

# Change the logic so that the MDP matches the desired logic. 

Combination of:
-changing member variables in mdp and nested state struct (in mdp.h)
    -keeping the initiation of those member variables updated in various functions in body. 
-provide logic for creating an initial state. 
-changing logic in the various functions that change states. 
-Change to a custom event if needed. 

# Key changes with respect to legacy DynaPlex

Models/MDPs are defined using very similar API as in the legacy DynaPlex library, but there are some important changes:

Strict requirements with namespaces will make it easier to copy and paste an MDP, change some identifiers, and add a new MDP. For details see below. 

Guidelines for modelling:
Integers elements of states and mdps are preferably held as int64_t variables (see below).
For continuous variables, use double.

Actions have moved from size_t (unsigned) to int64_t (signed). These two are _not_ mutually compatible. Reasons:
-easier interfacing with torch, which also uses int64_t at many places.
-int64_t is guaranteed same on all platforms, which simplifies interop.
-int64_t will enable the support of negative actions in future versions of dynaplex.
-by passing in an action as int64_t, and internally using int64_t for 

Default events are also int64_t, but manually overriding this is still possible.

Information on number of allowed actions, number of features, and alpha (now discount_factor) are no longer communicated via member functions NumFeatures() and NumValidActions() and member variable alpha. Instead
these are passed as part of a package of information provided by the MDP when GetStaticInfo() is called. This enables more flexibility towards the future, and has a number of other advantages. 

Every MPD should have a constructor that takes a const VarGroup&. VarGroup is a bit like a json file / nested dictionaries (but does not support all json features, e.g. only homongeous lists are supported, and the root must be a dictionary/object).
-Uniform construction enables the uniform creation of MDPs, and type erasure. I.e. a single function can be called to get any MDP, and there is a single type DynaPlex::MDP that can store every MDP. That makes algorithms easier to write, compilation faster, and bindings to python easier and more flexible.
-Incidentally, since the MDP is entirely determined by these vars, we can create a unique identifier from them. So no need anymore to manually implement and update GetIdentifier. 

VarGroup is used in many places in the code. For syntax, take a look at tests/other_unit_tests/t_model_provider.cpp and t_initiateclasswithvargroup.cpp in the same directory (the latter is more advanced then you might need and shows how to initiate a nested object).  
You are required to write a conversion from MDP::State to VarGroup. 
This then also enables states to be written to console in a simple way, and also to convert them to native python objects, e.g. for debugging/rendering purposes. 
The reverse conversion also needs to be provided, see lost_sales.mdp. 

# Details on namespaces, directory names, ids, registration:

When adding a new mdp, it is recommended to be consistent in naming directories, files, classes and namespaces 
(in the below example we suppose the mdp is identified by the name "mdp_id"), and directory names etc are named accordingly. Wherever you read mdp_id, you should
put the name of the new mdp you are adding. 

When defining an MDP, it is easiest (and recommended) to simply name the class MDP. However, to distinguish it from other MDPs, it must be wrapped in an appropriate namespace:

//file ../models/models/_mdp_id_/mdp.h
#pragma once
namespace DynaPlex::Models{
    //convention: MDP with id "mdp_id" is defined in namespace DynaPlex::Models::mdp_id 
    namespace _mdp_id_{
        class MDP{
          //Class definition.    
        }
    }
}

To ensure that the corresponding function definitions can be matched with this declaration, they should also be defined in the same namespace:


//file ../models/models/_mdp_id_/mdp.cpp
#include "mdp.h"
#include "dynaplex/erasure/mdpregistrar.h"
#include "policies.h"

namespace DynaPlex::Models {
	namespace _mdp_id_
	{
         VarGroup MDP::GetStaticInfo() const
         {
              //implementation.          
         }
         //other function definitions
    }
}

If at a later stage you would want to add custom policies, ensure that these also are declared and defined in the same namespace (here namespace DynaPlex::Models::mdp_id). 

Files should be in same directory, and filenames preferably ../models/models/mdp_id/policies.h and ../models/models/mdp_id/policies.cpp. 

Registering an MDP model

All DynaPlex mdp models should be retrievable via DynaPlex::DynaPlexProvider (see e.g. src/tests/other_unit_tests/t_model_provider.cpp for syntax to retrieve an MDP), and via python interface.

For this to work, each new MDP needs to be registered. Registration consists of two simple steps:
1. In the file that defines the MDP (which should be src/lib/models/models/place_holder/mdp.cpp for consistency), the function 
 
		void Register(DynaPlex::Registry& registry)

should register the MDP under an appropriate id. See existing models for syntax.

2. src/lib/models/models/registrationmanager.cpp: forward declaration to the resi, i.e. add:

3. src/lib/models/models/registrationmanager.cpp: the Register function for this MDP must be called in RegisterAll, i.e. add:

     new_id::Register(registry);



1. . For this to work, a forward declaration is needed

# Trouble-shooting:

mdp_unit_tests\testutils.cpp(28): error: Expected: mdp = dp.GetMDP(mdp_vars_from_json); doesn't throw an exception.
  Actual: it throws DynaPlex::Error with description "DynaPlex: No MDP available with identifier "<some_identifier>". Use ListMDPs() / list_mdps() to obtain available MDPs.".
  
  -Is the identifier <some_identifier> a correct identifier for an MDP that you added?
  -Did you correctly register that mdp under the desired name in MDP::Register(DynaPlex::Registry& registry) in models/models/<...>/mdp.cpp. 
  -Did you update models/models/registrationmanager.cpp with the new MDP?


      Actual: it throws DynaPlex::Error with description "DynaPlex: MDP, id "<some_identifier>" : discount_factor is invalid: -6277438562204192487878988888393020692503707483087375482269988814848.000000. Must be in (0.0,1.0].".
      Did you correctly read the discount_factor from the provided VarGroup in the MDP::MDP(const VarGroup& config) constructpr