#pragma once
#include "dynaplex/mdp.h"
#include "dynaplex/vargroup.h"
#include "dynaplex/trajectory.h"
#include "dynaplex/system.h"
#include <pybind11/pybind11.h>
#include <tuple>
#include <vector>
namespace py = pybind11;

namespace DynaPlex {

	class GymEmulator {
	public:
		using obs_type = std::tuple<std::vector<float>, std::vector<int>>;
		GymEmulator(DynaPlex::System system, MDP mdp, VarGroup& vars);

		obs_type Reset(VarGroup& vars);

		std::tuple<obs_type, double, bool, py::dict> Step(int64_t action);

		DynaPlex::VarGroup CurrentStateAsObject() const;

		void Close();

		// Returns the action space size (number of valid actions)
		int64_t ActionSpaceSize() const;

		// Returns the observation space size (state dimensionality)
		int64_t ObservationSpaceSize() const;

		// Destructor
		~GymEmulator();

	private:
		MDP mdp;
		DynaPlex::Trajectory trajectory;
		double last_cumulative_return;
		int64_t actions_taken_since_reset, events_since_reset;
		int64_t num_valid_actions, num_feats;
		bool seeded = false;
		int64_t num_actions_until_done;

		obs_type GetNextObservation() const{
			std::vector<float> feats_vec(num_feats);
			std::vector<int> mask_vec(num_valid_actions, 0);

			mdp->GetFlatFeatures(trajectory.GetState(), feats_vec);
			for (auto action: mdp->AllowedActions(trajectory.GetState()))
			{
				mask_vec.at(action) = 1;
			}

			return { feats_vec, mask_vec };
		}
	};

}  // namespace DynaPlex
