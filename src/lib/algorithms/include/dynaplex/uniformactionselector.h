#pragma once
#include "dynaplex/mdp.h"
#include "dynaplex/policy.h"
#include "dynaplex/system.h"
#include "dynaplex/vargroup.h"
#include "dynaplex/sample.h"

namespace DynaPlex::DCL {
	class UniformActionSelector {

	
	public:
		UniformActionSelector() = default;
		UniformActionSelector(int64_t rng_seed, int64_t H, int64_t M, DynaPlex::MDP&, DynaPlex::Policy&);

		void SetAction(DynaPlex::Trajectory& traj, DynaPlex::NN::Sample& sample, int64_t seed) const;


	

	private:
		int64_t rng_seed;
		int64_t H, M;
		DynaPlex::Policy policy;
		DynaPlex::MDP mdp;

	};
}//namespace DynaPlex::Utilities