#pragma once
#include "dynaplex/dynaplex_model_includes.h"
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/modelling/queue.h"
#include <list>
#include <optional>
#include <algorithm>

namespace DynaPlex::Models {
	namespace collaborative_picking {
		struct Location {
			int64_t row, column;
			int64_t node_id{ -1 };

			void UpdateNodeId(int64_t rows)
			{
				node_id = row * rows + column;
			}

			explicit Location(const DynaPlex::VarGroup& vars) {
				std::vector<int64_t> coords;
				vars.Get("coords", coords);
				if (coords.size() != 2)
					throw DynaPlex::Error("PickLocation: Should have exactly two coordinates.");
				row = coords[0];
				column = coords[1];
			}

			Location() {};
		};

		struct TruckDock : public Location {
			int64_t corresponding_dispatch_node_id;
			TruckDock() {};
			explicit TruckDock(const DynaPlex::VarGroup& vars) : Location(vars) {
			}
		};

		struct PickLocation : public Location {
			double prob;
			std::vector<int64_t> vehicle_nodes{};
			std::vector<int64_t> vehicle_loc{};

			PickLocation() {};
			explicit PickLocation(const DynaPlex::VarGroup& vars) : Location(vars) {
				vars.Get("prob", prob);
				vars.Get("vehicle_loc", vehicle_loc);
			}
		};

		struct PickerArea {
			std::vector<int64_t> nodes;

			DynaPlex::VarGroup ToVarGroup() const {
				return DynaPlex::VarGroup{
					{"nodes",nodes}
				};
			}

		};

	}
}