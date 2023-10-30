#pragma once
#include "dynaplex/mdp.h"
#include "dynaplex/sampledata.h"
namespace DynaPlex::NN
{
	class PolicyTrainer {

		std::string PathToPolicy(DynaPlex::VarGroup nn_architecture, int64_t generation);
	public:
		PolicyTrainer(const DynaPlex::System&, DynaPlex::MDP,const DynaPlex::VarGroup& training_config);
		PolicyTrainer() = default;
		void TrainPolicy(DynaPlex::VarGroup nn_architecture, int64_t generation, std::string path_to_sample_data, bool silent=false);
		DynaPlex::Policy LoadPolicy(DynaPlex::VarGroup nn_architecture, int64_t generation);

	private:
		DynaPlex::System system;
		DynaPlex::MDP mdp;
		int64_t mini_batch_size;
		int64_t early_stopping_patience;
		int64_t max_training_epochs;
	};
}//DynaPlex::NN