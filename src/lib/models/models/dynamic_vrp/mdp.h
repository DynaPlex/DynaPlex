#pragma once
#include "dynaplex/dynaplex_model_includes.h"
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/modelling/matrix.h"

namespace DynaPlex::Models {
	namespace dynamic_vrp /*must be consistent everywhere for complete mdp definition and associated policies and states (if not defined inline).*/
	{		
		class MDP
		{			
		public:	
			double discount_factor;
			//any other mdp variables go here:
			 
			struct State {
				//using this is recommended:
				DynaPlex::StateCategory cat;
				DynaPlex::VarGroup ToVarGroup() const;
				//Defaulting this does not always work. It can be removed as only the exact solver would benefit from this. 
				bool operator==(const State& other) const = default;

				std::vector<int64_t> Vehicles_loc{};
				std::vector<int64_t> Vehicles_remainingCapacity{};
				std::vector<int64_t> Vehicles_route{};
				std::vector<int64_t> Vehicles_numcust{};
				std::vector<int64_t> Demand{};
				int64_t currentVehicle{};//helper for applying decision to state
				int64_t depotVisits{};
				int64_t day{};
				int64_t newDemand{};//only used for assignment decision space
				bool decision1{};//only for hybrid decision
				int64_t countDeviation{};

			};

			class ArrivingCustomerEvent
			{
			public:
				int64_t location;

				ArrivingCustomerEvent(int64_t location)
					:location(location)
				{
				};

			};
			using Event = ArrivingCustomerEvent;

			//There might be some other variables that will determine how this MDP functions:
			int64_t GridSize = 22;//22=Amsterdam case
			int64_t NumVehicles = 20;
			int64_t DepotLoc = 0;
			int64_t VehicleCap = 20;
			double dailycustomerProb = 1.0;
			std::vector<std::vector<int>> Adjacency{};
			double holdingCosts = 5.0;
			double rejectionCosts = 50.0;

			static constexpr int64_t decisionType = 1;

			DynaPlex::DiscreteDist demandProbs;
			std::vector<std::vector<std::byte>> ActionMatrix{};
			std::vector<std::vector<int64_t>> enumerationMatrix{};

			//for features
			std::vector<std::vector<std::byte>> distanceMatrix{};
			std::vector<std::byte> edgeMatrix{};
			std::vector<std::vector<std::byte>>quadrantMatrix{};

			int64_t numEvents = 300;

			static constexpr int64_t featureSet = 0;//0 = engineered, 1= engineerder+state, 2 = state only

			struct Coordinate
			{
				int64_t row{ 0 };
				int64_t column{ 0 };
			};

			int64_t CoordinateToGraphElement(const Coordinate Coord) const;
			void GraphElementToCoordinate(Coordinate& coord, const int64_t element) const;
			std::vector<int64_t> getPotentialActions(const int64_t location) const;
			bool IsAllowedNeighbor(const int64_t i, const int64_t j) const;
			int64_t StepToDepotEnum(int64_t currentLoc) const;

			int64_t GetNumDemand(const State& state) const;
			void ApplyAction(State& state, int64_t action) const;
			std::byte Distance(const int64_t startPoint, const int64_t endPoint) const;
			float Variance(std::vector<float> samples) const;

			std::vector<int64_t> GetCheapestInsertionRoute(const State& state, const int64_t newInsertion, const int64_t currentPacman) const;
			int GetRoutingDistance(const std::vector<int64_t> routingSequence, const int64_t pacmanLocation, const int pacmanCapacity) const;
			void ApplyActionAssign(State& state) const;
			void ApplyActionAssignperVehicle(State& state, int64_t vehicle, int64_t deviatedLoc) const;
			int64_t StepTo(int64_t goal, std::vector<int64_t>potentialActions) const;
			bool hasArrivedAtCustomer(int vehicle, const State& state) const;
			void removeFirstCustomerFromRoute(int vehicle, State& state) const;
			int64_t getFirstCustomerLocation(int vehicle, const State& state) const;
			int64_t GetStartIndex(const State& state, const int64_t vehicle) const;


			//standard dynaplex functions
			double ModifyStateWithAction(State&, int64_t action) const;
			double ModifyStateWithEvent(State&, const Event& ) const;
			Event GetEvent(DynaPlex::RNG& rng) const;
			DynaPlex::VarGroup GetStaticInfo() const;
			DynaPlex::StateCategory GetStateCategory(const State&) const;
			bool IsAllowedAction(const State& state, int64_t action) const;			
			State GetInitialState() const;
			State GetState(const VarGroup&) const;
			//void RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>&) const;
			void GetFeatures(const State&, DynaPlex::Features&) const;
			explicit MDP(const DynaPlex::VarGroup&);
		};
	}
}

