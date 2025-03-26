#include <iostream>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/models/collaborative_picking/collaborative_picking_adaptor.h"
using namespace DynaPlex;

static int test_adaptor() {
	try {
		auto& dp = DynaPlex::DynaPlexProvider::Get();
		auto& system = dp.System();
		auto path_to_json = dp.FilePath({ "mdp_config_examples", "collaborative_picking" }, "mdp_config_3.json");
		//auto path_to_json = dp.FilePath({ "mdp_config_examples", "collaborative_picking" }, "mdp_config_4.json");
		auto config = VarGroup::LoadFromFile(path_to_json);
		DynaPlex::MDP mdp = dp.GetMDP(config);
		auto policy = mdp->GetPolicy("closest_pair");
		int64_t v1, v2, v3, v4;
		Adaptors::collaborative_picking_adaptor adaptor(mdp);
		{
			//set at least next pick
			std::vector<int64_t> remaining_pick_list = { adaptor.node_at(19,6) };
			int64_t row = 14, col = 5;
			//whether the vehicle is allready at the pick location:
			bool at_pick_location = false;
			bool is_blocked = false;
			bool is_dropping_off = false;
			//whether the pick (of the first element in remaining pick list) is allready going on, i.e. whether
			//the picker is allready picking it.
			bool is_being_picked_for = false;
			v1 = adaptor.add_vehicle(row, col, remaining_pick_list, at_pick_location, is_blocked, is_dropping_off, is_being_picked_for);
		}
		{
			std::vector<int64_t> remaining_pick_list = { adaptor.node_at(14,13) };
			int64_t row = 10, col = 13;
			v2 = adaptor.add_vehicle(row, col, remaining_pick_list, false, false, false, false);
		}
		{
			std::vector<int64_t> remaining_pick_list = { adaptor.node_at(11,9) };
			int64_t row = 12, col = 9;
			v3 = adaptor.add_vehicle(row, col, remaining_pick_list, false, false, false, false);
		}
		{
			//no picks remaining for this vehicle <->
			std::vector<int64_t> remaining_pick_list = {};
			int64_t row = 5, col = 21;
			bool at_pick_location = false;
			bool is_blocked = false;
			//So we are heading towards drop-off location:
			bool is_dropping_off = true;
			bool is_being_picked_for = false;
			v4 = adaptor.add_vehicle(row, col, remaining_pick_list, at_pick_location, is_blocked, is_dropping_off, is_being_picked_for);
		}
		//note that value of v3 should be 2, as indexes start at 0 and are incremented every insert.
		adaptor.add_picker(9, 9, v3);
		//note that this will be 1, as the id start at 0 and are incremented every time. 
		//-1 means that this picker is currently not assigned to a vehicle/pick.
		auto focal_picker = adaptor.add_picker(8, 10, -1);
		//reassign=false means that it will allow to assign to any unassigned vehicle, including vehicles not yet at the
		// pick location. (IF reassign = true, then that means the present picker is being reassigned after a vehicle did
		//not show up. to prevent gridlocks, in those cases we like to assign to a vehicle that is actually at the pick location allready.)
		bool reassign = false;
		//we want a pick for this picker..
		adaptor.set_focal_picker(focal_picker, reassign);
		std::cout << "Selected: vehicle " << adaptor.select_vehicle(policy) << std::endl;
		//Note that for a new selection, you would need to make a new adaptor..
		return 0;
	}
	catch (const std::exception& e)
	{
		std::cout << "exception: " << e.what() << std::endl;
	}
}

static int test_model() {

	auto& dp = DynaPlex::DynaPlexProvider::Get();
	auto& system = dp.System();
	//auto path_to_json = dp.FilePath({ "mdp_config_examples", "collaborative_picking" }, "mdp_config_3.json");
	//auto path_to_json = dp.FilePath({ "mdp_config_examples", "collaborative_picking" }, "mdp_config_8.json");
	auto path_to_json = dp.FilePath({ "mdp_config_examples", "collaborative_picking" }, "mdp_config_non_stat.json");
	//auto path_to_json = dp.FilePath({ "mdp_config_examples", "resource_allocation" }, "mdp_config_1.json");

	auto config = VarGroup::LoadFromFile(path_to_json);

	DynaPlex::MDP mdp = dp.GetMDP(config);

	auto policy = mdp->GetPolicy("closest_pair");
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
		//{"N",1000},//number of samples
		{"N",5},//number of samples
		{"num_gens",num_gens},//number of neural network generations.
		{"M",10},//rollouts per action, default is 1000. 
		//{"M",50},//rollouts per action, default is 1000. 
		{"H",100 },//horizon, i.e. number of steps for each rollout.
		{"nn_architecture",nn_architecture},
		{"nn_training",nn_training},
		{"enable_sequential_halving",true},
		{"retrain_lastgen_only",false},
		{"resume_gen",0}
	};

	//try
	//{
	//	auto dcl = dp.GetDCL(mdp, policy, dcl_config);
	//	dcl.TrainPolicy();
	//	auto policies = dcl.GetPolicies();
	//	//Compare the various trained policies:
	//	auto comparer = dp.GetPolicyComparer(mdp, DynaPlex::VarGroup{
	//		{"number_of_trajectories",512}
	//		});
	//	auto comparison = comparer.Compare(policies);
	//	for (auto& VarGroup : comparison)
	//	{
	//		std::cout << VarGroup.Dump() << std::endl;
	//	}
	//}
	//catch (const std::exception& e)
	//{
	//	std::cout << "exception: " << e.what() << std::endl;
	//}

	DynaPlex::Policy random_policy = mdp->GetPolicy("random");
	DynaPlex::Policy heuristic_policy = mdp->GetPolicy("closest_pair");

	VarGroup comparerConfig;
	comparerConfig.Add("number_of_trajectories", 10);
	comparerConfig.Add("periods_per_trajectory", 1800);

	auto comparer = dp.GetPolicyComparer(mdp, comparerConfig);

	auto comparison = comparer.Compare({ random_policy, heuristic_policy });
	//auto comparison = comparer.Compare({ heuristic_policy });
	//auto comparison = comparer.Compare({ random_policy });
	for (auto& VarGroup : comparison)
	{
		std::cout << VarGroup.Dump() << std::endl;
	}

	return 0;
}

int main() {

	//test_adaptor();

	test_model();

	return 0;
}