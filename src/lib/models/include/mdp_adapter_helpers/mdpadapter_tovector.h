#pragma once
#include <vector>
#include "dynaplex/mdpadapter.h"
template <typename t_MDP>
std::vector<typename t_MDP::State>& MDPAdapter<t_MDP>::ToVector(DynaPlex::States& states) const
{
	// Check that the states belong to this MDP
	if (states->mdp_int_hash != mdp_int_hash)
	{
		throw DynaPlex::Error("Error in MDP->ToVector: It seems you tried to call with states not created by this MDP");
	}
	// Cast to the specific StatesAdapter type and access the underlying data
	StatesAdapter<typename t_MDP::State>* statesAdapter = static_cast<StatesAdapter<typename t_MDP::State>*>(states.get());
	return statesAdapter->get();
}

template <typename t_MDP>
const std::vector<typename t_MDP::State>& MDPAdapter<t_MDP>::ToVector(const DynaPlex::States& states) const
{
	// Check that the states belong to this MDP
	if (states->mdp_int_hash != mdp_int_hash)
	{
		throw DynaPlex::Error("Error in MDP->ToVector: It seems you tried to call with states not created by this MDP");
	}
	// Cast to the specific StatesAdapter type and access the underlying data
	const StatesAdapter<typename t_MDP::State>* statesAdapter = static_cast<const StatesAdapter<typename t_MDP::State>*>(states.get());
	return statesAdapter->get();
}