#pragma once
#include "dynaplex/mdp.h"
#include "dynaplex/policy.h"
#include "dynaplex/system.h"
#include "dynaplex/vargroup.h"
namespace DynaPlex::Utilities {
	class Demonstrator {
	public:
		/**
		 *VarGroup must provide int64_t max_event_count, which will will be used when calling GetTrace. 
		 * it may also provide seed. 
		 */
		Demonstrator(const DynaPlex::System& system, const VarGroup& config);


		/**
		 * Returns a trace, i.e. a sequence of states (converted ToVarGroup()), starting from an initial state,
		 * until reaching a final state or until MaxEventCount events have occured. 
		 */
		std::vector<VarGroup> GetTrace(DynaPlex::MDP mdp, DynaPlex::Policy policy = nullptr);

	private:
		int64_t max_event_count;
		int64_t seed;
		System system;


	};
}//namespace DynaPlex