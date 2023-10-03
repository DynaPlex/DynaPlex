#pragma once
#include "dynaplex/mdp.h"
#include "dynaplex/policy.h"
#include "dynaplex/system.h"
#include "dynaplex/vargroup.h"
namespace DynaPlex::Utilities {
	class PolicyComparer {

		template<bool SkipTrivialActions=false>
		void Evolve(const DynaPlex::Policy& policy, std::span<DynaPlex::Trajectory>, int64_t) const;

		void CheckTrajectoriesInfiniteHorizon(std::span<DynaPlex::Trajectory>, int64_t) const;
		void CheckTrajectoriesFiniteHorizon(std::span<DynaPlex::Trajectory>) const;

		void ComputeReturns(std::span<double>& ReturnPerTrajectory, const DynaPlex::Policy& policy, int64_t offset) const;

	public:
		/**
		 * Config may include number_of_trajectories (default:4096 for infinite horizon mdps; 16384 for finite horizon mdps).  
		 * If mdp is infinite horizon, undiscounted: config may include warmup_periods (default: 128), periods_per_trajectory (default: 1024). 
		 * If mdp is infinite horizon, discounted: config may include periods_per_trajectory (default: 1024). 
		 * If mdp is finite horizon: config may include max_periods_until_error (default: 16384), this is the maximum number of steps in a trajectory until
		 * mdp is expected to terminate by reaching final state. 
		 * Config may also include rng_seed (default 0). 
		 */
		PolicyComparer(const DynaPlex::System& system, DynaPlex::MDP mdp, const DynaPlex::VarGroup& config = VarGroup{});


		/**
		 * @brief Assesses a policy by evaluating the return averaged over a number of trajectories.
		 */
		VarGroup Assess( DynaPlex::Policy policy) const;
		/**
		 * @brief Assesses two policies by evaluating the return averaged over a number of trajectories.
		 */
		std::vector<VarGroup> Compare(DynaPlex::Policy first, DynaPlex::Policy second, int64_t index_of_benchmark = -1) const;
		/**
         * @brief Assesses various policies by evaluating the return averaged over a number of trajectories.
		 * 
		 * @param index_of_benchmark Index of the policy that will be used as a benchmarks -- all other performance will be reported relative to the benchmark. Defaults to -1 in which case
		 * absolute performance of all policies is returned. 
         */
		std::vector<VarGroup> Compare(std::vector<DynaPlex::Policy> policies, int64_t index_of_benchmark = -1) const;

	private:
		int64_t number_of_trajectories, periods_per_trajectory, warmup_periods, max_periods_until_error, rng_seed;
		DynaPlex::MDP mdp;
		System system;


	};
}//namespace DynaPlex::Utilities