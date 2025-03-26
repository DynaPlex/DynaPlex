#include <iostream>
#include "dynaplex/dynaplexprovider.h"

using namespace DynaPlex;


void TestReinitiation(DynaPlex::MDP mdp)
{

	auto& dp = DynaPlex::DynaPlexProvider::Get();
	//system << config.Dump(1) << std::endl;
	auto conf = DynaPlex::VarGroup{
		{"max_period_count",2000}
	};
	auto dem = dp.GetDemonstrator(conf);
	auto trace = dem.GetObjectTrace(mdp);

	auto& trace_element = trace.at(trace.size() - 1);
	std::vector<DynaPlex::Trajectory> trajectories{};

	for (size_t i = 0; i < 10; i++)
	{
		trajectories.push_back(std::move(DynaPlex::Trajectory(i)));
		trajectories[i].RNGProvider.SeedEventStreams(false, i);
	}


	mdp->InitiateState(trajectories, trace_element.state);
	std::cout << trace_element.state->ToVarGroup().Dump(1) << std::endl;
	for (auto& traj : trajectories)
	{
		auto vg = traj.GetState()->ToVarGroup();
		std::vector<DynaPlex::VarGroup> jobs;
		vg.Get("scheduled_event_queue", jobs);

		for (auto& job : jobs)
			std::cout << job.Dump(1) << std::endl;
		std::cout << std::endl;
	}
}


int main() {
	auto& dp = DynaPlex::DynaPlexProvider::Get();
	auto& system = dp.System();
	auto path_to_json = dp.FilePath({ "mdp_config_examples", "resource_allocation" }, "mdp_config_1.json");

	auto config = VarGroup::LoadFromFile(path_to_json);

	DynaPlex::MDP mdp = dp.GetMDP(config);
	
	auto policy = mdp->GetPolicy("shortest_processing_time");
	DynaPlex::VarGroup nn_training{
		{"early_stopping_patience",15},
		{"mini_batch_size", 64},
		{"max_training_epochs", 1000},
		{"train_based_on_probs", false}
	};

	DynaPlex::VarGroup nn_architecture{
		{"type","mlp"},//mlp - multi-layer-perceptron. 
		{"hidden_layers",DynaPlex::VarGroup::Int64Vec{128,64}}//Note: Input/output layer sizes are determined by MDP. 
	};
	int64_t num_gens = 1;

	DynaPlex::VarGroup dcl_config{
		//use defaults everywhere. 
		{"N",3000},//number of samples
		{"num_gens",num_gens},//number of neural network generations.
		{"M",200},//rollouts per action, default is 1000. 
		{"H",30 },//horizon, i.e. number of steps for each rollout.
		{"nn_architecture",nn_architecture},
		{"nn_training",nn_training},
		{"enable_sequential_halving",true},
		{"retrain_lastgen_only",false},
		{"resume_gen",0}
	};
	
	try
	{
		auto dcl = dp.GetDCL(mdp, policy, dcl_config);
		dcl.TrainPolicy();
		auto policies = dcl.GetPolicies();
		//Compare the various trained policies:
		auto comparer = dp.GetPolicyComparer(mdp, DynaPlex::VarGroup{
			{"number_of_trajectories",512}
			});
		auto comparison = comparer.Compare(policies);
		for (auto& VarGroup : comparison)
		{
			std::cout << VarGroup.Dump() << std::endl;
		}
	}
	catch (const std::exception& e)
	{
		std::cout << "exception: " << e.what() << std::endl;
	}
	return 0;
}