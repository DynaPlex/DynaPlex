#pragma once
#include <vector>
#include <string>
#include <cstddef>
#include <random>
#include "dynaplex/dynaplex_model_includes.h"

namespace DynaPlex::Models
{
	namespace order_picking
	{
		class State //representation of a state in the corresponding MDP
		{

		public:


			class Order {
			public:
				int64_t location{ };
				int64_t state{ }; //if order states are used, an order can be in three states: 0==confirmed, 1==ongoing, 2==tardy
				std::vector<int64_t> time_windows{ }; //one time window for each state:
				//0: how long before the order becomes available
				//1: expected mac time of pickup for ongoing orders
				//2: tardiness of the order

				int64_t assigned{ }; //index of the assigned picker, -1 if not assigned

				bool tardy{ };
				bool canceled{ };

				DynaPlex::VarGroup ToVarGroup() const
				{
					DynaPlex::VarGroup vars;

					vars.Add("location", location);
					vars.Add("state", state);
					vars.Add("time_windows", time_windows);
					vars.Add("assigned", assigned);
					vars.Add("tardy", tardy);
					vars.Add("canceled", canceled);

					return vars;
				}

				bool operator==(const Order& other) const = default;

				Order(int64_t location, int64_t state, std::vector<int64_t> time_windows) :
					location{ location }, state{ state }, time_windows{ time_windows }, assigned{ -1 }, tardy{ false }
				{ };

				explicit Order(const VarGroup& vars)
				{
					vars.Get("location", location);
					vars.Get("state", state);
					vars.Get("time_windows", time_windows);
					vars.Get("assigned", assigned);
					vars.Get("tardy", tardy);
					vars.Get("canceled", canceled);
				}

				Order() {};

				void evolve(int64_t confirmedTransition) {

					if (state == 0) {

						//always decrease time_window
						time_windows[0]--;

						//the order is becoming active!
						if (time_windows[0] == 0) {
							state = 1;
						}
						else {
							size_t transition = confirmedTransition;
							//at each step, orders in the confirmed state have 98% chance of staying where they are and 2% chance of being canceled
							if (transition >= 100) {
								canceled = true;
							}
						}
					}
					else if (state == 1) {
						time_windows[1]--;
						if (time_windows[1] == 0) {
							tardy = true;
							state = 2;
						}
					}
					else if (state == 2) {
						time_windows[2]++;
					}
					else if (state == 3) {
						time_windows[1]--;
						if (time_windows[1] == 0) {
							canceled = true;
						}
					}
				}
			};

			class Picker
			{
			public:
				int64_t location{ };
				int64_t destination{ };

				bool allocated{ };

				bool operator==(const Picker& other) const = default;

				DynaPlex::VarGroup ToVarGroup() const
				{
					DynaPlex::VarGroup vars;

					vars.Add("location", location);
					vars.Add("destination", destination);
					vars.Add("allocated", allocated);

					return vars;
				}

				Picker(int64_t location) :
					location{ location }, destination{ location }, allocated{ false }
				{ }

				explicit Picker(const VarGroup& vars)
				{
					vars.Get("location", location);
					vars.Get("destination", destination);
					vars.Get("allocated", allocated);

				};

				Picker() {};
			};

			//The order_picking MDP is a problem where one or more pickers walk on a grid, making Manhattan-style movements
			//Every round, all Pickers make a move decision, after which an event happens - the possible addition of an order on the grid
			//The Picker's objective is to pick orders to minimize holding costs paid for every order on the grid
			DynaPlex::StateCategory cat;
			//enum class Status : int { AwaitEvent = 0, AwaitAction = 1 };

			int64_t GridSize{ };
			std::vector<Order> orderList{ };
			std::vector<Picker> pickerList{ };
			std::vector<int64_t> decisionsRequired{ };
			std::vector<int64_t> assignedOrders{ };
			std::vector<int64_t> activeLocations{ };
			std::vector<int64_t> currentDistances{ };
			int64_t currentPicker{ };

			struct Coordinate
			{
				size_t row{ 0 };
				size_t column{ 0 };
			};

			static void GraphElementToCoordinate(struct Coordinate& coord, const int64_t i, const int64_t GridSize);
			static int64_t CoordinateToGraphElement(const Coordinate Coord, const int64_t GridSize);
			static int64_t CoordinateToGraphElement(const int64_t row, const int64_t column, const int64_t GridSize);
			static int64_t Distance(const int64_t startPoint, const int64_t endPoint, const int64_t GridSize);
			static int64_t Distance(const Coordinate startCoordinate, const Coordinate endCoordinate);

			//Helper function to get the matrices in form of array
			void flattenState(Features& out_feat_array) const;

			void changeActiveLocations();

			static constexpr bool CanSerialize = true;

			//declaration; for definition see mdp.cpp:
			DynaPlex::VarGroup ToVarGroup() const;

			bool operator==(const State& other) const = default;

			State(int64_t GridSize, std::vector<Order> orderListP, std::vector<Picker> pickerListP,
				std::vector<int64_t> decisionsRequiredP, int64_t currentPickerP, std::vector<int64_t> activeLocations,
				std::vector<int64_t> currentDistancesP, DynaPlex::StateCategory catP) :
				GridSize{ GridSize }, orderList{ orderListP }, pickerList{ pickerListP },
				decisionsRequired{ decisionsRequiredP }, currentPicker{ currentPickerP }, activeLocations{ activeLocations },
				currentDistances{ currentDistancesP }, cat{catP}
			{
			};

			explicit State(const VarGroup& vars) {
				vars.Get("cat", cat);
				//initiate any other state variables. 
				vars.Get("grid_size", GridSize);
				vars.Get("picker_list", pickerList);
				vars.Get("order_list", orderList);
				vars.Get("decisions_required", decisionsRequired);
				vars.Get("assigned_orders", assignedOrders);
				vars.Get("active_locations", activeLocations);
				vars.Get("current_distances", currentDistances);
				vars.Get("current_picker", currentPicker);
			};

			State() {};
		};
	}
}