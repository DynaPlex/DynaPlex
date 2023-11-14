#include "policies.h"
#include "mdp.h"
#include "dynaplex/error.h"

namespace DynaPlex::Models {
	namespace order_picking /*keep this namespace name in line with the name space in which the mdp corresponding to this policy is defined*/
	{
        using State = MDP::State;

        class Comparer
        {
        private:
            size_t curLoc;
            size_t gridSize;
            bool costBasedFlag;
            std::vector<double> costs;
        public:
            Comparer(size_t curLoc, size_t gridSize, bool costBasedFlag, std::vector<double> costs) :
                curLoc{ curLoc }, gridSize{ gridSize }, costBasedFlag{ costBasedFlag }, costs{ costs }
            {
            }

            bool operator()(State::Order const& l, State::Order const& r)
            {
                if (costBasedFlag) {
                    if (l.state == 2 && r.state == 2)
                        return State::Distance(curLoc, l.location, gridSize) < State::Distance(curLoc, r.location, gridSize);
                    else if (l.state == 2 && r.state == 1) {
                        int dl = State::Distance(curLoc, l.location, gridSize);
                        int dr = State::Distance(curLoc, r.location, gridSize);
                        int dlr = State::Distance(l.location, r.location, gridSize);
                        size_t lCost = costs[2] * dl + costs[1] * std::min((dl + dlr), (int)r.time_windows[1]) + costs[2] * std::max(0, ((dl + dlr) - (int)r.time_windows[1]));
                        size_t rCost = costs[2] * (dr + dlr) + costs[1] * std::min(dr, (int)r.time_windows[1]) + costs[2] * std::max(0, (dr - (int)r.time_windows[1]));
                        return lCost < rCost;
                    }
                    else if (l.state == 1 && r.state == 2) {
                        int dl = State::Distance(curLoc, l.location, gridSize);
                        int dr = State::Distance(curLoc, r.location, gridSize);
                        int dlr = State::Distance(l.location, r.location, gridSize);
                        size_t lCost = costs[2] * (dl + dlr) + costs[1] * std::min(dl, (int)l.time_windows[1]) + costs[2] * std::max(0, (dl - (int)l.time_windows[1]));
                        size_t rCost = costs[2] * dr + costs[1] * std::min((dr + dlr), (int)l.time_windows[1]) + costs[2] * std::max(0, ((dr + dlr) - (int)l.time_windows[1]));
                        return lCost < rCost;
                    }
                    else if (l.state == 1 && r.state == 1) {
                        int dl = State::Distance(curLoc, l.location, gridSize);
                        int dr = State::Distance(curLoc, r.location, gridSize);
                        int dlr = State::Distance(l.location, r.location, gridSize);
                        size_t lCost = costs[1] * std::min(dl, (int)l.time_windows[1]) + costs[2] * std::max(0, (dl - (int)l.time_windows[1])) + costs[1] * std::min((dl + dlr), (int)r.time_windows[1]) + costs[2] * std::max(0, ((dl + dlr) - (int)r.time_windows[1]));
                        size_t rCost = costs[1] * std::min(dr, (int)r.time_windows[1]) + costs[2] * std::max(0, (dr - (int)r.time_windows[1])) + costs[1] * std::min((dr + dlr), (int)l.time_windows[1]) + costs[2] * std::max(0, ((dr + dlr) - (int)l.time_windows[1]));
                        return lCost < rCost;
                    }
                    else {
                        std::string errorMsg = "The state of the considered orders should be in 1 or 2, but " + std::to_string(l.state) + " and " + std::to_string(r.state) + " were found.";
                        throw DynaPlex::Error(errorMsg);
                    }
                }
                else
                    return State::Distance(curLoc, l.location, gridSize) < State::Distance(curLoc, r.location, gridSize);
            }
        };

		//MDP and State refer to the specific ones defined in current namespace
		GreedyHeuristic::GreedyHeuristic(std::shared_ptr<const MDP> mdp, const VarGroup& config)
			:mdp{ mdp }
		{
			config.GetOrDefault("coordinated", coordinated, false);
			config.GetOrDefault("cost_based", costBased, false);

			if (costBased) {
                costs = { 0, (double)mdp->holding_cost, (double)mdp->tardiness_cost };
			}
		}

        int64_t GreedyHeuristic::FindAction(const State& state) const
        {
            //Finds closest order, taking into account other pickers and order states

            size_t curPicker = state.currentPicker;
            size_t curLoc = state.pickerList[curPicker].location;
            size_t gridSize = mdp->grid_size;
            bool costBasedFlag = costBased;
            std::vector<State::Order> rankedOrders{};

            size_t curBestValue = std::numeric_limits<size_t>::max();
            size_t curBestOrder = std::numeric_limits<size_t>::max();
            std::vector<size_t> excludedOrders{};

            //Rank unassigned orders by state and distance, then check if other pickers are closer

            //Find active unassigned orders
            for (auto order : state.orderList)
            {
                if (order.state >= 1 && mdp->IsAllowedAction(state, order.location))
                    rankedOrders.push_back(order);
            }

            //Sort active orders by distance from current picker
            std::sort(rankedOrders.begin(), rankedOrders.end(), Comparer(curLoc, gridSize, costBasedFlag, costs));

            if (coordinated) {
                //Check if there are other unallocated pickers closer to the orders, in distance order
                for (auto order : rankedOrders) {
                    bool closest = true;
                    for (size_t i = 0; i < state.pickerList.size(); i++) {
                        if (i != curPicker && !state.pickerList[i].allocated) {
                            size_t otherLoc = state.pickerList[i].location;
                            if (State::Distance(otherLoc, order.location, gridSize) < State::Distance(curLoc, order.location, gridSize)) {
                                closest = false;
                                break;
                            }
                        }
                    }
                    if (closest)
                        return order.location;
                }
            }
            else {
                if (rankedOrders.size() > 0)
                    return rankedOrders[0].location;
            }

            //If nothing was found, empty ranked order array
            rankedOrders.clear();


            //Find predicted unassigned orders
            for (auto order : state.orderList)
            {
                if (order.state == 0 && mdp->IsAllowedAction(state, order.location))
                    //if (order.state == 0 && order.assigned == -1 && order.time_windows[0] <= State::Distance(curLoc, order.location, gridSize))
                    rankedOrders.push_back(order);
            }

            //Sort confirmed orders by distance from current picker
            std::sort(rankedOrders.begin(), rankedOrders.end(),
                [curLoc, gridSize, costBasedFlag](State::Order const& l, State::Order const& r) {
                    if (costBasedFlag)
                        return State::Distance(curLoc, l.location, gridSize) + l.time_windows[0] < State::Distance(curLoc, r.location, gridSize) + r.time_windows[0];
                    else
                        return State::Distance(curLoc, l.location, gridSize) < State::Distance(curLoc, r.location, gridSize);
                }
            );


            if (coordinated) {
                //Check if there are other unallocated pickers closer to the orders, in distance order
                for (auto order : rankedOrders) {
                    bool closest = true;
                    for (size_t i = 0; i < state.pickerList.size(); i++) {
                        if (i != curPicker && !state.pickerList[i].allocated) {
                            size_t otherLoc = state.pickerList[i].location;
                            if (State::Distance(otherLoc, order.location, gridSize) < State::Distance(curLoc, order.location, gridSize)) {
                                closest = false;
                                break;
                            }
                        }
                    }
                    if (closest)
                        return order.location;
                }
            }
            else {
                if (rankedOrders.size() > 0)
                    return rankedOrders[0].location;
            }
            //If nothing was found, empty ranked order array
            rankedOrders.clear();

            //Stay in place if allowed, else move to one of the four neighbors
            if (mdp->IsAllowedAction(state, curLoc))
                return curLoc;
            else {
                if (mdp->IsAllowedAction(state, curLoc - gridSize))
                    return curLoc - gridSize;
                if (mdp->IsAllowedAction(state, curLoc + gridSize))
                    return curLoc + gridSize;
                if (mdp->IsAllowedAction(state, curLoc - 1))
                    return curLoc - 1;
                if (mdp->IsAllowedAction(state, curLoc + 1))
                    return curLoc + 1;
            }

            throw DynaPlex::Error("No valid action can be selected.");
        }

		int64_t GreedyHeuristic::GetAction(const State& state) const
        {
            size_t action = FindAction(state);

            if (!mdp->IsAllowedAction(state, action))
            {
                throw "Heuristic is making a non-allowed move!";
            }
            return action;
        }
	}
}