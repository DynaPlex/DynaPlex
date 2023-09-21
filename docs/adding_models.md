## Models

DynaPlex facilitates extension with new native models (and policies). This document explains how to add an additional model, that
will then be generally usable and retrievable (both in python and for the generic c++ functionality.) 


# Understanding what is provided 
For starters, review the implementations under src/lib/models/models, in particular the one in src/lib/models/models/lost_sales.

The lost_sales model provides both a native mdp (in ../lost_sales/mdp.h and ../lost_sales/mdp.cpp) and a native policy (in ../lost_sales/policies.h and ../lost_sales/policies.cpp). One may define an MDP without native policies, i.e. leaving out the policies.* files. Standard policies (like "random") and 
learned policies will be available anyhow. Also some json files are provded, those are default configuration files for unit testing purposes. They also help in getting a quick idea what configuration parameters an MDP allows.

# Adding a new model

It is recommended to be consistent in naming directories, files, and classes. For example, to define a new MDP (say named "place_holder"), we would add the directory src/lib/models/models/place_holder),
and in that directory we would add the files mdp.h and mdp.cpp. 

When defining an MDP, it is easiest (and recommended) to simply name the class MDP. However, to distinguish it from other MDPs, it must be wrapped in an appropriate namespace:


//file ../models/models/place_holder/mdp.h

#pragma once

namespace DynaPlex::Models{
    //convention: MDP with id "place_holder" is defined in namespace DynaPlex::Models::place_holder. 
    namespace place_holder{
        class MDP{
          //Class definition.    
        }
    }
}

To ensure that the corresponding function definitions can be matched with this declaration, they should also be defined in the same namespace:


//file ../models/models/place_holder/mdp.cpp
#include "mdp.h"
#include "dynaplex/erasure/mdpregistrar.h"
#include "policies.h"

namespace DynaPlex::Models {
	namespace place_holder 
	{
         VarGroup MDP::GetStaticInfo() const
         {
         
         }
          //other function definitions
    }
}

If at a later stage you would want to add custom policies, ensure that these also are declared and defined in the same namespace (here namespace DynaPlex::Models::place_holder). 

/src/lib/models/models/lost_sales

Similarly, the MDP DynaPlex::Models::bin_packing::MDP is defined in files located in src/lib/models/models/mdp.cpp. (and declared in the corresponding .h file). It is registered under the id "lost_sales". 


# Other changes with respect to legacy DynaPlex

Models/MDPs are defined using very similar API as in the legacy DynaPlex library, but there are some important changes:


Guidelines for modelling:
Integers elements of states and mdps are preferably held as int64_t variables (see below).
For continuous variables, use double.

Actions have moved from size_t (unsigned) to int64_t (signed). These two are not mutually compatible. Reasons:
-easier interfacing with torch, which also uses int64_t at many places.
-int64_t is guaranteed same on all platforms, which simplifies interop.
-int64_t will enable the support of negative actions in future versions of dynaplex.
-by passing in an action as int64_t, and internally using int64_t for 

Default events are also int64_t, but manually overriding this is still possible.

Information on number of allowed actions, number of features, and alpha (now discount_factor) are no longer communicated via member functions NumFeatures() and NumValidActions() and member variable alpha. Instead
these are passed as part of a package of information provided by the MDP when GetStaticInfo() is called. This enables more flexibility towards the future, and has a number of other advantages. 

Every MPD should have a constructor that takes a VarGroup, which is a group of named vars. VarGroup is a bit like a json file, but does not support all json features. 
-Uniform construction enables the uniform creation of MDPs, and type erasure. I.e. a single function can be called to get any MDP, and there is a single type DynaPlex::MDP that can store every MDP. That makes algorithms easier to write, compilation faster, and bindings to python easier and more flexible.
-Incidentally, since the MDP is entirely determined by these vars, we can create a unique identifier from them. So no need anymore to manually implement and update GetIdentifier. 

VarGroup is used in many places in the code. You are required to write a conversion from MDP::State to VarGroup. This then also enables states to be written to console in a simple way, and also to convert them to native python objects, e.g. for debugging/rendering purposes. The reverse conversion also needs to be provided, see lost_sales.mdp. 

# registering an MDP model

All DynaPlex mdp models should be retrievable via DynaPlex::DynaPlexProvider (see e.g. src/tests/other_unit_tests/t_model_provider.cpp for syntax to retrieve an MDP). This also ensures they are retrievable in python. 

For this to work, a new MDP needs to be registered. Suppose you are adding a new mdp with id "place_holder". Registration consists of two simple steps:
1. In the file that defines the MDP (which should be src/lib/models/models/place_holder/mdp.cpp for consistency), the function 


# Trouble-shooting:

mdp_unit_tests\testutils.cpp(28): error: Expected: mdp = dp.GetMDP(mdp_vars_from_json); doesn't throw an exception.
  Actual: it throws DynaPlex::Error with description "DynaPlex: No MDP available with identifier "<some_identifier>". Use ListMDPs() / list_mdps() to obtain available MDPs.".

  Is the identifier <some_identifier> a correct identifier for an MDP that you added?
    Did you correctly register that mdp under the desired name in MDP::Register(DynaPlex::Registry& registry)?
    Did you update modelling/models/registrationmanager with the new MDP?


      Actual: it throws DynaPlex::Error with description "DynaPlex: MDP, id "<some_identifier>" : discount_factor is invalid: -6277438562204192487878988888393020692503707483087375482269988814848.000000. Must be in (0.0,1.0].".


      Did you correctly read the discount_factor from the provided VarGroup in the MDP::MDP(const VarGroup& vargroup) constructor?s