#include "policies.h"
#include "dynaplex/models/collaborative_picking/mdp.h"
#include "dynaplex/error.h"
namespace DynaPlex::Models {
	namespace collaborative_picking
	{

		ClosestPair::ClosestPair(std::shared_ptr<const MDP> mdp, const VarGroup& config)
			:mdp{ mdp }
		{			
		}

		int64_t ClosestPair::GetAction(const MDP::State& state) const
		{
			auto& first_event = state.scheduled_event_queue.first();
			if (!first_event.has<PickerAwaitsAction>())
				throw DynaPlex::Error("closest_pair: error in state flow logic.");
			auto& payload = first_event.get<PickerAwaitsAction>();
			
			int64_t picker_node = state.pickers[payload.picker_id].node_id;
			int64_t best_action = mdp->num_vehicles;

			int64_t best_pick_loc = -1;
			DynaPlex::Graph::WeightType minimum_distance = std::numeric_limits<DynaPlex::Graph::WeightType>::max();
			for (auto& [vehicle_index, vehicle] : state.vehicles)
			{
				if (vehicle.IsAssignable(payload.reassign))
				{
					auto pick_loc = vehicle.remaining_pick_list.front();
					auto distance = mdp->picker_graph.Distance(picker_node, pick_loc);
					if (distance < minimum_distance)
					{
						best_pick_loc = pick_loc;
						minimum_distance = distance;
						best_action = vehicle_index;
					}
				}
			}
			if (mdp->graph_feats){
				if (best_pick_loc < 0)
					best_action = picker_node;
				else
					best_action = best_pick_loc;
			}
			return best_action;
		}
	}
}