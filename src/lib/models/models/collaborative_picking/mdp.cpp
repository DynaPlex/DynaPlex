#include "dynaplex/models/collaborative_picking/mdp.h"
#include "dynaplex/erasure/mdpregistrar.h"
#include "policies.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace DynaPlex::Models {
	namespace collaborative_picking
	{
		VarGroup MDP::GetStaticInfo() const
		{
			int64_t num_actions;
			if (graph_feats)
				num_actions = vehicle_graph.NumNodes();
			else
				num_actions = num_vehicles + 1;

			auto vg = DynaPlex::VarGroup{
				{"valid_actions",num_actions},
				{ "config_info",config_info}
			};
			vg.Add("warehouse_width", vehicle_graph.Width());
			vg.Add("warehouse_height", vehicle_graph.Height());
			vg.Add("num_nodes", vehicle_graph.NumNodes());
			return vg;
		}



		void MDP::InitiatePick(State& state, Picker& picker, Vehicle& vehicle, RNG& rng) const
		{
			if (vehicle.is_being_picked_for)
				throw DynaPlex::Error("error in vehicle picking assignment logic");


			//update info and schedule pick completion event:
			vehicle.is_being_picked_for = true;
			picker.num_picks_started++;
			auto trigger_time = state.current_time + pick_times[state.current_distribution_id].GetSample(rng);
			//auto trigger_time = state.current_time + pick_time.GetSample(rng);
			state.Schedule(trigger_time, PickCompletes(vehicle.assigned_picker));
		}
		void MDP::VehicleContinueTravel(State& state, int64_t& vehicle_id, int64_t& destination, RNG& rng) const
		{
			auto& vehicle = state.vehicles[vehicle_id];
	
			auto& next_node_index = vehicle_graph.NextEdge(vehicle.node_id, destination).dest;
			auto& next_node = state.nodes[next_node_index];

			if (next_node.is_accessible)
			{
				if (!next_node.node_entry_queue.IsEmpty())
					throw DynaPlex::Error("Node is accessible but queue is not empty");
				next_node.node_entry_queue.push_back(vehicle_id);
				int64_t entry_time = state.current_time + vehicle_travel_time.GetSample(rng);
				state.Schedule(entry_time, VehicleNodeEntry(next_node_index));
				//Drop-off nodes are always accessible to prevent conflicts
				if (next_node_index != vehicle.drop_off_node_id)
					next_node.is_accessible = false;
			}
			else {
				next_node.node_entry_queue.push_back(vehicle_id);
				vehicle.is_blocked = true;
			}
		}

		double MDP::ModifyStateWithEvent(State& state, DynaPlex::RNG& rng) const
		{
			ScheduledEvent first_event = state.scheduled_event_queue.first();
			state.scheduled_event_queue.pop();
			if (first_event.trigger_time < state.current_time)
				throw DynaPlex::Error("resource_allocation::ModifyStateWithEvent - time is decreasing");

			double costs = 0.0;
			state.current_time = first_event.trigger_time;
			switch (first_event.PayLoadIndex())
			{
			case ScheduledEvent::IndexOf<TimeStep>():
			{
				if (minimize_idling_objective) {
					int64_t num_picking{ 0 };
					for (const auto& [picker_index, picker] : state.pickers) {
						if (picker.assigned_vehicle_id >= 0)
						{
							auto& vehicle = state.vehicles[picker.assigned_vehicle_id];
							if (vehicle.is_being_picked_for)
								num_picking++;
						}
					}
					costs += static_cast<double>((num_pickers - num_picking)) / num_pickers;
				}
				state.Schedule(first_event.trigger_time + timestep_delta, TimeStep());
				break;
			}
			case ScheduledEvent::IndexOf<MakeNodeAccessible>():
			{
				auto& payload = first_event.get<MakeNodeAccessible>();
				auto& node = state.nodes.at(payload.node_id);

				//This should only happen for drop-off locations
				//If it happens elsewhere, it is a problem, most likely due to the vacating time being shorter than the vehicle travel time
				if (node.occupying_vehicle_id != -1)
					throw DynaPlex::Error("ModifyStateWithEvent : cannot make an occupied node accessible");
			
				if (!node.node_entry_queue.IsEmpty())
				{//if there are vehicles that are waiting to enter this node, then the first such vehicle will start moving towards this node. 
				 //as this vehicle will be accessing the node, the node is not available for other vehicles anymore, hence remains inaccessible.
					auto& vehicle = state.vehicles[node.node_entry_queue.front()];
					vehicle.is_blocked = false;
					int64_t entry_time = state.current_time + vehicle_travel_time.GetSample(rng);
					state.Schedule(entry_time, VehicleNodeEntry(payload.node_id));
				}
				else
					//if there is no vehicle waiting to enter this node, then it is accessible again. 
					node.is_accessible = true;
				break;
			}
			case ScheduledEvent::IndexOf<VehicleReadyForPick>():
			{
				auto& payload = first_event.get<VehicleReadyForPick>();
				auto& vehicle = state.vehicles[payload.vehicle_id];
			
				if (vehicle.at_pick_location)
					throw DynaPlex::Error("vehicle.at_pick_location tracking is off");
				else
					vehicle.at_pick_location = true;

				if (vehicle.assigned_picker >= 0)
				{
					auto& picker = state.pickers[vehicle.assigned_picker];
					if (picker.node_id == vehicle.node_id)//picker assigned and present
						InitiatePick(state, picker, vehicle, rng);
				}
				break;
			}
			case ScheduledEvent::IndexOf<VehicleNodeEntry>():
			{
				auto& payload = first_event.get<VehicleNodeEntry>();
				//node where vehicle will enter
				auto& node = state.nodes[payload.node_id];
				auto vehicle_id = node.node_entry_queue.pop_front();
				auto& vehicle = state.vehicles[vehicle_id];
			    //node where vehicle was previously located.
				auto& previous_node = state.nodes[vehicle.node_id];
				
				if (payload.node_id != vehicle.drop_off_node_id)
				{
					if (node.occupying_vehicle_id != -1)
						throw DynaPlex::Error("VehicleNodeEntry: Entry node is not vacant");
					if (node.is_accessible)
						throw DynaPlex::Error("VehicleNodeEntry: Entry node is accessible but it should have been made inaccessible prior to entry to avoid collisions.");
				}

				//set the previous node as empty (this happens before the previous node is accessible for the next vehicle to enter.)
				previous_node.occupying_vehicle_id = -1;

				//for making the previous node accessible at some point in the future, we fire a MakeNodeAccessible event.
				//except if the vehicle is leaving a drop-off location, which are always accessible.
				bool leaving_drop_off_loc = false;
				for (auto& truck_dock : truck_docks)
				{
					if (truck_dock.node_id == vehicle.node_id)
					{
						leaving_drop_off_loc = true;
						break;
					}
				}
				
				if (!leaving_drop_off_loc)
				{
					int64_t delay_time = vehicle_vacate_node_time.GetSample(rng);
					state.Schedule(state.current_time + delay_time, MakeNodeAccessible(vehicle.node_id));
				}
				
				vehicle.node_id = payload.node_id;
				node.occupying_vehicle_id = vehicle_id;

				int64_t destination = vehicle.remaining_pick_list.IsEmpty() ?  vehicle.drop_off_node_id : vehicle.remaining_pick_list.front();
				
				if (vehicle.node_id == destination)
				{//we arrived at destination. 
					if (vehicle.remaining_pick_list.IsEmpty())
					{
						if (vehicle.drop_off_node_id != vehicle.node_id)
							throw DynaPlex::Error("Vehicle_exit is not in the drop-off location");
						state.Schedule(state.current_time + drop_off_time.GetSample(rng), ConfigurePickRun(vehicle_id));
					}
					else
					{
						state.Schedule(state.current_time, VehicleReadyForPick(vehicle_id));
					}
				}
				else
					VehicleContinueTravel(state, vehicle_id, destination, rng);
				break;
			}		
			case ScheduledEvent::IndexOf<PickerArrival>():
			{
				auto& payload = first_event.get<PickerArrival>();
				auto& picker = state.pickers[payload.picker_id];

				picker.node_id = payload.node_id;

				if (picker.assigned_vehicle_id >= 0)
				{
					auto& vehicle = state.vehicles[picker.assigned_vehicle_id];
					if (vehicle.remaining_pick_list.IsEmpty())
						throw DynaPlex::Error("MDP::ModifyStateWithEvent - assigned vehicle has no destination.");
					
					auto& destination_node = vehicle.remaining_pick_list.front();							
					if (destination_node == picker.node_id)
					{//we have arrived
						if (vehicle.node_id == picker.node_id)
							InitiatePick(state, picker, vehicle, rng);
						else
							//Vehicle is not at the pick location yet.
							state.Schedule(state.current_time + deadlock_check_time, PickerCheckDeadlock(payload.picker_id, vehicle.node_id, picker.num_picks_started));
					}
					else
					{//continue to travel
						auto next_node_index = picker_graph.NextEdge(picker.node_id, destination_node).dest;
						auto trigger_time = state.current_time + picker_travel_time.GetSample(rng);
						state.Schedule(trigger_time, PickerArrival(payload.picker_id, next_node_index));
					}
				}
				else 
					throw DynaPlex::Error("MDP::ModifyStateWithEvent - picker arrival but no vehicle assigned.");				
				break;
			}
			case ScheduledEvent::IndexOf<PickerCheckDeadlock>():
			{
				auto& payload = first_event.get<PickerCheckDeadlock>();
				auto& picker = state.pickers[payload.picker_id];
				if (picker.num_picks_started == payload.last_num_picks_started)
				{
					//no new picks between the moment the CheckDeadLock event was fired and the current time: 
					//picker has been idle
					auto& vehicle = state.vehicles[picker.assigned_vehicle_id];
					//Now check if the assigned vehicle has moved since the last check.
					//If not, allow the picker to take a new action
					if (vehicle.node_id == payload.old_vehicle_node_id) {
						//vehicle has not moved - reassign to new vehicle:
						state.Schedule(state.current_time, PickerAwaitsAction(payload.picker_id,true));
					}
					else {
						//Vehicle has moved since last check
						//Schedule a new check after a certain time
						state.Schedule(state.current_time + deadlock_check_time, PickerCheckDeadlock(payload.picker_id, vehicle.node_id,payload.last_num_picks_started));
					}
				}
				break;
			}
			case ScheduledEvent::IndexOf<PickCompletes>():
			{
				if (!minimize_idling_objective)
				{
					costs -= 1.0;
				}
				state.picked_items++;

				auto& payload = first_event.get<PickCompletes>();
				auto& picker = state.pickers[payload.picker_id];
				auto& vehicle = state.vehicles[picker.assigned_vehicle_id];
				auto vehicle_id = picker.assigned_vehicle_id;
				
				vehicle.assigned_picker = -1;
				picker.assigned_vehicle_id = -1;

				vehicle.is_being_picked_for = false;
				vehicle.at_pick_location = false;
	
				if (picker.node_id != vehicle.node_id)
					throw DynaPlex::Error("Picker " + std::to_string(payload.picker_id) + " is not at the vehicle location.");

				//picker is ready for work again.
				state.Schedule(state.current_time+1, PickerAwaitsAction(payload.picker_id));

				vehicle.remaining_pick_list.pop_front();
				int64_t new_destination;
				if (vehicle.remaining_pick_list.IsEmpty())
				{
					new_destination = vehicle.drop_off_node_id;
					vehicle.is_dropping_off = true;
				}
				else
				{
					new_destination = vehicle.remaining_pick_list.front();
				}		
				if (new_destination == vehicle.node_id)
				{
					if (new_destination == vehicle.drop_off_node_id)
						throw DynaPlex::Error("Vehicle_exit is a pick_location: this is not supported");
					else						
						state.Schedule(state.current_time, VehicleReadyForPick(vehicle_id));
				}
				else
					VehicleContinueTravel(state, vehicle_id, new_destination, rng);
				break;
			}
			case ScheduledEvent::IndexOf<ConfigurePickRun>():
			{	
				auto& payload = first_event.get<ConfigurePickRun>();
				auto& vehicle = state.vehicles[payload.vehicle_id];

				if (vehicle.at_pick_location || vehicle.is_blocked || vehicle.is_being_picked_for || !vehicle.is_dropping_off)
					throw DynaPlex::Error("MDP::ModifyStateWithEvent - ConfigurePickRun : Unexpected flags set.");

				//location where we previously dropped off - corresponding dispatch, is where we will start
				int64_t vehicle_start_node = truck_docks[vehicle.drop_off_truck_doc_index].corresponding_dispatch_node_id;
				int64_t last_node = vehicle_start_node;

				vehicle.drop_off_truck_doc_index = rng.genUniform() * truck_docks.size();
				vehicle.drop_off_node_id = truck_docks[vehicle.drop_off_truck_doc_index].node_id;

				//Complete the pick list
				vehicle.remaining_pick_list.clear();
				for (auto& pick_loc : pick_locations)
				{
					if (rng.genUniform() < pick_loc.prob)
					{//create pick list - visits each pick location with designated probability;
						int64_t selected_vehicle_node = pick_loc.vehicle_nodes[0];
						if (!vehicle.remaining_pick_list.IsEmpty())
						{
							int64_t last_visit = vehicle.remaining_pick_list.back();
							auto [last_row, last_col] = vehicle_graph.Coordinates(last_visit);
							for (auto& candidate : pick_loc.vehicle_nodes)
							{
								auto [candidate_row, candidate_col] = vehicle_graph.Coordinates(candidate);
								if (candidate_col == last_col)
								{
									//if we can pick while staying within the same lane, we do so, and we deviate from the preferred:
									selected_vehicle_node = candidate;

								}
							}
						}
						vehicle.remaining_pick_list.push_back(selected_vehicle_node);
						if (!vehicle_graph.ExistsPath(last_node, selected_vehicle_node))
							throw DynaPlex::Error("Cannot find feasible path for pick list");
						last_node = selected_vehicle_node;
					}
				}
				
				if (!vehicle.remaining_pick_list.IsEmpty()) {
					vehicle.is_dropping_off = false;
				}
				auto& entrance_node = state.nodes[vehicle_start_node];


				if (payload.vehicle_id < 0)
					throw DynaPlex::Error("MDP::ModifyStateWithEvent: attempting to add negative id");

				entrance_node.node_entry_queue.push_back(payload.vehicle_id);
				if (entrance_node.is_accessible && entrance_node.node_entry_queue.front() == payload.vehicle_id)
				{//if not currently occupied, then invite this vehicle to enter such that it will be dispatched.

					entrance_node.is_accessible = false;
					state.Schedule(state.current_time, MakeNodeAccessible(vehicle_start_node));
				}
				else {
					//Flag the vehicle as blocked
					vehicle.is_blocked = true;
				}
				break;
			}	
			case ScheduledEvent::IndexOf<ChangeDistribution>():
			{
				auto& payload = first_event.get<ChangeDistribution>();
				state.current_distribution_id = payload.distribution_id;

				state.old_change_time = state.next_change_time;
				if (n_distributions == 2)
				{
					state.next_change_time += change_time * pow(2, state.current_distribution_id * 2);
					if (state.current_distribution_id < n_distributions - 1)
						state.Schedule(state.next_change_time, ChangeDistribution(state.current_distribution_id + 1));
					else
						state.Schedule(state.next_change_time, ChangeDistribution(0));
				}
				else
				{
					state.next_change_time += change_time * pow(2, state.current_distribution_id);
					if (state.current_distribution_id < n_distributions - 1)
						state.Schedule(state.next_change_time, ChangeDistribution(state.current_distribution_id + 1));
					else
						state.Schedule(state.next_change_time, ChangeDistribution(0));
				}

				break;

			}
			default:
				throw DynaPlex::Error("MDP::ModifyStateWithEvent : case not covered.");
				break;
			}
			UpdateStateCategory(state);
			return costs;
		}

		void MDP::UpdateStateCategory(State& state) const {
			auto& first_event = state.scheduled_event_queue.first();
			switch (first_event.PayLoadIndex())
			{
			case ScheduledEvent::IndexOf<ConfigurePickRun>():
				state.cat = StateCategory::AwaitEvent(2);
				break;
			case ScheduledEvent::IndexOf<PickerAwaitsAction>():
			{
				auto& payload = first_event.get<PickerAwaitsAction>();
				state.cat = StateCategory::AwaitAction(payload.picker_id + 1);
				break;
			}
			case ScheduledEvent::IndexOf<TimeStep>():
				//0 means dynaplex will associate each firing of this event as a timestep. 
				state.cat = StateCategory::AwaitEvent(0);
				break;
			default:
				state.cat = StateCategory::AwaitEvent(1);
				break;
			}
		}

		int64_t MDP::GetSelectedVehicleIndex(const State& state, int64_t action) const {
			auto& first_event = state.scheduled_event_queue.first();
			if (!first_event.has<PickerAwaitsAction>())
				throw DynaPlex::Error("collaborative_picking: error in state flow logic.");
			auto& payload = first_event.get<PickerAwaitsAction>();
			auto& picker = state.pickers[payload.picker_id];		

			if (!graph_feats) {
				// When graph_feats is false, action directly corresponds to vehicle indices or stay-in-place command
				if (action == num_vehicles) { // Assuming num_vehicles indicates "stay in place"
					return -1;
				}
				return action;
			}
			else {
				// When graph_feats is true, actions correspond to node locations in the warehouse
				int64_t min_distance = std::numeric_limits<int64_t>::max();
				int64_t best_vehicle_id = -1;

				for (const auto& [vehicle_index, vehicle] : state.vehicles) {
					if (vehicle.IsAssignable(payload.reassign) && vehicle.remaining_pick_list.front() == action) {
						DynaPlex::Graph::WeightType distance;
						if (vehicle_graph.ExistsPath(vehicle.node_id, vehicle.remaining_pick_list.front())) {
							distance = vehicle_graph.Distance(vehicle.node_id, vehicle.remaining_pick_list.front());
						}
						else {
							// If the vehicle is not dispatched yet
							auto start_node = truck_docks[vehicle.drop_off_truck_doc_index].corresponding_dispatch_node_id;
							distance = vehicle_graph.Distance(start_node, vehicle.remaining_pick_list.front());
						}
						if (distance < min_distance) {
							best_vehicle_id = vehicle_index;
							min_distance = distance;
						}
					}
				}
				if (best_vehicle_id == -1)
				{
					if (action != picker.node_id)
						throw DynaPlex::Error("GetSelectedVehicleIndex: issue with node");
				}
				return best_vehicle_id; // Will return -1 if no suitable vehicle was found
			}
		}


		double MDP::ModifyStateWithAction(MDP::State& state, int64_t action_index) const
		{
			auto& first_event = state.scheduled_event_queue.first();

			if (state.current_time > first_event.trigger_time)
				throw DynaPlex::Error("MDP::ModifyStateWithAction - Time updating issue.");
			state.current_time = first_event.trigger_time;

			if (!first_event.has<PickerAwaitsAction>())
				throw DynaPlex::Error("collaborative_picking: error in state flow logic.");

			bool allowed = IsAllowedAction(state, action_index);
			if (!allowed)
				throw DynaPlex::Error("MDP::ModifyStateWithAction - Invalid action proposed");

			auto& payload = first_event.get<PickerAwaitsAction>();

			auto& picker = state.pickers[payload.picker_id];
			auto picker_id = payload.picker_id;
			auto reassign = payload.reassign;
			auto vehicle_index = GetSelectedVehicleIndex(state, action_index);

			//note that payload becomes dangling now.
			state.scheduled_event_queue.pop();

			if (picker.assigned_vehicle_id >= 0) {
				if (!reassign)
					throw DynaPlex::Error("No reassign but picker is allready assigned.");
				auto& vehicle = state.vehicles[picker.assigned_vehicle_id];
				vehicle.assigned_picker = -1;
				picker.assigned_vehicle_id = -1;
			}

		
			if (vehicle_index == -1) {
				state.Schedule(state.current_time + idling_time, PickerAwaitsAction(picker_id));
			}
			else
			{   //make the appropriate assignment:
				state.vehicles[vehicle_index].assigned_picker = picker_id;
				picker.assigned_vehicle_id = vehicle_index;
				state.Schedule(state.current_time, PickerArrival(picker_id, picker.node_id));
			}


			UpdateStateCategory(state);
			return 0.0;
		}

		DynaPlex::VarGroup MDP::State::ToVarGroup() const
		{
			DynaPlex::VarGroup vars;
			vars.Add("cat", cat);
			vars.Add("current_time", current_time);
			vars.Add("picked_items", picked_items);
			vars.Add("nodes", nodes);
			vars.Add("pickers", pickers);
			vars.Add("vehicles", vehicles);
			vars.Add("scheduled_event_queue", scheduled_event_queue);
	
			return vars;
		}

		MDP::State MDP::GetState(const VarGroup& vars) const
		{
			State state{};
			vars.Get("cat", state.cat);
			vars.Get("current_time", state.current_time);
			vars.Get("picked_items", state.picked_items);
			vars.Get("nodes", state.nodes);
			vars.Get("pickers", state.pickers);
			vars.Get("vehicles", state.vehicles);
			vars.Get("scheduled_event_queue", state.scheduled_event_queue);
			return state;
		}

		MDP::State MDP::GetInitialState(DynaPlex::RNG& rng) const
		{
			State state{};		
			state.nodes = std::vector<Node>(vehicle_graph.NumNodes(), Node{});
			state.current_time = 0;
			state.picked_items = 0;

			//randomly divide the pickers over the pick locations. 
			for (int p = 0; p < num_pickers; p++)
			{
				auto& [picker_index, picker] = state.pickers.AddNew();
				if (picker_index != p)
					throw DynaPlex::Error("Error in logic while creating new state");
				picker.node_id = picker_areas.at(picker_index).nodes[0];
				state.Schedule(state.current_time, PickerAwaitsAction(picker_index));
			}
			//randomly divide the vehicles over the truck docks:
			for (int p = 0; p < num_vehicles; p++)
			{
				auto& [vehicle_index, vehicle] = state.vehicles.AddNew();
				vehicle.drop_off_truck_doc_index = rng.genUniform() * truck_docks.size();
				vehicle.is_dropping_off = true;
				auto& truck_dock= truck_docks.at(vehicle.drop_off_truck_doc_index);
				vehicle.node_id = truck_dock.node_id;
				auto trigger_time = state.current_time + drop_off_time.GetSample(rng);
				//vehicles are roughly released over a time duration of startup_duration:
				trigger_time += startup_duration * p / num_vehicles;

				if (vehicle.at_pick_location)
					throw DynaPlex::Error("Vehicle should not be at pick location");
				state.Schedule(trigger_time, ConfigurePickRun(vehicle_index));
			}

			//recurring time-steps that don't do anything but just track status of system, mainly for visualization, while increasing
			//the period. 
			state.Schedule(state.current_time, TimeStep());

			state.old_change_time = state.current_time + startup_duration;
			if (n_distributions == 2)
			{
				state.next_change_time = state.current_time + startup_duration + change_time * pow(2, state.current_distribution_id * 2);
				state.Schedule(state.next_change_time, ChangeDistribution(state.current_distribution_id + 1));
			}
			else
			{
				state.next_change_time = state.current_time + startup_duration + change_time * pow(2, state.current_distribution_id);
				state.Schedule(state.next_change_time, ChangeDistribution(state.current_distribution_id + 1));
			}
			
			UpdateStateCategory(state);
			return state;
		}
		std::vector<int64_t> GenerateIslePattern(int64_t isle_width, int64_t num_isles) {
			std::vector<int64_t> pattern;

			// Loop through each isle
			for (int64_t isle = 0; isle < num_isles; ++isle) {
				// Add up lanes
				for (int64_t i = 0; i < isle_width; ++i) {
					pattern.push_back(0); // 0 for up lanes
				}

				// Add 2 storage lanes, independent of isle width
				pattern.push_back(2); // 2 for storage
				pattern.push_back(2);

				// Add down lanes
				for (int64_t i = 0; i < isle_width; ++i) {
					pattern.push_back(1); // 1 for down lanes
				}

				// Add 2 storage lanes after each isle except the last
				if (isle < num_isles - 1) {
					pattern.push_back(2); // 2 for storage
					pattern.push_back(2);
				}
			}
			return pattern;
		}


		MDP::MDP(const VarGroup& config)
		{
			config.GetOrDefault("minimize_idling_objective", minimize_idling_objective, false);
			config.GetOrDefault("timestep_delta", timestep_delta, 1);
			config.GetOrDefault("startup_duration", startup_duration, 0);
			config.GetOrDefault("idling_time", idling_time, 1);
			config.GetOrDefault("graph_feats", graph_feats, false);
			config.GetOrDefault("non_stationary", non_stationary, false);
			config.GetOrDefault("n_distributions", n_distributions, 1);
			config.GetOrDefault("deadlock_check_time", deadlock_check_time, 1);
			//initiate the config_info, for purposes of visualization. 
			DynaPlex::VarGroup vehicle_graph_vg{};
			DynaPlex::VarGroup picker_graph_vg{};
			DynaPlex::VarGroup::VarGroupVec pick_locations_vec{};


			if (config.HasKey("vehicle_graph"))
			{
				config.Get("vehicle_graph", vehicle_graph_vg);
				config.Get("picker_graph", picker_graph_vg);
				config.Get("truck_docks", truck_docks);
				config.Get("pick_locations", pick_locations_vec);
				config.Get("first_pick_location", first_pick_location);

			}
			else
			{
				int64_t num_isles, isle_width, isle_length;
				double average_picks_per_run;
				config.Get("num_isles", num_isles);
				if (num_isles != (num_isles / 2) * 2)
				{
					throw DynaPlex::Error("num_isles must be even number");
				}
				config.Get("average_picks_per_run", average_picks_per_run);
				config.GetOrDefault("isle_width", isle_width, 2);
				config.Get("isle_length", isle_length);
				vehicle_graph_vg = GenerateVehicleGraph(isle_length, num_isles / 2, isle_width);
				picker_graph_vg = GeneratePickerGraph(isle_length, num_isles / 2, isle_width);
				auto pattern = GenerateIslePattern(isle_width, num_isles / 2);

				for (int col = 0; col < pattern.size(); col++)
				{
					truck_docks.push_back(TruckDock{});
					truck_docks.back().row = isle_length + 3;
					truck_docks.back().column = col + 1;
				}

				auto num_pick_locs = isle_length * 2 * num_isles;
				auto prob = average_picks_per_run / num_pick_locs;

				first_pick_location.column = 1;
				first_pick_location.row = isle_length + 1;
				for (int isle = 0; isle < num_isles; isle++)
				{
					for (int row = 1; row <= isle_length; row++)
					{
						for (int64_t side : {0, 1})
						{
							auto col1 = isle * (isle_width + 2) + side * (isle_width + 1);
							std::vector<int64_t> vehicle_locs{};
							for (size_t i = 0; i < isle_width; i++)
								vehicle_locs.push_back(side ? (-i - 1) : (i + 1));

							DynaPlex::VarGroup vg;
							vg.Add("coords", DynaPlex::VarGroup::Int64Vec{ row,col1 });
							vg.Add("prob", prob);
							vg.Add("vehicle_loc", vehicle_locs);
							pick_locations_vec.push_back(std::move(vg));
						}
					}
				}
			}

			config_info = DynaPlex::VarGroup{
				{"vehicle_graph", vehicle_graph_vg},
				{"pick_locations", pick_locations_vec}
			};

			picker_graph = DynaPlex::Graph(picker_graph_vg);
			vehicle_graph = DynaPlex::Graph(vehicle_graph_vg);

			//validation:
			if (picker_graph.Height() != vehicle_graph.Height() ||
				picker_graph.Width() != vehicle_graph.Width())
				throw DynaPlex::Error("Collaborative picking - graph dimensions do not match");


			config.Get("num_pickers", num_pickers);
			config.Get("num_vehicles", num_vehicles);

			for (auto& vg : pick_locations_vec)
			{
				pick_locations.emplace_back(vg);
			}

			//various statistical distributions associated with durations of certain activities:
			config.Get("drop_off_time", drop_off_time);
			config.Get("vehicle_travel_time", vehicle_travel_time);
			config.Get("picker_travel_time", picker_travel_time);

			DynaPlex::DiscreteDist pick_time;

			if (n_distributions == 2)
				change_time = 3600;
			else
				change_time = static_cast<int>(std::round(18000 / (pow(2, n_distributions) - 1)));

			if (non_stationary)
			{
				if (n_distributions == 2) {
					config.Get("pick_time_short", pick_time);
					pick_time.OptimizeForSampling();
					pick_times.push_back(pick_time);
					config.Get("pick_time_long", pick_time);
					pick_time.OptimizeForSampling();
					pick_times.push_back(pick_time);
				}
				else if (n_distributions == 3) {
					config.Get("pick_time_short", pick_time);
					pick_time.OptimizeForSampling();
					pick_times.push_back(pick_time);
					config.Get("pick_time_medium", pick_time);
					pick_time.OptimizeForSampling();
					pick_times.push_back(pick_time);
					config.Get("pick_time_long", pick_time);
					pick_time.OptimizeForSampling();
					pick_times.push_back(pick_time);
				}
				else {
					config.Get("pick_time_very_short", pick_time);
					pick_time.OptimizeForSampling();
					pick_times.push_back(pick_time);
					config.Get("pick_time_short", pick_time);
					pick_time.OptimizeForSampling();
					pick_times.push_back(pick_time);
					config.Get("pick_time_medium", pick_time);
					pick_time.OptimizeForSampling();
					pick_times.push_back(pick_time);
					config.Get("pick_time_long", pick_time);
					pick_time.OptimizeForSampling();
					pick_times.push_back(pick_time);
				}
			}
			else
			{
				config.Get("pick_time", pick_time);
				pick_time.OptimizeForSampling();
				pick_times.push_back(pick_time);
				pick_times.push_back(pick_time);
				pick_times.push_back(pick_time);
			}

			config.Get("vehicle_vacate_node_time", vehicle_vacate_node_time);
			drop_off_time.OptimizeForSampling();
			vehicle_travel_time.OptimizeForSampling();
			picker_travel_time.OptimizeForSampling();
			//pick_time.OptimizeForSampling();
			vehicle_vacate_node_time.OptimizeForSampling();

			first_pick_location.UpdateNodeId(vehicle_graph.Width());


			for (auto& truck_dock : truck_docks)
			{
				truck_dock.UpdateNodeId(vehicle_graph.Width());
				//the dispatch node is assumed to be one above the location of the truck dock. 
				truck_dock.corresponding_dispatch_node_id = vehicle_graph.NodeAt(truck_dock.row - 1, truck_dock.column);
			}

			for (auto& pick_loc : pick_locations)
			{
				pick_loc.node_id = vehicle_graph.NodeAt(pick_loc.row, pick_loc.column);
				for (int64_t delta : pick_loc.vehicle_loc)
				{
					auto node = vehicle_graph.NodeAt(pick_loc.row, pick_loc.column + delta);
					pick_loc.vehicle_nodes.push_back(node);
					if (!this->vehicle_graph.ExistsPath(first_pick_location.node_id, node))
					{
						throw DynaPlex::Error("Pick node pickup location not reachable from first_pick_location in vehicle graph.");
					}
				}
			}
			OrderPickLocations();
			std::cout << "finished initiation" << std::endl;
		}
		
		DynaPlex::VarGroup MDP::GeneratePickerGraph(int64_t isle_length, int64_t num_isles, int64_t isle_width) {
			std::vector<std::string> rows;
			std::vector<int64_t> lanePattern = GenerateIslePattern(isle_width, num_isles);

			// Upper Row (same as in vehicle_graph)
			std::string upperRow = "|";
			for (auto lane : lanePattern) {
				upperRow += (lane == 1) ? "RD |" : "R  |"; // RD for down lanes, R for up and storage
			}
			rows.push_back(upperRow);

			// Isle Lanes with special handling for UR and DR except for the last lane in every isle
			for (int64_t i = 0; i < isle_length; ++i) {
				std::string row = "|";
				for (size_t j = 0; j < lanePattern.size(); ++j) {
					if (lanePattern[j] == 0) { // Up lanes
						row += (j == lanePattern.size() - 1 || lanePattern[j + 1] == 2) ? "U  |" : "UR |";
					}
					else if (lanePattern[j] == 1) { // Down lanes
						row += (j+1!=lanePattern.size()) && (lanePattern.at(j + 1) == 1) ? "DR |" : "D  |";
					}
					else { // Storage lanes
						row += "   |";
					}
				}
				rows.push_back(row);
			}

			// First Row After Isle Lanes
			std::string firstRowAfterIsle = "|";
			for (auto lane : lanePattern) {
				firstRowAfterIsle += (lane == 0) ? "URD|" : "RD |"; // URD for up lanes, RD for down and storage
			}
			rows.push_back(firstRowAfterIsle);

			// Special Row
			std::string specialRow = "|  D|L D|";
			for (size_t i = 2; i < lanePattern.size(); ++i) { // Adjust for initial special entries
				specialRow += "L D|";
			}
			rows.push_back(specialRow);

			// Last Row (same as in vehicle_graph)
			std::string lastRow = "|";
			for (size_t i = 0; i < lanePattern.size(); ++i) {
				lastRow += "   |";
			}
			rows.push_back(lastRow);

			DynaPlex::VarGroup vars{};
			vars.Add("rows", rows);
			vars.Add("format", "grid");
			vars.Add("type", "undirected");
			return vars;

		}

		DynaPlex::VarGroup MDP::GenerateVehicleGraph(int64_t isle_length, int64_t num_isles, int64_t isle_width) {
			std::vector<std::string> rows;
			std::vector<int64_t> lanePattern = GenerateIslePattern(isle_width, num_isles);

			// Upper Row
			std::string upperRow = "|";
			for (auto lane : lanePattern) {
				upperRow += (lane == 1) ? "RD |" : "R  |"; // RD for down lanes, R for up and storage
			}
			rows.push_back(upperRow);

			// Isle Lanes
			std::string isleLane = "|";
			for (auto lane : lanePattern) {
				isleLane += (lane == 0) ? "U  |" : ((lane == 1) ? "D  |" : "   |");
			}
			for (int64_t i = 0; i < isle_length; ++i) {
				rows.push_back(isleLane);
			}

			// First Row After Isle Lanes
			std::string firstRowAfterIsle = "|";
			for (size_t i = 0; i < lanePattern.size()-1; ++i) {
				if (lanePattern[i] == 1)//down
					firstRowAfterIsle += "RD |";
				else
				{
					firstRowAfterIsle += (lanePattern[i] == 0) ? "UR |" : "R  |"; // UR for up lanes, R for storage
				}
			}
			firstRowAfterIsle += "D  |";

			rows.push_back(firstRowAfterIsle);

			// Second row after isle lanes
			std::string secondRowAfterIsle = "| UD|LUD|";
			for (size_t i = 2; i < lanePattern.size(); ++i) { // Start from 2 to account for the initial UD|LUD|			
				secondRowAfterIsle += lanePattern[i] == 0 ? "LUD|" : "L D|";
			}
			rows.push_back(secondRowAfterIsle);

			// Last Row
			std::string lastRow = "|";
			for (size_t i = 0; i < lanePattern.size(); ++i) {
				lastRow += "   |";
			}
			rows.push_back(lastRow);
					

			DynaPlex::VarGroup vars{};
			vars.Add("rows", rows);
			vars.Add("format", "grid");
			vars.Add("type", "directed");
			return vars;

		}
		
		void MDP::OrderPickLocations() {

			//order the pick locations such that there always exists an efficient path
			//do this via greedy addition  -assuming some sort of s-curve is possible.
			std::vector<PickLocation> new_pick_locations; // Temporary container for the new order
			new_pick_locations.reserve(pick_locations.size());
			int64_t current_node = first_pick_location.node_id;
			std::vector<bool> visited(pick_locations.size(), false);
			while (new_pick_locations.size() < pick_locations.size()) {
				double min_distance = std::numeric_limits<double>::max();
				size_t closest_pick_index = std::numeric_limits<size_t>::max();

				for (size_t i = 0; i < pick_locations.size(); ++i) {
					if (!visited[i]) {
						for (int64_t candidate_node : pick_locations[i].vehicle_nodes)
						{

							if (vehicle_graph.ExistsPath(current_node, candidate_node))
							{
								double distance = vehicle_graph.Distance(current_node, candidate_node);

								if (distance < min_distance) {
									min_distance = distance;
									closest_pick_index = i;
								}
							}
						}
					}
				}
				if (closest_pick_index != std::numeric_limits<size_t>::max()) {
					// We've found the next closest pick location
					visited[closest_pick_index] = true; // Mark as visited
					new_pick_locations.push_back(pick_locations[closest_pick_index]);
					// Update the current node for the next iteration
					current_node = pick_locations[closest_pick_index].vehicle_nodes.at(0);
				}
				else {
					throw DynaPlex::Error("Failed to construct ordering of pick locations for pickruns.");
				}
			}
			// Replace the original pick_locations with the newly ordered list
			pick_locations.swap(new_pick_locations);

			std::vector<bool> added(vehicle_graph.NumNodes(),false);
			//now, the pick nodes are all the nodes where there can be picks. 
			for (auto& pick_loc : pick_locations)
			{
				for (auto& node : pick_loc.vehicle_nodes)
				{
					if (!added.at(node)) {
						pick_nodes.push_back(node);
						added.at(node) = true;
					}
				}
			}
			picker_areas = std::vector<PickerArea>(num_pickers);

			// Guard against division by zero
			if (num_pickers == 0 || pick_nodes.empty()) return;

			size_t min_nodes_per_picker = pick_nodes.size() / num_pickers;
			size_t pickers_with_extra_node = pick_nodes.size() % num_pickers;

			size_t current_node_index = 0;
			for (size_t picker = 0; picker < num_pickers; ++picker) {
				// Determine how many nodes this picker will get
				size_t nodes_for_this_picker = min_nodes_per_picker + (picker < pickers_with_extra_node ? 1 : 0);

				// Assign nodes to this picker
				for (size_t node = 0; node < nodes_for_this_picker; ++node) {
					picker_areas[picker].nodes.push_back(pick_nodes.at(current_node_index++));
				}
			}
			if (current_node_index != pick_nodes.size())
				throw DynaPlex::Error("issue with distributing nodes over pickers.");
		}


		void MDP::GetFeatures(const State& state, DynaPlex::Features& features)const {
			//note that otherwise it throws as it cannot find the features during MDP construction.

			auto& first_event = state.scheduled_event_queue.first();
			if (!first_event.has<PickerAwaitsAction>())
				throw DynaPlex::Error("GetFeatures: error in state flow logic.");
			auto& payload = first_event.get<PickerAwaitsAction>();

			if (!graph_feats){
				features.Add(payload.picker_id);

				for (auto& [picker_id, picker] : state.pickers) {
					features.Add(picker_id == payload.picker_id ? 1 : 0);
					features.Add(picker.node_id);
					features.Add(picker.assigned_vehicle_id);
					features.Add(picker.assigned_vehicle_id >= 0 ? state.vehicles[picker.assigned_vehicle_id].remaining_pick_list.front() : -1);
					features.Add(picker.assigned_vehicle_id >= 0 ? picker_graph.Distance(picker.node_id, state.vehicles[picker.assigned_vehicle_id].remaining_pick_list.front()) : -1);
					features.Add(picker.assigned_vehicle_id >= 0 ? state.vehicles[picker.assigned_vehicle_id].is_being_picked_for : false);
				}
				for (auto& [vehicle_id, vehicle] : state.vehicles)
				{
					features.Add(vehicle.node_id);
					int64_t destination = vehicle.remaining_pick_list.IsEmpty() ? vehicle.drop_off_node_id : vehicle.remaining_pick_list.front();
					features.Add(destination);
					features.Add(vehicle.is_blocked);
					features.Add(vehicle.is_being_picked_for);
					features.Add(vehicle.is_dropping_off);
				}
			}
			else {

				if (non_stationary) {
					// Add time until next distribution change
					features.Add(((float_t)state.next_change_time - (float_t)state.current_time) / ((float_t)state.next_change_time - (float_t)state.old_change_time));

					//One-hot encoding of pick-time distribution type
					for (int i = 0; i < n_distributions; i++) {
						features.Add(state.current_distribution_id == i ? 1 : 0);
					}
				}

				//Features based on graph structure
				auto current_picker = state.pickers[payload.picker_id];
				int64_t node_id = 0;
				for (auto& node : state.nodes) {
					//Current picker features
					features.Add(node_id == current_picker.node_id ? 1 : 0); // Whether the node contains the currently controlled picker
					
					if (vehicle_graph.ExistsPath(node_id, current_picker.node_id))
						features.Add(vehicle_graph.Distance(node_id, current_picker.node_id)); // Vehicle distance of the node from the location of the controlled picker
					else
						features.Add(-1);
					
					if (picker_graph.ExistsPath(node_id, current_picker.node_id))
						features.Add(picker_graph.Distance(node_id, current_picker.node_id)); // Picker distance of the node from the location of the controlled picker
					else
						features.Add(-1);
					
					//Vehicle features
					features.Add(node.occupying_vehicle_id != -1 ? 1 : 0); // Whether the node contains a vehicle
					
					//Picker features
					int num_free_pickers = 0;
					int num_assigned_pickers = 0;
						for (auto& [picker_id, picker] : state.pickers) {
						if (picker_id != payload.picker_id) {
							if (picker.assigned_vehicle_id >= 0) {
								if (picker.node_id == node_id)
									num_assigned_pickers++;
							}
							else {
								if (picker.node_id == node_id)
									num_free_pickers++;
							}
						}
					}
					features.Add(num_free_pickers); // Number of free pickers contained in the node
					features.Add(num_assigned_pickers); // Number of assigned pickers contained in the node
		
					//General features
					features.Add(node.occupying_vehicle_id != -1 && state.vehicles[node.occupying_vehicle_id].is_being_picked_for); // Whether a picking operation is happening in the node

					node_id++;
				}
			}
		}

		void MDP::RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>& registry) const
		{
			registry.Register<ClosestPair>("closest_pair", "myopically matches the closest available pair.");
		}

		DynaPlex::StateCategory MDP::GetStateCategory(const State& state) const
		{
			return state.cat;
		}

		bool MDP::IsAllowedAction(const State& state, int64_t action_index) const {

			auto& payload = state.scheduled_event_queue.first().get<PickerAwaitsAction>();


			if (graph_feats) {
				//Picker location is always allowed
				if (action_index == state.pickers[payload.picker_id].node_id) 
					return true;
				for (auto& [vehicle_id, vehicle] : state.vehicles) {
					//note that IsAssignable means that it is going towards a pick, hence picklist cannot be empty
					if (vehicle.IsAssignable(payload.reassign) && vehicle.remaining_pick_list.front() == action_index) {
						return true;
					}
				}
				return false;
			}
			else {
				if (action_index == num_vehicles)
					return true;
				else
					return state.vehicles[action_index].IsAssignable(payload.reassign);
			}
		}


		void Register(DynaPlex::Registry& registry)
		{
			DynaPlex::Erasure::MDPRegistrar<MDP>::RegisterModel("collaborative_picking",
				"Model for collaborative - objective is assigning work to right picker.", registry);
		}
		void MDP::ResetHiddenStateVariables(State& state, DynaPlex::RNG& rng) const
		{

		}

	}
}
