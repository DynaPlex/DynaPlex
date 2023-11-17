#include "mdp.h"
#include "dynaplex/erasure/mdpregistrar.h"
#include "policies.h"


namespace DynaPlex::Models {
	namespace order_picking /*keep this in line with id below and with namespace name in header*/
	{
		VarGroup MDP::GetStaticInfo() const
		{
			VarGroup vars;
			vars.Add("valid_actions", grid_size * grid_size);

			VarGroup feats{};
			vars.Add("features", feats);

			vars.Add("discount_factor", discount_factor);

			VarGroup::VarGroupVec dist_mat_vec_group{};
			for (size_t i = 0; i < distanceMatrix.size(); i++) {
				VarGroup dist_mat_var_group{};
				std::string key = "row_" + std::to_string(i);
				dist_mat_var_group.Add(key, distanceMatrix[i]);
				dist_mat_vec_group.push_back(dist_mat_var_group);
			}
			vars.Add("distance_matrix", dist_mat_vec_group);

			//potentially add any stuff that was computed for diagnostics purposes
			//not used by dynaplex framework itself. 
			VarGroup diagnostics{};
			vars.Add("diagnostics", diagnostics);
			return vars;
		}

		//check if a given action is possible given the current state
		bool MDP::IsAllowedAction(const State& state, int64_t action) const
		{
			//action inside the grid
			if (action >= 0 && action < n_valid_actions)
			{
				//staying in place is always allowed
				if (action == state.pickerList[state.currentPicker].location)
					return true;

				//check if destination contains an order
				bool orderDest = false;

				auto order = std::find_if(state.orderList.begin(), state.orderList.end(),
					[action](State::Order const& o) {
						return o.location == action; });
				if (order != state.orderList.end())
					orderDest = true;

				if (orderDest) {
					//if destination is an assigned order, edge not allowed
					if (std::find(state.assignedOrders.begin(), state.assignedOrders.end(), action) != state.assignedOrders.end())
						return false;
					else
						return true;
				}
			}
			return false;
		}

		double MDP::ModifyStateWithEvent(State& state, const Event& event) const
		{
			double movingCosts = 0;

			//before applying the event, move the agents
			for (auto picker : state.pickerList)
			{
				movingCosts += ApplyMove(state, GetBestMove(picker.location, picker.destination));
				state.currentPicker++;
			}
			state.currentPicker = 0;

			//holding costs computed before evolution
			double holdingCosts = GetHoldingCosts(state);

			//remove canceled orders and pick potential orders becoming active
			manageOrders(state, event.confirmedTransitions);

			//New orders are announced on the board
			for (int i = 0; i < max_orders_per_event; i++) {
				if (event.locations[i] != (grid_size * grid_size))
				{
					std::vector<int64_t> tws{ event.time_windows[i][0], event.time_windows[i][1], 0 };
					State::Order newOrder = State::Order(event.locations[i], 0, tws); //state = 0: orders are always confirmed before arriving

					//need to check if no order is at the location
					if (state.orderList.size() == 0)
					{
						state.orderList.push_back(newOrder);
					}
					else
					{
						bool flag_already_there = false;
						for (auto& order : state.orderList)
						{
							if (order.location == event.locations[i])
							{
								flag_already_there = true;
							}
						}
						if (!flag_already_there)
						{
							state.orderList.push_back(newOrder);
						}
					}
				}
			}

			//all the pickers are allocated to an order (no need to take any action)
			if (state.assignedOrders.size() == n_pickers)
			{
				state.cat = StateCategory::AwaitEvent();
			}
			else
			{
				for (size_t i = 0; i < state.pickerList.size(); i++)
				{
					if (!state.pickerList[i].allocated)
						state.decisionsRequired.push_back(i);
				}

				//await decision for first picker in list
				state.currentPicker = state.decisionsRequired[0];

				//set distances from current picker location
				state.currentDistances = distanceMatrix[state.pickerList[state.currentPicker].location];

				//state.status = CherryAllocationNAMDP::State::Status::AwaitAction;
				state.cat = StateCategory::AwaitAction();
			}

			state.changeActiveLocations();

			return movingCosts + holdingCosts;
		}

		//check if node is an obstacle
		//bool MDP::isObstacle(const int64_t i) const
		//{
		//	for (size_t j = 0; j < obstacleLocations.size(); j++)
		//	{
		//		if (i == obstacleLocations[j])
		//			return true;
		//	}
		//	return false;
		//}

		//returns true if j is adjacent to i and j is a valid position on the map (eg it does not exceed the map's dimensions) and j is not an obstacle
		bool MDP::IsAllowedNeighbor(const int64_t i, const int64_t j) const
		{
			//Check not out of borders
			if (j < 0 || j >= grid_size * grid_size)
				return false;

			//Check obstacles
			//if (isObstacle(j))
			//	return false;

			//Check maximum manhattan distance
			if (State::Distance(i, j, grid_size) > 1)
				return false;

			return true;
		}

		//get holding costs for orders in ongoing and tardy state
		double MDP::GetHoldingCosts(const State& state) const
		{
			size_t ongoingOrders{ 0 };
			size_t tardyOrders{ 0 };
			for (auto& o : state.orderList)
			{
				if (o.state == 1)
					ongoingOrders += 1;	//calculate holdingCosts before addition of new order
				if (o.state == 2)
					tardyOrders += 1;	//calculate holdingCosts before addition of new order
			}
			return ongoingOrders * holding_cost + tardyOrders * tardiness_cost;
		}

		//manage all the operations related to the evolution of orders
		//i.e. it decrements confirmed and ongoing orders' time_windows and increases tardy ones'
		//with given probability, it cancels orders either in state 0 at no cost (orders are canceled by the customer)
		void MDP::manageOrders(State& state, const std::vector<int64_t>& confirmedTransitions) const
		{
			size_t numOrders{ 0 };

			auto it = state.orderList.begin();
			while (it != state.orderList.end()) {

				size_t confirmedTransition = confirmedTransitions.at(it->location);
				size_t oldState = it->state;

				it->evolve(confirmedTransition);

				if (it->canceled) { //can only happen in state confirmed
					//if the order was assigned remove it from assigned list and deallocate related picker
					if (it->assigned >= 0) {
						state.assignedOrders.erase(std::remove(state.assignedOrders.begin(), state.assignedOrders.end(), it->location), state.assignedOrders.end());
						state.pickerList[it->assigned].allocated = false;
					}
					it = state.orderList.erase(it);
				}
				else if (oldState == 0 && it->state == 1) {
					//picker picks order if it evolves while it is on it, then delete order and deallocate related picker
					if (it->assigned >= 0 && state.pickerList[it->assigned].allocated &&
						state.pickerList[it->assigned].location == it->location &&
						state.pickerList[it->assigned].destination == it->location) {

						state.assignedOrders.erase(std::remove(state.assignedOrders.begin(), state.assignedOrders.end(), it->location), state.assignedOrders.end());
						state.pickerList[it->assigned].allocated = false;

						//orders picked before due date will stay on board until due date to handle clairvoyant solution event preprocessing
						it = state.orderList.erase(it);
					}
					else {
						++it;
					}
				}
				else {
					++it;
				}
			}
			return;
		}

		//helper function for ModifyStateWithAction
		double MDP::ApplyMove(State& state, int64_t move) const
		{
			//move to new location
			state.pickerList[state.currentPicker].location = move;

			//pick potential order
			for (int i = 0; i < state.orderList.size(); i++)
			{
				//pick a order only if it is in the ongoing or tardy state and only if the picker was allocated to it
				if (state.orderList[i].location == move &&
					(state.orderList[i].state == 1 || state.orderList[i].state == 2) &&
					state.orderList[i].assigned == state.currentPicker)
				{
					//delete order from assigned list
					state.assignedOrders.erase(std::remove(state.assignedOrders.begin(), state.assignedOrders.end(), move), state.assignedOrders.end());

					//delete order from the board
					state.orderList.erase(state.orderList.begin() + i);

					//free picker
					state.pickerList[state.currentPicker].allocated = false;

					//return no moving costs because picking happened
					return 0;
				}
			}

			return move_cost;
		}

		int64_t MDP::GetBestMove(const int64_t currentLocation, const int64_t destination) const
		{
			std::vector<int64_t> potentialMoves = GetPotentialMoves(currentLocation);

			int64_t currentSmallestDist = std::numeric_limits<int64_t>::max();
			int64_t currentBestMove{};

			for (auto move : potentialMoves)
			{
				if (IsAllowedNeighbor(currentLocation, move))
				{
					int dist = State::Distance(move, destination, grid_size);
					if (dist < currentSmallestDist)
					{
						currentSmallestDist = dist;
						currentBestMove = move;
					}
				}
			}
			return currentBestMove;
		}

		//computes available moves given the current picker location
		std::vector<int64_t> MDP::GetPotentialMoves(const int64_t location) const
		{
			std::vector<int64_t> potentialMoves{};
			State::Coordinate coordi{};
			State::Coordinate newCoord{};
			State::GraphElementToCoordinate(coordi, location, grid_size);

			newCoord.column = coordi.column;
			newCoord.row = coordi.row;
			potentialMoves.push_back(State::CoordinateToGraphElement(newCoord, grid_size));//stay

			newCoord.column = coordi.column;
			newCoord.row = coordi.row - 1;
			potentialMoves.push_back(State::CoordinateToGraphElement(newCoord, grid_size));//left

			newCoord.column = coordi.column;
			newCoord.row = coordi.row + 1;
			potentialMoves.push_back(State::CoordinateToGraphElement(newCoord, grid_size));//right

			newCoord.column = coordi.column - 1;
			newCoord.row = coordi.row;
			potentialMoves.push_back(State::CoordinateToGraphElement(newCoord, grid_size));//bottom

			newCoord.column = coordi.column + 1;
			newCoord.row = coordi.row;
			potentialMoves.push_back(State::CoordinateToGraphElement(newCoord, grid_size));//top

			return potentialMoves; //will return some positions outside grid, but this is handled later
		}

		double MDP::ModifyStateWithAction(State& state, int64_t action) const
		{
			//part I: explicit allocation
			state.pickerList[state.currentPicker].destination = action;

			for (size_t i = 0; i < state.orderList.size(); i++)
			{
				//assign order to a picker if the picker is moving there
				//only if it is not assigned yet (to make stay in place action always possible and prevent deadlocks)
				if (state.orderList[i].assigned == -1 && state.orderList[i].location == action &&
					state.orderList[i].state < 3)
				{
					state.pickerList[state.currentPicker].allocated = true;
					state.assignedOrders.push_back(state.orderList[i].location);
					state.orderList[i].assigned = int(state.currentPicker);
					break;
				}
			}

			if (state.decisionsRequired.size() > 0)
				state.decisionsRequired.erase(state.decisionsRequired.begin());
			else
				throw "Error: decision taken, but no decisions required!";

			if (state.decisionsRequired.size() == 0) //if all decisions have been taken
			{
				//move to events
				//state.status = CherryAllocationNAMDP::State::Status::AwaitCherry;
				state.cat = StateCategory::AwaitEvent();
				state.currentPicker = 0;
			}
			else
			{
				//await decision for next pacman in list
				state.currentPicker = state.decisionsRequired[0];

				//set distances from current pacman location
				state.currentDistances = distanceMatrix[state.pickerList[state.currentPicker].location];

				//state.status = CherryAllocationNAMDP::State::Status::AwaitAction;
				state.cat = StateCategory::AwaitAction();
			}

			state.changeActiveLocations();

			return 0;
		}

		DynaPlex::VarGroup MDP::State::ToVarGroup() const
		{
			DynaPlex::VarGroup vars;
			vars.Add("cat", cat);
			vars.Add("grid_size", GridSize);
			vars.Add("order_list", orderList);		
			vars.Add("picker_list", pickerList);	
			vars.Add("decisions_required", decisionsRequired);
			vars.Add("assigned_orders", assignedOrders);
			vars.Add("active_locations", activeLocations);
			vars.Add("current_distances", currentDistances);
			vars.Add("current_picker", currentPicker);
			return vars;
		}

		MDP::State MDP::GetState(const VarGroup& vars) const
		{
			State state{};

			vars.Get("cat", state.cat);
			//initiate any other state variables. 
			vars.Get("grid_size", state.GridSize);
			vars.Get("picker_list", state.pickerList);
			vars.Get("order_list", state.orderList);
			vars.Get("decisions_required", state.decisionsRequired);
			vars.Get("assigned_orders", state.assignedOrders);
			vars.Get("active_locations", state.activeLocations);
			vars.Get("current_distances", state.currentDistances);
			vars.Get("current_picker", state.currentPicker);
			return state;
		}



		MDP::State MDP::GetInitialState() const
		{
			int64_t currentPicker = 0;
			int64_t initialOrders = 0;
			std::vector<int64_t> initialOrderLocations;
			std::vector<int64_t> gridPositions;
			std::vector<int64_t> PickerLocations;
			std::vector<int64_t> activeLocations;

			if (manual_initial_state) //supply own initial state
			{
				if (grid_size == 6) {
					initialOrders = 2;
					PickerLocations = { 26, 9 };
					initialOrderLocations = { 1, 11 };

					std::vector<State::Picker> pickerList{ };
					std::vector<State::Order> orderList{ };
					std::vector<int64_t> decisionsRequired{ };

					for (size_t i = 0; i < n_pickers; i++) {
						State::Picker newPicker = State::Picker(PickerLocations[i]);
						pickerList.push_back(newPicker);

						//initially every pacman requires a new action
						decisionsRequired.push_back(i);

						//Add pacman locations to active ones
						activeLocations.push_back(PickerLocations[i]);
					}

					orderList.push_back(State::Order(initialOrderLocations[0], 1, { 0, 10, 0 }));
					orderList.push_back(State::Order(initialOrderLocations[1], 1, { 0, 10, 0 }));
					activeLocations.push_back(initialOrderLocations[0]);
					activeLocations.push_back(initialOrderLocations[1]);

					std::vector<int64_t> currentDistances = distanceMatrix[pickerList[currentPicker].location];

					return State(grid_size, orderList, pickerList, decisionsRequired, currentPicker, activeLocations, currentDistances, DynaPlex::StateCategory::AwaitAction());
				}
				else if (grid_size == 10) {
					initialOrders = 5;
					PickerLocations = { 26, 53, 50, 77, 30, 82 };
					initialOrderLocations = { 8, 37, 83, 51, 18 };

					std::vector<State::Picker> pickerList{ };
					std::vector<State::Order> orderList{ };
					std::vector<int64_t> decisionsRequired{ };

					for (size_t i = 0; i < n_pickers; i++) {
						State::Picker newPicker = State::Picker(PickerLocations[i]);
						pickerList.push_back(newPicker);

						//initially every pacman requires a new action
						decisionsRequired.push_back(i);

						//Add pacman locations to active ones
						activeLocations.push_back(PickerLocations[i]);
					}

					orderList.push_back(State::Order(initialOrderLocations[0], 1, { 0, 10, 0 }));
					orderList.push_back(State::Order(initialOrderLocations[1], 1, { 0, 9, 0 }));
					orderList.push_back(State::Order(initialOrderLocations[2], 1, { 0, 3, 0 }));
					orderList.push_back(State::Order(initialOrderLocations[3], 1, { 0, 8, 0 }));
					orderList.push_back(State::Order(initialOrderLocations[4], 1, { 0, 10, 0 }));
					activeLocations.push_back(initialOrderLocations[0]);
					activeLocations.push_back(initialOrderLocations[1]);
					activeLocations.push_back(initialOrderLocations[2]);
					activeLocations.push_back(initialOrderLocations[3]);
					activeLocations.push_back(initialOrderLocations[4]);

					std::vector<int64_t> currentDistances = distanceMatrix[pickerList[currentPicker].location];

					return State(grid_size, orderList, pickerList, decisionsRequired, currentPicker, activeLocations, currentDistances, DynaPlex::StateCategory::AwaitAction());
				}
				//grid_size == 20
				else {
					initialOrders = 14;
					PickerLocations = { 184, 56, 98, 374, 235, 101, 61, 77, 11, 313,
									   315, 311, 172, 304, 31, 162, 137, 343, 159, 368,
									   345, 143, 173, 357 };
					initialOrderLocations = { 49, 349, 199, 239, 147, 174, 150, 227, 278, 392, 53, 353, 361, 139, 122 };

					std::vector<State::Picker> pickerList{ };
					std::vector<State::Order> orderList{ };
					std::vector<int64_t> decisionsRequired{ };

					for (size_t i = 0; i < n_pickers; i++) {
						State::Picker newPicker = State::Picker(PickerLocations[i]);
						pickerList.push_back(newPicker);

						//initially every pacman requires a new action
						decisionsRequired.push_back(i);

						//Add pacman locations to active ones
						activeLocations.push_back(PickerLocations[i]);
					}

					std::vector<int64_t> initialOrderDuration = { 9, 10, 9,	3, 9, 3, 6, 8, 3, 7, 8, 5, 9, 5, 8 };

					for (size_t i = 0; i < initialOrderDuration.size(); i++) {
						orderList.push_back(State::Order(initialOrderLocations[i], 1, { 0, initialOrderDuration[i], 0 }));
						activeLocations.push_back(initialOrderLocations[i]);
					}

					std::vector<int64_t> currentDistances = distanceMatrix[pickerList[currentPicker].location];

					return State(grid_size, orderList, pickerList, decisionsRequired, currentPicker, activeLocations, currentDistances, DynaPlex::StateCategory::AwaitAction());
				}
			}
			else {
				if (!fixed_initial_state)
					srand((unsigned int)time(0));
				else
					srand(221122);

				for (size_t i = 0; i < n_pickers; i++)//Generate random pacman initial locations
				{
					size_t randLoc{ 0 };
					//do
					//{//do not initialise a Pacman on an obstacle
					//	randLoc = rand() % (grid_size * grid_size);
					//} while (std::find(obstacleLocations.begin(), obstacleLocations.end(), randLoc) != obstacleLocations.end());
					randLoc = rand() % (grid_size * grid_size);
					PickerLocations.push_back(randLoc);
				}

				initialOrders = (rand() % grid_size); //max GridSize number of initial cherries
				for (size_t i = 0; i < initialOrders; i++) //random number of initial cherries
				{
					size_t randLoc{ 0 };
					do
					{//do not initialise a cherry on an obstacle or on other cherries
						randLoc = rand() % (grid_size * grid_size);
					} while (std::find(initialOrderLocations.begin(), initialOrderLocations.end(), randLoc) != initialOrderLocations.end());
					//} while (std::find(obstacleLocations.begin(), obstacleLocations.end(), randLoc) != obstacleLocations.end() ||
					//	std::find(initialOrderLocations.begin(), initialOrderLocations.end(), randLoc) != initialOrderLocations.end());
					initialOrderLocations.push_back(randLoc);
				}
			}

			if (PickerLocations.size() != n_pickers)
			{
				throw "The number of pickers is unequal to the number of initial picker locations!";
			}

			std::vector<State::Picker> pickerList{ };
			std::vector<State::Order> orderList{ };
			std::vector<int64_t> decisionsRequired{ };

			for (size_t i = 0; i < n_pickers; i++) {
				State::Picker newPacman = State::Picker(PickerLocations[i]);
				pickerList.push_back(newPacman);

				//initially every pacman requires a new action
				decisionsRequired.push_back(i);

				//Add pacman locations to active ones
				activeLocations.push_back(PickerLocations[i]);
			}

			for (size_t i = 0; i < initialOrders; i++) {

				int64_t t0 = 0;
				int64_t t1 = 0;
				int64_t t2 = 0;

				int64_t cherryState = rand() % (2);

				switch (cherryState) {
				case 0:
					t0 = (rand() % (min_time_window + max_time_window)) + 1;
					t1 = min_time_window + rand() % (max_time_window + 1);
					break;
				case 1:
					t1 = rand() % (min_time_window + max_time_window + 1);
					break;
				default:
					break;
				}
				State::Order newOrder = State::Order(initialOrderLocations[i], cherryState, { t0, t1, t2 });

				orderList.push_back(newOrder);

				//Add cherry locations to active ones
				activeLocations.push_back(initialOrderLocations[i]);
			}

			std::vector<int64_t> currentDistances = distanceMatrix[pickerList[currentPicker].location];

			return State(grid_size, orderList, pickerList, decisionsRequired, currentPicker, activeLocations, currentDistances, DynaPlex::StateCategory::AwaitAction());
		}

		MDP::MDP(const VarGroup& config)
		{
			//In principle, state variables should be initiated as follows:
			//config.Get("name_of_variable",name_of_variable);

			config.Get("grid_size", grid_size);
			n_valid_actions = grid_size * grid_size;
			config.Get("n_pickers", n_pickers);
			config.Get("max_orders_per_event", max_orders_per_event);
			config.GetOrDefault("holding_cost", holding_cost, 1.0);
			config.GetOrDefault("move_cost", move_cost, 0.0);
			config.GetOrDefault("tardiness_cost", tardiness_cost, 10.0);
			config.GetOrDefault("order_probability", order_probability, 0.8);
			config.GetOrDefault("min_time_window", min_time_window, 5);
			config.GetOrDefault("max_time_window", max_time_window, 5);
			config.GetOrDefault("max_confirmed", max_confirmed, 100);
			config.GetOrDefault("fixed_initial_state", fixed_initial_state, false);
			config.GetOrDefault("manual_initial_state", manual_initial_state, false);
			
			//we may also have config arguments that are not mandatory, and the internal value takes on 
			// a default value if not provided. Use sparingly. 
			if (config.HasKey("discount_factor"))
				config.Get("discount_factor", discount_factor);
			else
				discount_factor = 1.0;
		
			for (size_t i = 0; i < grid_size * grid_size; i++)
			{
				posProbs.push_back(order_probability / ((double)grid_size * grid_size));
			}

			//chance of no cherry
			posProbs.push_back(1 - order_probability);
			posDist = DiscreteDist::GetCustomDist(posProbs);

			twDist = std::vector<DynaPlex::DiscreteDist>();
			for (size_t i = 0; i < 2; i++) {
				twProbs.push_back({});
				for (size_t j = 0; j <= max_time_window; j++) {
					twProbs[i].push_back(static_cast <double>(1) / (max_time_window + 1));
				}
				twDist.push_back(DiscreteDist::GetCustomDist(twProbs[i]));
			}

			for (size_t i = 0; i < max_confirmed; i++) {
				probsConfirmed.push_back(static_cast<double>(1) / (max_confirmed));
			}
			confirmedDist = DiscreteDist::GetCustomDist(probsConfirmed);

			//Create adjacencyMatrix for grid_graph of size GridSize
			distanceMatrix = std::vector<std::vector<int64_t>>(grid_size * grid_size, std::vector<int64_t>(grid_size * grid_size));

			for (size_t i = 0; i < grid_size * grid_size; i++) {
				for (size_t j = 0; j < grid_size * grid_size; j++) {
					if (i == j)
						distanceMatrix[i][j] = 1.0;
					else
						distanceMatrix[i][j] = (int64_t)State::Distance(i, j, grid_size);
				}
			}
			
			//std::cout << "Number of edges: " << (grid_size * grid_size) * (grid_size * grid_size) << std::endl;
		}

		//(dynaplex standard) evolutions in the environment are all stored into this function: if multiple random events must occurr in a single step, use rng.GenEvent multiple times
		MDP::Event MDP::GetEvent(RNG& rng) const
		{
			std::vector<int64_t> locations{};
			std::vector<std::vector<int64_t>> time_windows{};

			for (int i = 0; i < max_orders_per_event; i++) {
				size_t location = posDist.GetSample(rng);
				locations.push_back(posDist.GetSample(rng));
				time_windows.push_back({});
				for (size_t j = 0; j < 2; j++) {
					time_windows[i].push_back(min_time_window + twDist[i].GetSample(rng));
				}
			}

			int vec_size = grid_size * grid_size;

			//generate all transitions for orders in confirmed state
			std::vector<int64_t> confirmedTransitions(vec_size, 0);
			for (int i = 0; i < size(confirmedTransitions); i++) {
				confirmedTransitions.at(i) = confirmedDist.GetSample(rng);
			}

			Event instance(locations, time_windows, confirmedTransitions);
			return instance;
		}

		std::vector<std::tuple<MDP::Event, double>> MDP::EventProbabilities() const {
			throw DynaPlex::NotImplementedError("Left unimplemented :order_picking cannot be solved using exact methods anyhow");
		}



		void MDP::GetFeatures(const State& state, DynaPlex::Features& features)const {

			//action mask (for inference)
			for (size_t i = 0; i < n_valid_actions; i++) {
				if (IsAllowedAction(state, i))
					features.Add(1);
				else
					features.Add(0);
			}

			state.flattenState(features);
		}
		
		DynaPlex::StateCategory MDP::GetStateCategory(const State& state) const
		{
			//this typically works, but state.cat must be kept up-to-date when modifying states. 
			return state.cat;
		}

		void MDP::RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>& registry) const
		{//Here, we register any custom heuristics we want to provide for this MDP.	
		 //On the generic DynaPlex::MDP constructed from this, these heuristics can be obtained
		 //in generic form using mdp->GetPolicy(VarGroup vars), with the id in var set
		 //to the corresponding id given below.
			registry.Register<GreedyHeuristic>("greedy_heuristic",
				"Order picking allocation heuristic, based on a greedy approach considering:"
				"- multiple pickers in the system"
				"- different state of orders"
				"- possibly, costs (short) lookahead");
		}

		void Register(DynaPlex::Registry& registry)
		{
			DynaPlex::Erasure::MDPRegistrar<MDP>::RegisterModel("order_picking", "nonempty description", registry);			
		}
	}
}

