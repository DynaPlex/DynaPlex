#pragma once
#include <string>
#include "dynaplex/mdp.h"

namespace DynaPlex {	

	class TrainedPolicyProvider {
		
	public:
		
		//Attempts to load a policy from the mentioned path. 
		static DynaPlex::Policy LoadPolicy(DynaPlex::MDP mdp, std::string path_to_policy_without_extension);
		//Attempts to save the policy, assuming it is a neural network policy.
		static void SavePolicy(DynaPlex::Policy, std::string path_to_policy_without_extension);
	};

}//namespace DynaPlex


