#include "state.h"
#include <iostream>

namespace DynaPlex::Models 
{
	namespace order_picking
	{
		//using Picker = order_picking::State::Picker;
		//using Order = order_picking::State::Order;

		//fill a Coordinate object with the row and column corresponding to a given place on the grid (normally places on the grid are enumerated)
		void State::GraphElementToCoordinate(Coordinate& coord, const int64_t element, const int64_t GridSize)
		{
			coord.row = element / GridSize;	//should always floor because it is int
			coord.column = element - (coord.row * GridSize);
		}

		//return the place on the grid based on the corresponding coordinate
		int64_t State::CoordinateToGraphElement(const Coordinate Coord, const int64_t GridSize)
		{
			return Coord.column + (Coord.row * GridSize);
		}


		//return the place on the grid based on the corresponding coordinate
		int64_t State::CoordinateToGraphElement(const int64_t row, const int64_t column, const int64_t GridSize)
		{
			return column + (row * GridSize);
		}

		//compute distance between two given cells on the grid
		int64_t State::Distance(const int64_t startPoint, const int64_t endPoint, const int64_t GridSize)
		{
			Coordinate startCoordinate{};
			GraphElementToCoordinate(startCoordinate, startPoint, GridSize);
			Coordinate endCoordinate{};
			GraphElementToCoordinate(endCoordinate, endPoint, GridSize);

			int64_t horizontalDistance = abs((int64_t)startCoordinate.row - (int64_t)endCoordinate.row);
			int64_t verticalDistance = abs((int64_t)startCoordinate.column - (int64_t)endCoordinate.column);

			return horizontalDistance + verticalDistance;
		}

		//compute distance between two given points on the grid
		int64_t State::Distance(const Coordinate startCoordinate, const Coordinate endCoordinate)
		{
			int64_t horizontalDistance = abs((int64_t)startCoordinate.row - (int64_t)endCoordinate.row);
			int64_t verticalDistance = abs((int64_t)startCoordinate.column - (int64_t)endCoordinate.column);

			return horizontalDistance + verticalDistance;
		}

		void  State::flattenState(Features& out_feat_array) const {

			// Defaults to zero initial value
			std::vector<float> currentPickerMatrix(
				GridSize * GridSize);

			std::vector<float> otherUnallocatedPickerMatrix(
				GridSize * GridSize);

			std::vector<float> otherAllocatedPickerMatrix(
				GridSize * GridSize);

			std::vector<float> pickerDestinationMatrix(
				GridSize * GridSize);

			std::vector<float> confirmedOrderMatrix(
				GridSize * GridSize);

			std::vector<float> orderMatrix(
				GridSize * GridSize);

			std::vector<float> tardyOrderMatrix(
				GridSize * GridSize);

			int64_t maxDistance = 2 * GridSize - 1;

			//fill both picker matrices and destination matrix
			int curr_picker = 0;
			for (const auto& picker : pickerList) {

				auto pickerLocation = picker.location;
				auto pickerDestination = picker.destination;

				if (curr_picker == currentPicker)
					currentPickerMatrix[pickerLocation] = 1.0;
				else
				{
					if (picker.allocated) {
						otherAllocatedPickerMatrix[pickerLocation] += 1.0;

						int distance = Distance(pickerLocation, pickerDestination, GridSize);
						pickerDestinationMatrix[pickerDestination] = (distance > 0) ? distance : 1.0;
					}
					else
						otherUnallocatedPickerMatrix[pickerLocation] += 1.0;
				}
				curr_picker++;
			}

			//fill order matrix(es)
			for (const auto& order : orderList) {
				auto orderLocation = order.location;
				Coordinate orderCoord{};
				GraphElementToCoordinate(orderCoord, orderLocation, GridSize);
				if (order.state == 0) { //confirmed order
					confirmedOrderMatrix[orderCoord.row * GridSize + orderCoord.column] = order.time_windows[0];
				}
				else if (order.state == 1) { //ongoing order
					orderMatrix[orderCoord.row * GridSize + orderCoord.column] = order.time_windows[1];
				}
				else if (order.state == 2) { //tardy order
					tardyOrderMatrix[orderCoord.row * GridSize + orderCoord.column] = order.time_windows[2] + 1;
				}
			}

			size_t currPickerLocation = pickerList[currentPicker].location;

			for (size_t i = 0; i < (GridSize * GridSize); i++) {
				out_feat_array.Add(currentPickerMatrix[i]);
				out_feat_array.Add(otherAllocatedPickerMatrix[i]);
				out_feat_array.Add(otherUnallocatedPickerMatrix[i]);
				out_feat_array.Add(pickerDestinationMatrix[i]);
				out_feat_array.Add(confirmedOrderMatrix[i]);
				out_feat_array.Add(orderMatrix[i]);
				out_feat_array.Add(tardyOrderMatrix[i]);
				out_feat_array.Add(currentDistances[i]);
			}
			return;
		}

		void State::changeActiveLocations() {

			activeLocations.clear();

			//Add picker location to actives if not already present (e.g. a picker is on it or it is a destination)
			for (const auto& picker : pickerList) {
				if (std::find(activeLocations.begin(), activeLocations.end(), picker.location) == activeLocations.end())
					activeLocations.push_back(picker.location);
			}

			//Add order location to actives if not already present (e.g. a picker is on it or it is a destination)
			for (const auto& order : orderList) {
				if (std::find(activeLocations.begin(), activeLocations.end(), order.location) == activeLocations.end())
					if (order.state < 3)
						activeLocations.push_back(order.location);
			}
		}
	}
}