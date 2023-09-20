## Models

DynaPlex facilitates extension with new native models and policies. This document explains how. 


# Understanding what is provided 
For starters, review the implementations under src/lib/models/models, e.g. in src/lib/models/models/lost_sales.

The lost_sales model provides both a native mdp (in lost_sales/mdp.h and lost_sales/mdp.cpp) and a native policy (in lost_sales/policies.h and lost_sales/policies.cpp). One may define an MDP without native policies, i.e. leaving out the policies.* files. Standard policies (like "random") and 
learned policies will be available anyhow. Also some json files are provded, those are default configuration files for testing. They also help in getting a quick idea what configuration parameters an MDP allows.




# Summary of changes with respect to legacy DynaPlex

Models are defined using very similar API as in the legacy DynaPlex library, but there are some important changes:

Guidelines for modelling:
Integers elements of states and mdps are preferably held as int64_t variables (see below).
For continuous variables, use double.

Actions have moved from size_t (unsigned) to int64_t (signed). These two are not mutually compatible. Reasons:
-easier interfacing with torch, which also uses int64_t at many places.
-int64_t is guaranteed same on all platforms, which simplifies interop.
-int64_t will enable the support of negative actions in future versions of dynaplex.
-by passing in an action as int64_t, and internally using int64_t for 

Default events are also int64_t, but manually overriding this is still possible.

Information on number of allowed actions, number of features, and alpha (discount factor) are no longer communicated via member functions NumFeatures() and NumValidActions() and member variables. Instead
these are passed as part of a package of information provided by the MDP when GetStaticProperties is provided. This enables more flexibility towards the future, e.g. for MDPs where states have a variable number of features. 

Every MPD should have a constructor that takes a VarGroup, which is a group of named vars. VarGroup is a bit like a json file, but does not support all json features. 
-Uniform construction enables the uniform creation of MDPs, and type erasure. I.e. a single function can be called to get any MDP, and there is a single type DynaPlex::MDP that can store every MDP. That makes algorithms easier to write, compilation faster, and bindings to python easier and more flexible.
-Incidentally, since the MDP is entirely determined by these vars, we can create a unique identifier from them. So no need anymore to manually implement and update GetIdentifier. 

VarGroup is used in many places in the code. You are required to write a conversion from MDP::State to VarGroup. This then also enables states to be written to console in a simple way, and also to convert them to native python objects, e.g. for debugging/rendering purposes. The reverse conversion also needs to be provided, see lost_sales.mdp. 


# Trouble-shooting:

mdp_unit_tests\testutils.cpp(28): error: Expected: mdp = dp.GetMDP(mdp_vars_from_json); doesn't throw an exception.
  Actual: it throws DynaPlex::Error with description "DynaPlex: No MDP available with identifier "<some_identifier>". Use ListMDPs() / list_mdps() to obtain available MDPs.".

  Is the identifier <some_identifier> a correct identifier for an MDP that you added?
    Did you correctly register that mdp under the desired name in MDP::Register(DynaPlex::Registry& registry)?
    Did you update modelling/models/registrationmanager with the new MDP?


      Actual: it throws DynaPlex::Error with description "DynaPlex: MDP, id "<some_identifier>" : discount_factor is invalid: -6277438562204192487878988888393020692503707483087375482269988814848.000000. Must be in (0.0,1.0].".


      Did you correctly read the discount_factor from the provided VarGroup in the MDP::MDP(const VarGroup& vargroup) constructor?s