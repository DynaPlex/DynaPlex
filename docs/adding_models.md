## Adding Models

To Do: add instruction for adding models. 

Trouble-shooting:

mdp_unit_tests\testutils.cpp(28): error: Expected: mdp = dp.GetMDP(mdp_vars_from_json); doesn't throw an exception.
  Actual: it throws DynaPlex::Error with description "DynaPlex: No MDP available with identifier "<some_identifier>". Use ListMDPs() / list_mdps() to obtain available MDPs.".

  Is the identifier <some_identifier> a correct identifier for an MDP that you added?
    Did you correctly register that mdp under the desired name in MDP::Register(DynaPlex::Registry& registry)?
    Did you update modelling/models/registrationmanager with the new MDP?


      Actual: it throws DynaPlex::Error with description "DynaPlex: MDP, id "<some_identifier>" : discount_factor is invalid: -6277438562204192487878988888393020692503707483087375482269988814848.000000. Must be in (0.0,1.0].".


      Did you correctly read the discount_factor from the provided VarGroup in the MDP::MDP(const VarGroup& vargroup) constructor?s