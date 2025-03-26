#pragma once
#include "dynaplex/dynaplex_model_includes.h"
#include "dynaplex/modelling/discretedist.h"
#include "state_helper_classes.h"
#include "mdp_helper_classes.h"
#include "dynaplex/modelling/idcontainer.h"
#include "dynaplex/modelling/idkeycontainer.h"
#include "dynaplex/modelling/graph.h"
#include "dynaplex/modelling/eventheap.h"

namespace DynaPlex::Models {
	namespace collaborative_picking
	{
		class MDP
		{
		public:
			DynaPlex::Graph picker_graph, vehicle_graph;

			Location first_pick_location;
			std::vector<PickLocation> pick_locations;

			std::vector<int64_t> pick_nodes;
			std::vector<TruckDock> truck_docks;

			std::vector<PickerArea> picker_areas;

			int64_t num_pickers;
			int64_t num_vehicles;
			int64_t idling_time;
			int64_t deadlock_check_time;
			DynaPlex::DiscreteDist vehicle_travel_time, picker_travel_time, drop_off_time, vehicle_vacate_node_time;

			int64_t n_distributions;
			std::vector<DynaPlex::DiscreteDist> pick_times;

			int64_t startup_duration;
			int64_t timestep_delta;

			//To ensure balanced data collection, the time changes are handled proportionally to the pick_times of the current_distribution
			//Base pick time is multiplied by a factor that increases exponentially in base 2
			int64_t change_time;

			bool minimize_idling_objective;

			bool graph_feats;

			bool non_stationary;

			//here, we store info that a user might want to have for rendering the scene/warehouse
			DynaPlex::VarGroup config_info;
		public:


			struct State {
				DynaPlex::StateCategory cat;
				int64_t current_time{ 0 };
				int64_t current_distribution_id{ 0 };
				int64_t old_change_time{ 0 };
				int64_t next_change_time{ 0 };
				int64_t picked_items{ 0 };
				std::vector<Node> nodes{};
				DynaPlex::IdContainer<Picker> pickers{};
				DynaPlex::IdContainer<Vehicle> vehicles{};
				DynaPlex::EventHeap<ScheduledEvent> scheduled_event_queue{};
			
				DynaPlex::VarGroup ToVarGroup() const;
				inline void Schedule(int64_t trigger_time, ScheduledEvent::EventPayLoad&& payload)
				{
					scheduled_event_queue.push_back(ScheduledEvent(trigger_time, std::forward<ScheduledEvent::EventPayLoad>(payload)));
				}

			};

			//helper function:
			void UpdateStateCategory(State&) const;
			//Gets the vehicle index corresponding to a certain action
			int64_t GetSelectedVehicleIndex(const State&, int64_t action) const;

			DynaPlex::VarGroup GeneratePickerGraph(int64_t isle_length, int64_t num_isles, int64_t isle_width);
			DynaPlex::VarGroup GenerateVehicleGraph(int64_t isle_length, int64_t num_isles, int64_t isle_width);
			void OrderPickLocations();


			void InitiatePick(State& state, Picker& picker, Vehicle& vehicle, RNG& rng) const;
			void VehicleContinueTravel(State& state, int64_t& vehicle_id, int64_t& destination, RNG& rng) const;

			//API:
			double ModifyStateWithAction(State&, int64_t action) const;
			double ModifyStateWithEvent(State&, DynaPlex::RNG&) const;
			DynaPlex::VarGroup GetStaticInfo() const;
			DynaPlex::StateCategory GetStateCategory(const State&) const;
			bool IsAllowedAction(const State& state, int64_t action) const;
			State GetInitialState(DynaPlex::RNG& rng) const;
			State GetState(const VarGroup&) const;
			void RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>&) const;
			void GetFeatures(const State&, DynaPlex::Features&) const;
			explicit MDP(const DynaPlex::VarGroup&);
			void ResetHiddenStateVariables(State&, DynaPlex::RNG&) const;

		};
	}
}