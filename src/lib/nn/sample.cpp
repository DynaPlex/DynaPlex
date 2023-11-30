#include "dynaplex/sample.h"
#include "dynaplex/error.h"
#include <cmath> 

namespace DynaPlex::NN {

	Sample::Sample(int64_t action_label, DynaPlex::dp_State state)
		: action_label(action_label), state(std::move(state))
	{
	}
	DynaPlex::VarGroup Sample::ToVarGroup() const
	{
		//note: loading logic is in sampledata.cpp
		DynaPlex::VarGroup vars;
		vars.Add("action_label", action_label);
		vars.Add("sample_number", sample_number);
		vars.Add("state", state->ToVarGroup());
		for (const auto& value : q_hat_vec) {
			if (std::isnan(value) || std::isinf(value)) {
				DynaPlex::Error("Sample: Value error, q-values contain NaN or infinity.");
			}
		}
		vars.Add("q_hat_vec", q_hat_vec);
		if (std::isnan(q_hat) || std::isinf(q_hat)) {
			throw DynaPlex::Error("Sample: Value error, best q-value must be a real number.");
		}
		vars.Add("q_hat", q_hat);
		if (std::isnan(z_stat) || std::isinf(z_stat)) {
			throw DynaPlex::Error("Sample: Value error, z-statistic must be a real number.");
		}
		vars.Add("z_stat", z_stat);
		for (const auto& value : cost_improvement) {
			if (std::isnan(value) || std::isinf(value)) {
				DynaPlex::Error("Sample: Value error, relative costs contain NaN or infinity.");
			}
		}
		vars.Add("cost_improvement", cost_improvement);
		double sum = 0.0;
		for (const auto& value : probabilities) {
			if (std::isnan(value) || std::isinf(value)) {
				DynaPlex::Error("Sample: Value error, probabilities contain NaN or infinity.");
			}
			sum += value;
		}
		const double tolerance = 1e-6; // Define a small tolerance level
		if (std::abs(sum - 1.0) > tolerance) {
			DynaPlex::Error("Sample: Value error, the sum of probabilities is not 1.0.");
		}
		vars.Add("probabilities", probabilities);
		
		return vars;		
	}
	DynaPlex::VarGroup Sample::ToVarGroupWithFeats(DynaPlex::MDP mdp) const {
		DynaPlex::VarGroup vars;
		if (!mdp->CheckConformant(state))
			throw DynaPlex::Error("Sample::ToVarGroupWithFeats - state nonconformant with mdp.");
		
		vars.Add("action_label", action_label);
		auto allowedActions = mdp->AllowedActions(state);
		
		//vars.Add("allowed_actions", allowedActions);

		std::vector<int64_t> actionMask(mdp->NumValidActions(), 0);
		for (auto action : allowedActions) {
			actionMask[action] = 1;
		}
		vars.Add("allowed_actions", actionMask);

		if (!mdp->ProvidesFlatFeatures()) {
			throw DynaPlex::Error("Sample::ToVarGroupWithFeats - mdp does not provide flat features. This is currently unsupported.");
		}
		auto num_feats = mdp->NumFlatFeatures();

		std::vector<float> feats(num_feats);
		mdp->GetFlatFeatures(state,feats);
		std::vector<double> d_feats;
		for (auto f: feats)
		{
			d_feats.push_back(static_cast<double>(f));
		}
		vars.Add("features", d_feats);

		return vars;
	}


}

