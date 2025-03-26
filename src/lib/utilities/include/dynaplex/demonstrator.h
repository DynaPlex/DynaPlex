#pragma once
#include "dynaplex/mdp.h"
#include "dynaplex/policy.h"
#include "dynaplex/system.h"
#include "dynaplex/vargroup.h"
namespace DynaPlex::Utilities {

	class TraceElement {
	public:

		DynaPlex::dp_State state;      
		DynaPlex::StateCategory cat;
		int64_t period_count{ 0 };               
		double incr_return{ 0.0 };              
		double cum_return{ 0.0 };               
		int64_t action{ 0 };         

		VarGroup ToVarGroup() const;
	};

	class Demonstrator {
	public:
		/**
		 *Config may include max_event_count (default:3)
		 * it may also include rng_seed (default:11112014). 
		 */
		Demonstrator(const DynaPlex::System& system, const VarGroup& config = VarGroup{});


		/**
		 * Returns a trace, i.e. a sequence of states, starting from an initial state,
		 * until reaching a final state or until max_period_count periods have passed.
	     */
		std::vector<TraceElement> GetObjectTrace(DynaPlex::MDP mdp, DynaPlex::Policy policy = nullptr);

		/**
		 * Returns a trace, i.e. a sequence of states (converted ToVarGroup()), starting from an initial state,
		 * until reaching a final state or until max_period_count periods have passed. 
		 */
		std::vector<VarGroup> GetTrace(DynaPlex::MDP mdp, DynaPlex::Policy policy = nullptr);

	private:
		int64_t max_period_count;
		int64_t rng_seed;
		bool restricted_statecategory{ false };
		DynaPlex::StateCategory cat;
		System system;


	};
}//namespace DynaPlex