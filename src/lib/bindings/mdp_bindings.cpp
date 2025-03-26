#include "dynaplex/mdp.h"
#include "vargroupcaster.h"
#include <pybind11/stl.h> 
#include <pybind11/pybind11.h>

void define_mdp_bindings(pybind11::module_& m) {
	pybind11::class_<DynaPlex::MDPInterface, DynaPlex::MDP>(m, "MDP")
		.def("run_until_period_count", [](DynaPlex::MDPInterface& self, DynaPlex::Policy& policy, DynaPlex::Trajectory& trajectory, int64_t max_period_count)
		{
			while (self.IncorporateUntilAction({ &trajectory,1 }, max_period_count))
			{
				self.IncorporateAction({ &trajectory,1 }, policy);
			}
		}, pybind11::arg("policy"), pybind11::arg("trajectory"), pybind11::arg("max_period_count"), R"docstring(
	The trajectory (which has a current period_count and a cumulative_return) is continued until period_count equals max_period_count, while updating cumulative_return 
	)docstring")
		.def("get_state_category", [](DynaPlex::MDPInterface& self, DynaPlex::Trajectory& traj) {
			return *(traj.Category.ToVarGroup().ToPybind11Dict());
		}, pybind11::arg("traj"), R"docstring(
	Returns state category for a particular state.
	)docstring")
	.def("allowed_actions", [](DynaPlex::MDPInterface& self, DynaPlex::Trajectory& trajectory) {
		if (!trajectory.HasState())
			throw DynaPlex::Error("mdp.allowed_actions(trajectory) - trajectory does not contain state, did you initialize it?");
		auto actions = self.AllowedActions(trajectory.GetState());
		pybind11::list list;
		for (auto action : actions)
			list.append(action);
		return list;
			}, pybind11::arg("trajectory"), R"docstring(
	Returns list of allowed actions for the current state of trajectory. 
	)docstring")
	.def("incorporate_action", [](DynaPlex::MDPInterface& self, DynaPlex::Trajectory& trajectory) {
				return self.IncorporateAction({ &trajectory,1 });
	}, pybind11::arg("trajectory"), R"docstring(
	Incorporates the NextAction into the trajectories. Trajectory
	must have Category.IsAwaitsAction; throws DynaPlex::Exception otherwise. 
	Note: Assumes trajectory.NextAction is allowed for corresponding trajectory.GetState(). 	
	)docstring")
	.def("incorporate_action", [](DynaPlex::MDPInterface& self, DynaPlex::Trajectory& trajectory, DynaPlex::Policy& policy) {
		return self.IncorporateAction({ &trajectory,1 }, policy);
	}, pybind11::arg("trajectory"), pybind11::arg("policy"), R"docstring(
	Incorporates the action selected by the policy into the trajectory. Trajectory in span/vector
	must have Category.IsAwaitsAction; throws DynaPlex::Exception otherwise.
	Note: Immediately after the call, NextAction for the trajectory still contains the action taken. 
	)docstring")
	.def("incorporate_until_action", [](DynaPlex::MDPInterface& self, DynaPlex::Trajectory& trajectory, int64_t MaxPeriodCount) {
		return self.IncorporateUntilAction({ &trajectory,1 }, MaxPeriodCount);
	}, pybind11::arg("trajectory"), pybind11::arg("max_period_count") = std::numeric_limits<int64_t>::max(), R"docstring(
	Incorporates events in the provided trajectory until at least one of the following holds:
	1. Category IsFinal()
	2. trajectory.PeriodCount>=MaxPeriodCount, and Category.IsAwaitEvent();
	3. Category.IsAwaitAction()
	returns true if trajectory is in category 3, false otherwise.
	)docstring")
	.def("incorporate_until_nontrivial_action", [](DynaPlex::MDPInterface& self, DynaPlex::Trajectory& trajectory, int64_t MaxPeriodCount) {
		return self.IncorporateUntilNonTrivialAction({ &trajectory,1 });
	}, pybind11::arg("trajectory"), pybind11::arg("max_period_count") = std::numeric_limits<int64_t>::max(), R"docstring(
	Incorporates events in the provided trajectory until at least one of the following holds:
	1. Category IsFinal()
	2. trajectory.PeriodCount>=MaxPeriodCount, and Category.IsAwaitEvent();
	3. Category.IsAwaitAction(), and there is more than a single action allowed
	returns true if trajectory is in category 3, false otherwise.
	)docstring")
	.def("deep_copy", [](DynaPlex::MDPInterface& self, DynaPlex::Trajectory& trajectory) {
		return self.DeepCopy(trajectory);
	}, pybind11::arg("trajectory"), R"docstring(
	Returns a deep copy of the trajectory, copying all trajectory variables and cloning the underlying state.
	)docstring")
	.def("deep_copy_and_reinitiate", [](DynaPlex::MDPInterface& self, DynaPlex::Trajectory& trajectory) {
		return self.DeepCopyAndReinitiate(trajectory);
	}, pybind11::arg("trajectory"), R"docstring(
	Returns a deep copy of the trajectory, copying all trajectory variables and cloning the underlying state, while reinitiating any hidden state variables using the initiation RNG.
	)docstring")
	.def("initiate_state", [](DynaPlex::MDPInterface& self, DynaPlex::Trajectory& trajectory) {
		self.InitiateState({ &trajectory,1 });
	}, pybind11::arg("trajectory"), R"docstring(
	Initiates the state in the trajectory. Uses random initial state (GetInitialState(RNG&)) if available, otherwise uses deterministic state (GetInitialState()). 
	Updates the Category in the trajectory to reflect the initial state, and re-initiates PeriodCount, CumulativeReturn, and EffectiveDiscountFactor. 
	)docstring")
	.def("initiate_state", [](DynaPlex::MDPInterface& self, DynaPlex::Trajectory& trajectory, DynaPlex::Trajectory& initial_state_trajectory) {
		if (!initial_state_trajectory.HasState())
			throw DynaPlex::Error("mdp.initiate_state(trajectory,initial_state_provider) - initial_state_provider does not contain state, did you initialize it?");
		self.InitiateState({ &trajectory,1 }, initial_state_trajectory.GetState());
	}, pybind11::arg("trajectory"), pybind11::arg("initial_state_trajectory"), R"docstring(
	Sets the state in the trajectory to a specific state value deep-copied from the initial_state_trajectory, while reinitiating any hidden state variables. 
	Updates the Category in the trajectory, and re-initiates PeriodCount, CumulativeReturn, and EffectiveDiscountFactor.
	)docstring")
	.def("initiate_state", [](DynaPlex::MDPInterface& self, DynaPlex::Trajectory& trajectory, pybind11::dict& dict) {
	auto state = self.GetState(DynaPlex::VarGroup(dict));
	self.InitiateState({ &trajectory,1 }, std::move(state));
		}, pybind11::arg("trajectory"), pybind11::arg("state_as_dict"), R"docstring(
	Sets the state in the trajectory to a specific state value converted from state_as_dict, while reinitiating any hidden state variables. 
	Updates the Category in the trajectory, and re-initiates PeriodCount, CumulativeReturn, and EffectiveDiscountFactor.
	)docstring")
	.def("num_valid_actions", &DynaPlex::MDPInterface::NumValidActions)
	.def("provides_flat_features", &DynaPlex::MDPInterface::ProvidesFlatFeatures)
	.def("num_flat_features", &DynaPlex::MDPInterface::NumFlatFeatures)
	.def("identifier", &DynaPlex::MDPInterface::Identifier)
	.def("type_identifier", &DynaPlex::MDPInterface::TypeIdentifier)
	.def("get_static_info", [](DynaPlex::MDPInterface& mdp) {
	return *(mdp.GetStaticInfo().ToPybind11Dict());
	}, "Gets dictionary representing static information for this MDP, i.e. MDP properties.")
	.def("discount_factor", &DynaPlex::MDPInterface::DiscountFactor)
	.def("is_infinite_horizon", &DynaPlex::MDPInterface::IsInfiniteHorizon,
	"indicates whether the MDP is infinite or finite horizon")
	.def("list_policies",
	[](DynaPlex::MDPInterface& mdp) {
		return *(mdp.ListPolicies().ToPybind11Dict());
	}, "Lists key-value pairs (id,description) that represent the available build-in policies for this MDP.")
	.def("get_policy",
	[](DynaPlex::MDPInterface& mdp, py::kwargs& kwargs) {
	// Convert kwargs to DynaPlex::VarGroup
	auto vars = DynaPlex::VarGroup(kwargs);
	return mdp.GetPolicy(vars);
	},
	"Get a policy; supports keyword arguments.")
	.def("get_policy",
	pybind11::overload_cast<const std::string&>(&DynaPlex::MDPInterface::GetPolicy, py::const_),
	pybind11::arg("id"),
	"Convenience function that calls GetPolicy with the parameter id.");

}
