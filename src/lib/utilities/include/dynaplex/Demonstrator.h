#pragma once
#include "dynaplex/mdp.h"
#include "dynaplex/policy.h"
#include "dynaplex/system.h"
#include "dynaplex/vargroup.h"
namespace DynaPlex::Utilities {
	class Demonstrator {
		/**
		 *VarGroup must provide in64_t MaxEventCount, which will will be used when calling GetTrace. 
		 */
		Demonstrator(DynaPlex::System& system, VarGroup& varGroup);


		/**
		 * Returns a trace, i.e. a sequence of states (converted ToVarGroup()), starting from an initial state,
		 * until reaching a final state or until MaxEventCount events have occured. 
		 */
		VarGroup GetTrace(DynaPlex::MDP mdp, DynaPlex::Policy policy);


		


	};
}//namespace DynaPlex