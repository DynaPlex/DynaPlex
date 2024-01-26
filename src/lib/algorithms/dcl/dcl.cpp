#include "dynaplex/dcl.h"
#include "dynaplex/parallel_execute.h"
#include "dynaplex/policytrainer.h"
#include "dynaplex/sampledata.h"
#include "dynaplex/sample.h"


namespace DynaPlex::Algorithms {


	DCL::DCL(const DynaPlex::System& system, DynaPlex::MDP mdp, DynaPlex::Policy policy_0, const VarGroup& config)
		: system{ system }, mdp{ mdp }, policy_0{ nullptr }, sampleCollector(system,mdp,config)
	{
		if (!mdp)
			throw DynaPlex::Error("DCL: mdp should not be null");
		config.GetOrDefault("rng_seed", rng_seed, 15112017);
		if (rng_seed < 0)
			throw DynaPlex::Error("DCL :: Invalid rng_seed - should be non-negative");
		config.GetOrDefault("silent", silent, false);
		config.GetOrDefault("retrain_lastgen_only", retrain_lastgen_only, false);
		config.GetOrDefault("resume_gen", resume_gen,0);
		config.GetOrDefault("num_gens", num_gens, 1);

		//initiate policy_0, defaulting to random. 
		if (policy_0)
			this->policy_0 = policy_0;
		else
			this->policy_0 = mdp->GetPolicy("random");


		VarGroup nn_training;
		if (config.HasKey("nn_training"))
			config.Get("nn_training", nn_training);

		trainer = DynaPlex::NN::PolicyTrainer(system, mdp, nn_training,rng_seed);

		if (config.HasKey("nn_architecture"))
			config.Get("nn_architecture", nn_architecture);
		else
		{//default to neural network directly connecting input to output, i.e. logit model:
			nn_architecture = DynaPlex::VarGroup{};
			nn_architecture.Add("type", "mlp");
			nn_architecture.Add("hidden_layers", DynaPlex::VarGroup::Int64Vec{});
		}

	}


	void DCL::TrainPolicy() {

		if (retrain_lastgen_only)
		{
			if(system.WorldRank()==0)
				trainer.TrainPolicy(nn_architecture, num_gens, GetPathOfSampleFile(num_gens - 1),silent);
		}
		else
		{
			for (int64_t generation = resume_gen; generation < num_gens; generation++) {
				DynaPlex::Policy policy = GetPolicy(generation);
				sampleCollector.GenerateStateSamples(policy, GetPathOfSampleFile(generation));
				if(!silent)
					system << "Elapsed time: " << system.Elapsed() << std::endl;
				if(system.WorldRank()==0)
					trainer.TrainPolicy(nn_architecture, generation + 1, GetPathOfSampleFile(generation),silent);		
				system.AddBarrier();
			}
		}
	}


	std::vector<DynaPlex::Policy> DCL::GetPolicies() {
		std::vector<DynaPlex::Policy> policies;
		for (int64_t generation = 0; generation <= num_gens; generation++)
		{
			policies.push_back(GetPolicy(generation));
		}
		return policies;
	}

	DynaPlex::Policy DCL::GetPolicy(int64_t generation)
	{
		if (generation == -1)
			generation = num_gens;

		if (generation < 0 || generation>num_gens)
			throw DynaPlex::Error("Invalid generation requested: " + std::to_string(generation) + "/" + std::to_string(generation));

		if (generation == 0)
			return policy_0;

		return trainer.LoadPolicy(nn_architecture, generation);
	}


	std::string DCL::GetPathOfSampleFile(int64_t generation)
	{
		std::string filename = "samples_gen" + std::to_string(generation);
		filename += ".json";
		return this->system.filepath(this->mdp->Identifier(), filename);
	}
}