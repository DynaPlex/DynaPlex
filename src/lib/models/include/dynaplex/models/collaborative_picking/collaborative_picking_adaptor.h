#include "dynaplex/models/collaborative_picking/mdp.h"
#include "dynaplex/retrievemdp.h"

namespace CP = DynaPlex::Models::collaborative_picking;
namespace DynaPlex::Adaptors {


	class collaborative_picking_adaptor {
		DynaPlex::MDP mdp;

		DynaPlex::RNG rng{ false, 26071983 };
		int64_t warehouse_width, warehouse_height, num_nodes;

		CP::MDP::State state{};
		bool focal_picker_set{ false };

	public:
		DynaPlex::VarGroup StateAsVarGroup()
		{
			return state.ToVarGroup();
		}

		int64_t node_at(int64_t row, int64_t col)
		{
			return row * warehouse_width + col;
		}

		collaborative_picking_adaptor(DynaPlex::MDP mdp) :mdp{ mdp }
		{
			auto static_info = mdp->GetStaticInfo();

			static_info.Get("warehouse_width", warehouse_width);
			static_info.Get("warehouse_height", warehouse_height);
			static_info.Get("num_nodes", num_nodes);
			// our policies are time invariant, so time should not matter. But set it non-negative, just to be sure.
			state.current_time = 0;
			state.nodes = std::vector<CP::Node>(num_nodes, CP::Node{});
		}

		//-1 means no vehicle assigned.
		int64_t add_picker(int64_t row, int64_t col, int64_t assigned_vehicle_id = -1)
		{
			auto& [picker_id, picker] = state.pickers.AddNew();
			picker.node_id = node_at(row, col);
			picker.assigned_vehicle_id = assigned_vehicle_id;
			if (assigned_vehicle_id >= 0)
			{
				auto& vehicle = state.vehicles[assigned_vehicle_id];
				vehicle.assigned_picker = picker_id;
			}
			return picker_id;
		}

		int64_t select_vehicle(Policy policy)
		{
			//create a trajectory that enables actions to be set.
			DynaPlex::Trajectory traj{};
			traj.RNGProvider.SeedEventStreams(false, 12);
			auto dp_state = mdp->GetState(StateAsVarGroup());
			mdp->InitiateState({ &traj,1 }, dp_state);

			policy->SetAction({ &traj,1 });
			auto underlying_mdp = DynaPlex::RetrieveMDP<CP::MDP>(mdp);
			return underlying_mdp->GetSelectedVehicleIndex(state, traj.NextAction);

		}

		int64_t add_vehicle(int64_t row, int64_t col, std::vector<int64_t> remaining_pick_list, bool at_pick_location, bool is_blocked, bool is_dropping_off, bool is_being_picked_for)
		{
			auto& [vehicle_id, vehicle] = state.vehicles.AddNew();
			vehicle.node_id = node_at(row, col);
			vehicle.at_pick_location = at_pick_location;
			vehicle.is_blocked = is_blocked;
			vehicle.is_dropping_off = is_dropping_off;
			vehicle.is_being_picked_for = is_being_picked_for;
			for (auto pick : remaining_pick_list)
				vehicle.remaining_pick_list.push_back(pick);
			//since this is unlikely to affect anything, let's just randomly set the truck_dock index and troch dock. 
			auto underlying_mdp = DynaPlex::RetrieveMDP<CP::MDP>(mdp);
			vehicle.drop_off_truck_doc_index = rng.genUniform() * underlying_mdp->truck_docks.size();
			vehicle.drop_off_node_id = underlying_mdp->truck_docks[vehicle.drop_off_truck_doc_index].node_id;
			//update the information around the node:
			auto& node = state.nodes[vehicle.node_id];
			if (!node.is_accessible)
				throw DynaPlex::Error("collaborative_picking_state_creator::add_vehicle - node allready contains a vehicle.");
			node.is_accessible = false;
			node.occupying_vehicle_id = vehicle_id;
			return vehicle_id;
		}

		void set_focal_picker(int64_t picker_id, bool reassign = false)
		{
			if (focal_picker_set)
				throw DynaPlex::Error("Focal picker has allready been set");
			focal_picker_set = true;
			state.Schedule(0, CP::PickerAwaitsAction(picker_id, reassign));
			state.cat = StateCategory::AwaitAction(picker_id + 1);
		}

	};

}