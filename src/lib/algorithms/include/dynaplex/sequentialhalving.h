#pragma once
#include "dynaplex/mdp.h"
#include "dynaplex/policy.h"
#include "dynaplex/system.h"
#include "dynaplex/vargroup.h"
namespace DynaPlex::DCL {
	/**
  * Sequential Halving algorithm. 
  * A state-of-the-art bandit algorithm for selecting the best alternative out of others.
  * Automatically kept up-to-date with calls to MDP->func(Trajectories, ...).
  * See the original paper: https://proceedings.mlr.press/v28/karnin13.pdf
  */
	class SequentialHalving {


	public:
		SequentialHalving() = default;
		SequentialHalving(uint32_t rng_seed, int64_t H, int64_t M, DynaPlex::MDP&, DynaPlex::Policy&);

		void SetAction(DynaPlex::Trajectory& traj, const int32_t seed) const;




	private:
		uint32_t rng_seed;
		int64_t H, M;
		DynaPlex::Policy policy;
		DynaPlex::MDP mdp;

	};
}//namespace DynaPlex::Utilities