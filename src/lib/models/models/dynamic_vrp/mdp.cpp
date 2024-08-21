#include "mdp.h"
#include "dynaplex/erasure/mdpregistrar.h"
#include <numeric>


namespace DynaPlex::Models {
	namespace dynamic_vrp /*keep this in line with id below and with namespace name in header*/
	{
		VarGroup MDP::GetStaticInfo() const
		{
			VarGroup vars;
			int64_t numvalidactions = 0;
			if constexpr (decisionType == 0)
			{//one step per vehicle
				numvalidactions = std::pow(5, NumVehicles);
			}
			else if constexpr (decisionType == 1)
			{//assignment of customer to routes
				numvalidactions = NumVehicles;
			}
			else if constexpr (decisionType == 2)
			{//hybrid, assignment + possibility to deviate from planned route
				numvalidactions = NumVehicles + std::pow(5, NumVehicles);
			}
			else
			{//iterative one step, used for NNPFA
				numvalidactions = 5;
			}

			vars.Add("valid_actions", numvalidactions);
			vars.Add("horizon_type", "infinite");

			return vars;
		}


		double MDP::ModifyStateWithEvent(State& state, const Event& event) const
		{
			if (event.location == (GridSize * GridSize))
			{
				state.newDemand = event.location;
				state.cat = StateCategory::AwaitAction();//no demand added
			}
			else
			{
				state.Demand[event.location] = 1;
				state.cat = StateCategory::AwaitAction();
				state.newDemand = event.location;
			}

			//after processing this event, we await an action.
			state.cat = StateCategory::AwaitAction();
			state.day++;
			state.decision1 = true;
			return 0.0;
		}

		double MDP::ModifyStateWithAction(MDP::State& state, int64_t action) const
		{
			if constexpr (decisionType == 1)
			{
				//part I: route allocation
				if (state.newDemand != (GridSize * GridSize))
				{
					state.Vehicles_route = GetCheapestInsertionRoute(state, state.newDemand, action);
				}

				ApplyActionAssign(state);
				state.Vehicles_numcust[action] += 1;
				state.cat = StateCategory::AwaitEvent();

				state.currentVehicle = 0;
				return GetNumDemand(state) * holdingCosts;

			}
			if constexpr (decisionType == 2)
			{//hybrid decision

				if (state.decision1 == true)//assignment decision
				{
					int64_t alloc = rand() % (NumVehicles);//just for debug
					//part I: route allocation
					if (state.newDemand != (GridSize * GridSize))
					{
						state.Vehicles_route = GetCheapestInsertionRoute(state, state.newDemand, alloc);
					}
					state.decision1 = false;
					state.Vehicles_numcust[alloc] += 1;
					return 0.0;
				}
				else
				{
					for (int64_t i = 0; i < NumVehicles; i++)
					{
						std::vector<int64_t> PotentialActions = getPotentialActions(state.Vehicles_loc[i]);

						int64_t derivedAction = PotentialActions[enumerationMatrix[action][i]];

						ApplyActionAssignperVehicle(state, i, derivedAction);
					}

					//after processing this event, we await an event.
					state.cat = StateCategory::AwaitEvent();

					state.currentVehicle = 0;
					return GetNumDemand(state) * holdingCosts;
				}



			}
			if constexpr (decisionType == 3)
			{//for NNPFA
				std::vector<int64_t> PotentialActions = getPotentialActions(state.Vehicles_loc[state.currentVehicle]);

				int64_t derivedAction = PotentialActions[action];

				ApplyAction(state, derivedAction);


				if (state.currentVehicle == NumVehicles)
				{
					state.cat = StateCategory::AwaitEvent();

					state.currentVehicle = 0;
					return GetNumDemand(state) * holdingCosts;
				}
				else
				{
					return 0.0;
				}

			}
			else
			{
				for (int64_t i = 0; i < NumVehicles; i++)
				{
					std::vector<int64_t> PotentialActions = getPotentialActions(state.Vehicles_loc[i]);

					int64_t derivedAction = PotentialActions[enumerationMatrix[action][i]];

					ApplyAction(state, derivedAction);
				}

				//after processing this event, we await an event.
				state.cat = StateCategory::AwaitEvent();

				state.currentVehicle = 0;
				return GetNumDemand(state) * holdingCosts;
			}
		}

		DynaPlex::VarGroup MDP::State::ToVarGroup() const
		{
			//add all state variables
			DynaPlex::VarGroup vars;
			vars.Add("cat", cat);
			vars.Add("Vehicles_loc", Vehicles_loc);
			vars.Add("Vehicles_remainingCapacity", Vehicles_remainingCapacity);
			vars.Add("Vehicles_route", Vehicles_route);
			vars.Add("Vehicles_numcust", Vehicles_numcust);
			vars.Add("Demand", Demand);
			vars.Add("currentVehicle", currentVehicle);
			vars.Add("depotVisits", depotVisits);
			vars.Add("day", day);
			vars.Add("newDemand", newDemand);
			vars.Add("decision1", decision1);
			vars.Add("countDeviation", countDeviation);
			return vars;
		}

		MDP::State MDP::GetState(const VarGroup& vars) const
		{
			State state{};
			vars.Get("cat", state.cat);
			vars.Get("Vehicles_loc", state.Vehicles_loc);
			vars.Get("Vehicles_remainingCapacity", state.Vehicles_remainingCapacity);
			vars.Get("Vehicles_route", state.Vehicles_route);
			vars.Get("Vehicles_numcust", state.Vehicles_numcust);
			vars.Get("Demand", state.Demand);
			vars.Get("currentVehicle", state.currentVehicle);
			vars.Get("depotVisits", state.depotVisits);
			vars.Get("day", state.day);
			vars.Get("newDemand", state.newDemand);
			vars.Get("decision1", state.decision1);
			vars.Get("countDeviation", state.countDeviation);
			return state;
		}

		MDP::State MDP::GetInitialState() const
		{			
			State state{};
			for (int64_t i = 0; i < NumVehicles; i++)
			{
				state.Vehicles_remainingCapacity.push_back(VehicleCap);
				state.Vehicles_loc.push_back(DepotLoc);
				state.Vehicles_numcust.push_back(0);
			}
			state.Vehicles_route = {};

			state.countDeviation = 0;
			state.currentVehicle = 0;
			state.day = 0;
			state.decision1 = false;
			state.depotVisits = 0;
			state.newDemand = 0;
			for (int64_t i = 0; i < GridSize * GridSize; i++)
			{
				state.Demand.push_back(0);
			}
			state.cat = StateCategory::AwaitEvent();

			return state;
		}

		MDP::MDP(const VarGroup& config)
		{
			config.Get("GridSize", GridSize);
			config.Get("NumVehicles", NumVehicles);
			config.Get("DepotLoc", DepotLoc);
			config.Get("VehicleCap", VehicleCap);
			config.Get("dailycustomerProb", dailycustomerProb);
			config.Get("holdingCosts", holdingCosts);
			config.Get("rejectionCosts", rejectionCosts);

			//for speed reasons we use a constexpr here, so you can set decisonType diretcly in mdp.h
			//config.Get("decisionType", decisionType);//{ oneStep = 0, assignment = 1, hybrid = 2 }; 3 = iterative one step


			//Here, we configure the MDP
			std::vector<double> demProbs;
			double totalProb = 0.0;

			// Distribute dailycustomerProb equally across the grid cells
			for (int64_t i = 0; i < GridSize * GridSize; i++) {
				double prob = dailycustomerProb / static_cast<double>(GridSize * GridSize);
				demProbs.push_back(prob);
				totalProb += prob;
			}
			//demProbs.push_back(1.0 - totalProb);

			// Verify that the probabilities sum to exactly 1.0
			double finalSum = std::accumulate(demProbs.begin(), demProbs.end(), 0.0);
			if (std::abs(finalSum - 1.0) > 1e-9) {  // Tolerance for floating-point precision
				std::cout << "Program logic error, probabilities do not sum up to 1.0. Final sum: " << finalSum << std::endl;
				throw std::runtime_error("Program logic error, probabilities do not sum up to 1.0");
			}

			// Now create the custom distribution using the generated probabilities
			demandProbs = DiscreteDist::GetCustomDist(demProbs);


			if constexpr (decisionType == 0 || decisionType == 3 || decisionType == 2)
			{
				enumerationMatrix = std::vector<std::vector<int64_t>>((int64_t)pow(5, NumVehicles), std::vector<int64_t>(NumVehicles));
				int64_t p{ 0 };
				int64_t iter{ 0 };
				std::vector<int64_t> helperVector{};
				std::vector<int64_t> maxTo{};
				for (int64_t i = 0; i < NumVehicles; i++)
				{
					helperVector.push_back(0);
					maxTo.push_back(5);
				}
				helperVector.push_back(0);
				maxTo.push_back(0);

				while (helperVector.back() == 0)
				{
					for (int64_t i = 0; i < NumVehicles; i++)
					{
						enumerationMatrix[iter][i] = helperVector[i];
					}

					helperVector[0]++;
					while (helperVector[p] == maxTo[p])
					{
						helperVector[p] = 0;
						p++;
						helperVector[p]++;
						if (helperVector[p] != maxTo[p])
						{
							p = 0;
						}
					}
					iter++;
				}
				if (decisionType != 3)
				{
					std::cout << "Program logic error or settings are wrong";
					throw "Program logic error or settings are wrong";
				}

				//matrix contains all allowed actions from every position on the grid
				ActionMatrix = std::vector<std::vector<std::byte>>(GridSize * GridSize, std::vector<std::byte>(5));
				for (int64_t i = 0; i < GridSize * GridSize; i++)
				{
					std::vector<int64_t> PotentialActions = getPotentialActions(i);
					for (int64_t a = 0; a < 5; a++)
					{
						if (IsAllowedNeighbor(i, PotentialActions[a]))//for not including positions outside grid
						{
							ActionMatrix[i][a] = (std::byte)1;
						}
					}
				}
			}

			//distance matrix for fature calculations
			distanceMatrix = std::vector<std::vector<std::byte>>(GridSize * GridSize, std::vector<std::byte>(GridSize * GridSize, (std::byte)0));
			for (int64_t i = 0; i < GridSize * GridSize; i++)
			{
				for (int64_t j = 0; j < GridSize * GridSize; j++)
				{
					distanceMatrix[i][j] = Distance(i, j);
				}
			}

			//matrix that checks if location is on edge of grid, used for features
			edgeMatrix = std::vector<std::byte>(GridSize * GridSize, (std::byte)0);
			for (int64_t i = 0; i < GridSize * GridSize; i++)
			{
				Coordinate coordi{};
				GraphElementToCoordinate(coordi, i);
				if (coordi.column == 0 || coordi.column == (GridSize - 1) || coordi.row == 0 || coordi.row == (GridSize - 1))
				{
					edgeMatrix[i] = (std::byte)1;
				}
			}

			quadrantMatrix = std::vector<std::vector<std::byte>>(GridSize * GridSize, std::vector<std::byte>(4, (std::byte)0));
			for (int64_t i = 0; i < GridSize * GridSize; i++)
			{
				Coordinate coordi{};
				GraphElementToCoordinate(coordi, i);
				if (coordi.column < 6 && coordi.row < 6)
				{
					quadrantMatrix[i][0] = (std::byte)1;
				}
				else if (coordi.column > 5 && coordi.row < 6)
				{
					quadrantMatrix[i][1] = (std::byte)1;
				}
				else if (coordi.column < 6 && coordi.row > 5)
				{
					quadrantMatrix[i][2] = (std::byte)1;
				}
				else
				{
					quadrantMatrix[i][3] = (std::byte)1;
				}
			}
		
		}


		MDP::Event MDP::GetEvent(RNG& rng) const {
			int64_t loc = demandProbs.GetSample(rng);
			return MDP::ArrivingCustomerEvent(loc);
		}


		void MDP::GetFeatures(const State& state, DynaPlex::Features& features)const {
			if constexpr (featureSet == 1 || featureSet == 2)
			{
				for (int64_t i = 0; i < GridSize * GridSize; i++)
				{
					features.Add(0.0);
				}
				for (int64_t i = 0; i < NumVehicles; i++)
				{
					features[state.Vehicles_loc[i]] = 1.0;
				}
				for (int64_t i = 0; i < GridSize * GridSize; i++)
				{
					if (state.Demand[i] == 1)
					{
						features.Add(1.0);
					}
					else
					{
						features.Add(0.0);
					}
				}
			}

			if constexpr (featureSet == 0 || featureSet == 1)
			{
				//first some calculations
				int sumDistVeh = 0, sumDistDemand = 0, sumDistDepot = 0, sumCapacity = 0, mult_capDistdepot = 0, edgeVehicles = 0;
				std::vector<float> distAssign(NumVehicles, 0.0);
				std::vector<float> numAssign(NumVehicles, 0.0);
				std::vector<float>quadrant{ 0,0,0,0 };
				for (int64_t i = 0; i < NumVehicles; i++)
				{
					sumCapacity += state.Vehicles_remainingCapacity[i];
					sumDistDepot += (int)distanceMatrix[state.Vehicles_loc[i]][DepotLoc];
					mult_capDistdepot += state.Vehicles_remainingCapacity[i] * (int)distanceMatrix[state.Vehicles_loc[i]][DepotLoc];
					for (int64_t j = 0; j < NumVehicles; j++)
					{
						sumDistVeh += (int)distanceMatrix[state.Vehicles_loc[i]][state.Vehicles_loc[j]];
					}
					for (int64_t d = 0; d < GridSize * GridSize; d++)
					{
						if (state.Demand[d] == 1)
						{
							sumDistDemand += (int)distanceMatrix[state.Vehicles_loc[i]][d];
						}
					}
					if (edgeMatrix[state.Vehicles_loc[i]] == (std::byte)1)
					{
						edgeVehicles++;
					}
					for (int64_t q = 0; q < 4; q++)
					{
						if (quadrantMatrix[state.Vehicles_loc[i]][q] == (std::byte)1)
						{
							quadrant[q]++;
						}
					}
					if constexpr (decisionType != 0)
					{
						numAssign[i] = state.Vehicles_numcust[i];
						int64_t startIdx = 0;
						for (int v = 0; v < i; ++v) {
							startIdx += state.Vehicles_numcust[v];  // Sum up the number of customers for all previous vehicles
						}

						// Initialize distance for the current vehicle
						distAssign[i] = 0;

						// Loop through each customer in the current vehicle's route
						for (int64_t k = 0; k < state.Vehicles_numcust[i]; ++k) {
							int64_t customer1 = state.Vehicles_route[startIdx + k];
							int64_t customer2 = state.Vehicles_route[startIdx + ((k + 1) % state.Vehicles_numcust[i])]; 

							// Sum the distances between the consecutive customers
							distAssign[i] += static_cast<int>(distanceMatrix[customer1][customer2]);
						}
					}
				}

				features.Add(GetNumDemand(state));
				features.Add(sumDistVeh);
				features.Add(sumDistDemand);
				features.Add(sumDistDepot);
				features.Add(sumCapacity);
				features.Add(mult_capDistdepot);
				features.Add(edgeVehicles);
				features.Add(Variance(quadrant));
				if constexpr (decisionType != 0 && decisionType != 3)
				{
					features.Add(std::accumulate(distAssign.begin(), distAssign.end(), 0.0) / distAssign.size());
					features.Add(Variance(distAssign));
					features.Add(Variance(numAssign));
				}

			}
			if constexpr (decisionType == 3)
			{//iterative decision
				features.Add(state.currentVehicle);
			}
		}
		
		//void MDP::RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>& registry) const
		//{//Here, we register any custom heuristics we want to provide for this MDP.	
		// //On the generic DynaPlex::MDP constructed from this, these heuristics can be obtained
		// //in generic form using mdp->GetPolicy(VarGroup vars), with the id in var set
		// //to the corresponding id given below.
		//	registry.Register<EmptyPolicy>("empty_policy",
		//		"This policy is a place-holder, and throws a NotImplementedError when asked for an action. ");
		//}

		DynaPlex::StateCategory MDP::GetStateCategory(const State& state) const
		{
			//this typically works, but state.cat must be kept up-to-date when modifying states. 
			return state.cat;
		}

		bool MDP::IsAllowedAction(const State& state, int64_t action) const {
			if constexpr (decisionType == 1)
			{
				return true;//assignment always allowed
			}
			if constexpr (decisionType == 3)
			{
				if (ActionMatrix[state.Vehicles_loc[state.currentVehicle]][action] != (std::byte)1)
				{
					return false;
				}
				if (state.Vehicles_remainingCapacity[state.currentVehicle] == 0)
				{
					if (action != StepToDepotEnum(state.Vehicles_loc[state.currentVehicle]))
					{
						return false;
					}
				}
				return true;
				/*	int64_t minDec = state.currentVehicle * 5;
					int64_t maxDec = ((state.currentVehicle + 1) * 5) -1;
					if (action<= maxDec && action >= minDec)
					{
						return true;
					}
					else
					{
						return false;
					}*/

			}
			else
			{
				for (int64_t i = 0; i < NumVehicles; i++)
				{
					if (ActionMatrix[state.Vehicles_loc[i]][enumerationMatrix[action][i]] != (std::byte)1)
					{
						return false;
					}
					if (state.Vehicles_remainingCapacity[i] == 0)
					{//when no capacity left, only allow step towards depot
						if (enumerationMatrix[action][i] != StepToDepotEnum(state.Vehicles_loc[i]))
						{
							return false;
						}
					}
				}
			}

			return true;
		}


		void MDP::ApplyActionAssign(State& state) const
		{
			//part IIb: implicit route execution (single step)
			for (int64_t i = 0; i < NumVehicles; i++)
			{
				std::vector<int64_t> PotentialActions = getPotentialActions(state.Vehicles_loc[i]);
				if (state.Vehicles_remainingCapacity[i] == 0)
				{
					//move to depot

					state.Vehicles_loc[i] = PotentialActions[StepTo(DepotLoc, PotentialActions)];

					if (state.Vehicles_loc[i] == DepotLoc)
					{//unload at depot
						state.Vehicles_remainingCapacity[i] = VehicleCap;
						state.depotVisits++;
					}

					continue;//no cherries can be eaten untill the current load is delivered at depot
				}
				else
				{
					if (state.Vehicles_numcust[i] == 0)
					{//only execute an action if there is a routing plan
						continue;
					}

					state.Vehicles_loc[i] = PotentialActions[StepTo(getFirstCustomerLocation(i, state), PotentialActions)];
					//execute first element in routing plan

					//if (state.Vehicles[i].location == DepotLoc)
					//{//unload at depot, also if we just pass by
					//	state.Vehicles[i].remainingCapacity = VehicleCap;
					//}
				}

				if (hasArrivedAtCustomer(i, state) && state.Vehicles_remainingCapacity[i] > 0)
				{//if we arrived at the cherry,

					state.Demand[state.Vehicles_loc[i]] = 0;
					if (state.Vehicles_loc[i] != DepotLoc)
					{//only reduce capacity of demand did not pop up at the depot, AND we use the MDP depot variant
						state.Vehicles_remainingCapacity[i] = state.Vehicles_remainingCapacity[i] - (int64_t)1;
					}

					removeFirstCustomerFromRoute(i, state);
				}
			}
		}
		void MDP::ApplyActionAssignperVehicle(State& state, int64_t vehicle, int64_t deviatedLoc) const
		{
			//part IIb: implicit route execution (single step)
			std::vector<int64_t> PotentialActions = getPotentialActions(state.Vehicles_loc[vehicle]);
			if (state.Vehicles_remainingCapacity[vehicle] == 0)
			{
				//move to depot

				state.Vehicles_loc[vehicle] = PotentialActions[StepTo(DepotLoc, PotentialActions)];

				if (state.Vehicles_loc[vehicle] == DepotLoc)
				{//unload at depot
					state.Vehicles_remainingCapacity[vehicle] = VehicleCap;
					state.depotVisits++;
				}

				return;
				//continue;//no cherries can be eaten untill the current load is delivered at depot
			}
			else
			{
				if (state.Vehicles_numcust[vehicle] == 0)
				{//only execute an action if there is a routing plan
					state.Vehicles_loc[vehicle] = deviatedLoc;
					return;
				}
				else
				{
					int64_t routeLoc = PotentialActions[StepTo(getFirstCustomerLocation(vehicle, state), PotentialActions)];
					if (deviatedLoc != routeLoc && deviatedLoc != state.Vehicles_loc[vehicle])
					{
						state.Vehicles_loc[vehicle] = deviatedLoc;
						state.countDeviation++;
					}

					state.Vehicles_loc[vehicle] = PotentialActions[StepTo(getFirstCustomerLocation(vehicle, state), PotentialActions)];
				}

				//execute first element in routing plan

				//if (state.Vehicles[i].location == DepotLoc)
				//{//unload at depot, also if we just pass by
				//	state.Vehicles[i].remainingCapacity = VehicleCap;
				//}
			}

			if (hasArrivedAtCustomer(vehicle, state) && state.Vehicles_remainingCapacity[vehicle] > 0)
			{//if we arrived at the cherry,

				state.Demand[state.Vehicles_loc[vehicle]] = 0;
				if (state.Vehicles_loc[vehicle] != DepotLoc)
				{//only reduce capacity of demand did not pop up at the depot, AND we use the MDP depot variant
					state.Vehicles_remainingCapacity[vehicle] = state.Vehicles_remainingCapacity[vehicle] - (int64_t)1;
				}

				removeFirstCustomerFromRoute(vehicle, state);
			}
			return;
		}

		void MDP::GraphElementToCoordinate(Coordinate& coord, const int64_t element) const
		{
			coord.row = element / GridSize;//should always floor because it is int64_t
			coord.column = element - (coord.row * GridSize);
		}

		int64_t MDP::CoordinateToGraphElement(const Coordinate Coord) const
		{
			return Coord.column + (Coord.row * GridSize);
		}


		std::vector<int64_t> MDP::getPotentialActions(const int64_t location) const
		{
			std::vector<int64_t> potentialActions{};
			Coordinate coordi{};
			Coordinate newCoord{};
			GraphElementToCoordinate(coordi, location);

			newCoord.column = coordi.column;
			newCoord.row = coordi.row;
			potentialActions.push_back(CoordinateToGraphElement(newCoord));//stay

			newCoord.column = coordi.column;
			newCoord.row = coordi.row - (int64_t)1;
			potentialActions.push_back(CoordinateToGraphElement(newCoord));//left

			newCoord.column = coordi.column;
			newCoord.row = coordi.row + (int64_t)1;
			potentialActions.push_back(CoordinateToGraphElement(newCoord));//right

			newCoord.column = coordi.column - (int64_t)1;
			newCoord.row = coordi.row;
			potentialActions.push_back(CoordinateToGraphElement(newCoord));//bottom

			newCoord.column = coordi.column + (int64_t)1;
			newCoord.row = coordi.row;
			potentialActions.push_back(CoordinateToGraphElement(newCoord));//top

			return potentialActions;//will return some positions outside grid, but this is handled later
		}

		bool MDP::IsAllowedNeighbor(const int64_t i, const int64_t j) const
		{
			Coordinate coordi{};
			GraphElementToCoordinate(coordi, i);
			std::vector <Coordinate> allowedNeighbors{};
			//stay at current location (i==j)
			allowedNeighbors.push_back(Coordinate{ coordi.row ,coordi.column });
			//move actions
			allowedNeighbors.push_back(Coordinate{ coordi.row - (int64_t)1,coordi.column });
			allowedNeighbors.push_back(Coordinate{ coordi.row + (int64_t)1,coordi.column });
			allowedNeighbors.push_back(Coordinate{ coordi.row ,coordi.column - (int64_t)1 });
			allowedNeighbors.push_back(Coordinate{ coordi.row ,coordi.column + (int64_t)1 });

			Coordinate coordj{};
			GraphElementToCoordinate(coordj, j);
			for (auto& nghbr : allowedNeighbors)
			{
				if (nghbr.row == coordj.row && nghbr.column == coordj.column && coordj.row < GridSize && coordj.column < GridSize)
				{
					return true;
				}
			}
			return false;
		}

		int64_t MDP::StepToDepotEnum(int64_t currentLoc) const
		{//this function could be written better, it returns a step towards the depot for the enumeration action space
			Coordinate depotCoord{};
			Coordinate pacmanCoord{};
			GraphElementToCoordinate(depotCoord, DepotLoc);
			GraphElementToCoordinate(pacmanCoord, currentLoc);//legacy joke

			if (depotCoord.column != pacmanCoord.column)
			{
				if (depotCoord.column > pacmanCoord.column)
				{
					return (int64_t)4;
				}
				else
				{
					return (int64_t)3;
				}
			}
			else
			{
				if (depotCoord.row > pacmanCoord.row)
				{
					return (int64_t)2;
				}
				else
				{
					return (int64_t)1;
				}
			}
		}

		int64_t MDP::StepTo(int64_t goal, std::vector<int64_t>potentialActions) const
		{
			int minDist = 1000000;
			int64_t bestAction = 100;
			for (int64_t i = 0; i < potentialActions.size(); i++)
			{
				if (potentialActions[i] < (GridSize * GridSize) && potentialActions[i] >= 0)
				{
					int dist = (int)distanceMatrix[potentialActions[i]][goal];
					if (dist < minDist)
					{
						minDist = dist;
						bestAction = i;
					}
				}

			}

			return bestAction;

		}

		int64_t MDP::GetNumDemand(const State& state) const
		{
			int64_t numCustomers{ 0 };
			for (auto& n : state.Demand)
			{
				numCustomers += n;//calculate holdingCosts before addition of new customer
			}
			return numCustomers;
		}

		void MDP::ApplyAction(State& state, int64_t action) const
		{
			//move to new location
			state.Vehicles_loc[state.currentVehicle] = action;

			if (state.Vehicles_loc[state.currentVehicle] == DepotLoc)
			{
				state.Vehicles_remainingCapacity[state.currentVehicle] = VehicleCap;
				state.depotVisits++;
			}
			//eat potential cherry (when capacity left)
			if (state.Vehicles_remainingCapacity[state.currentVehicle] > 0)
			{
				state.Demand[state.Vehicles_loc[state.currentVehicle]] = 0;
				if (state.Vehicles_loc[state.currentVehicle] != DepotLoc)
				{//only reduce capacity of demand did not pop up at the depot, AND we use the MDP depot variant
					state.Vehicles_remainingCapacity[state.currentVehicle] = state.Vehicles_remainingCapacity[state.currentVehicle] - (int64_t)1;
				}

			}

			state.currentVehicle++;
		}

		std::byte MDP::Distance(const int64_t startPoint, const int64_t endPoint) const
		{
			Coordinate startCoordinate{};
			GraphElementToCoordinate(startCoordinate, startPoint);
			Coordinate endCoordinate{};
			GraphElementToCoordinate(endCoordinate, endPoint);

			int horizontalDistance = abs((int)startCoordinate.row - (int)endCoordinate.row);
			int verticalDistance = abs((int)startCoordinate.column - (int)endCoordinate.column);

			return (std::byte)(horizontalDistance + verticalDistance);
		}
		float MDP::Variance(std::vector<float> samples) const
		{
			int64_t size = samples.size();

			float variance = 0;
			float t = samples[0];
			for (int64_t i = 1; i < size; i++)
			{
				t += samples[i];
				float diff = ((i + 1) * samples[i]) - t;
				variance += (diff * diff) / (float)((i + 1.0) * i);
			}

			return variance / (size);//population variance
		}
		std::vector<int64_t> MDP::GetCheapestInsertionRoute(const State& state, const int64_t newInsertion, const int64_t currentPacman) const
		{
			// If the current Pacman's route is empty, directly insert the new customer
			if (state.Vehicles_numcust[currentPacman] == 0)
			{
				std::vector<int64_t> updatedRoute = state.Vehicles_route;  // Copy the entire route
				updatedRoute.insert(updatedRoute.begin() + GetStartIndex(state, currentPacman), newInsertion);
				return updatedRoute;
			}

			int currentBestCosts = std::numeric_limits<int>::max();
			std::vector<int64_t> currentBestRoute = state.Vehicles_route;  // Copy the entire route as the base

			// Define the starting and ending index in the 1D vector for the current vehicle
			int64_t startIdx = GetStartIndex(state, currentPacman);
			int64_t endIdx = startIdx + state.Vehicles_numcust[currentPacman];

			// Create a temporary route vector that contains only the current vehicle's route
			std::vector<int64_t> vehicleRoute(state.Vehicles_route.begin() + startIdx, state.Vehicles_route.begin() + endIdx);

			// Iterate through all possible insertion points in the current vehicle's route
			for (int64_t i = 0; i <= static_cast<int64_t>(vehicleRoute.size()); ++i)
			{
				std::vector<int64_t> tempRoute = vehicleRoute;
				tempRoute.insert(tempRoute.begin() + i, newInsertion);

				// Calculate the cost of this temporary route
				int tempCosts = GetRoutingDistance(tempRoute, state.Vehicles_loc[currentPacman], state.Vehicles_remainingCapacity[currentPacman]);

				// If the new route is cheaper, update the best route and costs
				if (tempCosts < currentBestCosts)
				{
					currentBestCosts = tempCosts;

					// Update the full route with the new best route for the current vehicle
					currentBestRoute = state.Vehicles_route;  // Start with the full route
					currentBestRoute.erase(currentBestRoute.begin() + startIdx, currentBestRoute.begin() + endIdx);  // Remove old route section
					currentBestRoute.insert(currentBestRoute.begin() + startIdx, tempRoute.begin(), tempRoute.end());  // Insert the best new route
				}
			}

			// Return the updated full route for all vehicles
			return currentBestRoute;
		}

		// Helper function to calculate the starting index for the vehicle in the global route
		int64_t MDP::GetStartIndex(const State& state, const int64_t vehicle) const
		{
			int64_t startIdx = 0;
			for (int v = 0; v < vehicle; ++v) {
				startIdx += state.Vehicles_numcust[v];
			}
			return startIdx;
		}

		bool MDP::hasArrivedAtCustomer(int vehicle, const State& state) const
		{
			// Calculate the start index of the vehicle's route in the 1D route vector
			int64_t startIdx = 0;
			for (int v = 0; v < vehicle; ++v) {
				startIdx += state.Vehicles_numcust[v];  // Sum up the number of customers for all previous vehicles
			}

			// Check if the first customer in the route matches the vehicle's current location
			if (state.Vehicles_numcust[vehicle] > 0) {  // Ensure the vehicle has customers in its route
				return state.Vehicles_route[startIdx] == state.Vehicles_loc[vehicle];
			}

			// If no customers, consider it has not arrived
			return false;
		}

		void MDP::removeFirstCustomerFromRoute(int vehicle, State& state) const
		{
			// Calculate the start index of the vehicle's route in the 1D route vector
			int64_t startIdx = 0;
			for (int v = 0; v < vehicle; ++v) {
				startIdx += state.Vehicles_numcust[v];  // Sum up the number of customers for all previous vehicles
			}

			// Check if the vehicle has customers in its route
			if (state.Vehicles_numcust[vehicle] > 0) {
				// Erase the first customer from the vehicle's route
				state.Vehicles_route.erase(state.Vehicles_route.begin() + startIdx);

				// Decrease the number of customers for this vehicle
				state.Vehicles_numcust[vehicle]--;
			}
		}

		int64_t MDP::getFirstCustomerLocation(int vehicle, const State& state) const
		{
			// Calculate the start index of the vehicle's route in the 1D route vector
			int64_t startIdx = 0;
			for (int v = 0; v < vehicle; ++v) {
				startIdx += state.Vehicles_numcust[v];  // Sum up the number of customers for all previous vehicles
			}

			// Check if the vehicle has customers in its route
			if (state.Vehicles_numcust[vehicle] > 0) {
				// Return the first customer's location
				return state.Vehicles_route[startIdx];
			}

			// Return an invalid value if no customers exist
			return -1; 
		}


		int MDP::GetRoutingDistance(const std::vector<int64_t> routingSequence, const int64_t pacmanLocation, const int pacmanCapacity) const
		{
			int totalRoutingDist{ 0 };
			int64_t curLocation = pacmanLocation;
			int curCapacity = pacmanCapacity;

			for (auto visit : routingSequence)
			{
				if (curCapacity == 0)
				{
					totalRoutingDist += (int)distanceMatrix[curLocation][DepotLoc];
					curCapacity = VehicleCap;
					curLocation = DepotLoc;
				}

				totalRoutingDist += (int)distanceMatrix[curLocation][visit];
				curCapacity = std::max((int64_t)0, (curCapacity - (int64_t)1));
				curLocation = visit;
			}

			return totalRoutingDist;
		}


		void Register(DynaPlex::Registry& registry)
		{
			//To use this MDP with dynaplex, register it like so, setting name equal to namespace and directory name
			// and adding appropriate description. 
			DynaPlex::Erasure::MDPRegistrar<MDP>::RegisterModel(
				"dynamic_vrp",
				"<A dynamic VRP with different decision space variants>",
				registry); 
		}
	}
}

