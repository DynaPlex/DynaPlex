#pragma once
#include <vector>
#include "dynaplex/error.h"
#include "dynaplex/state.h"
#include "stateadapter.h"
#include "mdpadapter.h"

namespace DynaPlex::Erasure
{
	//Two overloads provided to manually preserve expectations of 
	//not changing the underlying state if passed as const 
	template <typename t_MDP>
	const typename t_MDP::State& MDPAdapter<t_MDP>::ToState(const DynaPlex::dp_State& state) const
	{
		// Check that the states belong to this MDP
		if (state->mdp_int_hash != mdp_int_hash)
		{
			throw DynaPlex::Error("Error in MDP->ToState: It seems you tried to call with state(s) not created by this MDP, this should not be tried as it can lead to segmentation faults. ");
		}
		// Cast to the specific StatesAdapter type and access the underlying data
		StateAdapter<typename t_MDP::State>* stateAdapter = static_cast<StateAdapter<typename t_MDP::State>*>(state.get());
		return stateAdapter->state;
	}

	template <typename t_MDP>
	typename t_MDP::State& MDPAdapter<t_MDP>::ToState(DynaPlex::dp_State& state) const
	{
		// Check that the states belong to this MDP
		if (state->mdp_int_hash != mdp_int_hash)
		{
			throw ::DynaPlex::Error("Error in MDP->ToState: It seems you tried to call with state(s) not created by this MDP, this should not be tried as it can lead to segmentations faults. ");
		}
		// Cast to the specific StateAdapter type and access the underlying data
		StateAdapter<typename t_MDP::State>* stateAdapter = static_cast<StateAdapter<typename t_MDP::State>*>(state.get());
		return stateAdapter->state;
	}
}