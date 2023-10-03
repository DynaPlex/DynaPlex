#pragma once
#include "dynaplex/mdp.h"
#include "dynaplex/policy.h"
#include "dynaplex/system.h"
#include "dynaplex/vargroup.h"
namespace DynaPlex::Utilities {
	class Demonstrator {
	public:
		/**
		 *Config may include max_event_count (default:3), which will will be used when calling GetTrace. 
		 * it may also include rng_seed (default:0). 
		 */
		Demonstrator(const DynaPlex::System& system, const VarGroup& config = VarGroup{});


		/**
		 * Returns a trace, i.e. a sequence of states (converted ToVarGroup()), starting from an initial state,
		 * until reaching a final state or until max_period_count periods have passed. 
		 */
		std::vector<VarGroup> GetTrace(DynaPlex::MDP mdp, DynaPlex::Policy policy = nullptr);

	private:
		int64_t max_period_count;
		int64_t rng_seed;
		System system;


	};
}//namespace DynaPlex