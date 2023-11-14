#pragma once
#include "dynaplex/dynaplex_model_includes.h"
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/modelling/queue.h"
#include "state.h"

//namespace DynaPlex::Erasure { template <typename MDPType> class PolicyRegistry; }// Forward declaration 

//Luca Begnardi - TU/e
//l.begnardi@tue.nl
//based on the work of Fabian Akkerman (University of Twente) and Riccardo Lo Bianco (Eindhoven University of Technology)

namespace DynaPlex::Models {
	namespace order_picking /*must be consistent everywhere for complete mdp definition and associated policies and states (if not defined inline).*/
	{		
		class MDP
		{			
		public:	
			double discount_factor;
			
			//any other mdp variables go here:
			int64_t grid_size;
			int64_t n_pickers;
			int64_t max_orders_per_event;
			double holding_cost;
			double move_cost;
			double tardiness_cost;
			double order_probability;
			int64_t min_time_window;
			int64_t max_time_window;
			int64_t max_confirmed;
			bool fixed_initial_state;
			bool manual_initial_state;

			int64_t n_valid_actions;

			//events are complex objects in this environment, indicating order arrivals and state changes
			class ArrivalEvent
			{
			public:
				std::vector<int64_t> locations;
				std::vector<std::vector<int64_t>> time_windows; //matrix containing the different time_windows or the incoming orders
																//dim0 are the orders, dim1 the time_window for each state

				//we generate a value for the transitions in each position of the grid (maybe there is a better way to do this)
				std::vector<int64_t> confirmedTransitions;

				friend std::ostream& operator <<(std::ostream& os, const MDP::ArrivalEvent& event) {

					for (size_t i = 0; i < event.locations.size(); i++) {
						os << event.locations[i] << ",";
						for (size_t j = 0; j < event.time_windows[i].size(); j++) {
							os << event.time_windows[i][j] << ",";
						}
					}
					return os;
				}

				ArrivalEvent(std::vector<int64_t> locations, std::vector< std::vector<int64_t>> time_windows, std::vector<int64_t> confirmedTransitions)
					:locations(locations), time_windows(time_windows), confirmedTransitions{ confirmedTransitions }
				{
				};
			};

			using State = DynaPlex::Models::order_picking::State;
			using Event = ArrivalEvent;

			double ModifyStateWithAction(State&, int64_t action) const;
			double ModifyStateWithEvent(State&, const Event&) const;
			Event GetEvent(DynaPlex::RNG& rng) const;
			DynaPlex::VarGroup GetStaticInfo() const;
			DynaPlex::StateCategory GetStateCategory(const State&) const;
			bool IsAllowedAction(const State& state, int64_t action) const;			
			State GetInitialState() const;
			State GetState(const VarGroup&) const;
			void RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>&) const;
			void GetFeatures(const State&, DynaPlex::Features&) const;
			explicit MDP(const DynaPlex::VarGroup&);
		
			//custom functions
			bool IsAllowedNeighbor(const int64_t i, const int64_t j) const;
			//bool isObstacle(const int64_t i) const;
			double GetHoldingCosts(const State& state) const;
			double ApplyMove(State& state, int64_t move) const;
			void manageOrders(State& state, const std::vector<int64_t>& confirmedTransitions) const;

			std::vector<int64_t> GetPotentialMoves(const int64_t location) const;

			int64_t GetBestMove(const int64_t currentLocation, const int64_t destination) const;

			std::vector<std::vector<int64_t>> enumerationMatrix;
			std::vector<std::vector<int64_t>> edgeIndex;
			std::vector<std::vector<int64_t>> adjacencyMatrix;
			std::vector<std::vector<int64_t>> distanceMatrix;

			//std::vector<int64_t> obstacleLocations{};

		private:


			//vectors for assigning random values to the positions, time_windows of each cherry
			std::vector<double> posProbs;
			std::vector<std::vector<double>> twProbs;

			//probabilities for states transitions
			std::vector<double> probsConfirmed;

			DynaPlex::DiscreteDist posDist;
			std::vector<DynaPlex::DiscreteDist> twDist;
			DynaPlex::DiscreteDist confirmedDist;
		};
	}
}

