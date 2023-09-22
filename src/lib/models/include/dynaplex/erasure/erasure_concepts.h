#pragma once
#include <type_traits>
#include "dynaplex/vargroup.h"
#include "dynaplex/statecategory.h"
namespace DynaPlex::Erasure
{

	template <typename t_MDP,typename t_State>
	concept HasGetStateFromVars = requires(const t_MDP & t, const VarGroup & vars) {
		{ t.GetState(vars) } -> std::same_as<t_State>;
	};

	template <typename t_MDP, typename t_State, typename t_Event>
	concept HasModifyStateWithEvent = requires(const t_MDP & mdp, t_State & state, const t_Event & event) {
		{ mdp.ModifyStateWithEvent(state, event) } -> std::same_as<double>;
	};

	template <typename t_MDP, typename t_Event, typename t_RNG>
	concept HasGetEvent = requires(const t_MDP & mdp, t_RNG & rng) {
		{ mdp.GetEvent(rng) } -> std::same_as<t_Event>;
	};

	template <typename t_MDP,typename t_State, typename t_Event, typename t_RNG>
	concept HasGetStateDependentEvent = requires(const t_MDP & mdp, const t_State& state, t_RNG & rng) {
		{ mdp.GetEvent(state, rng) } -> std::same_as<t_Event>;
	};

	template<typename t_Policy, typename t_State>
	concept HasGetAction = requires(const t_Policy mdp, const t_State & state) {
		{ mdp.GetAction(state) } -> std::same_as<int64_t>;
	};

	template<typename t_Policy, typename t_State, typename t_MDP>
	concept HasGetActionRNG = requires(const t_Policy mdp, const t_State & state, t_MDP & rng) {
		{ mdp.GetAction(state, rng) } -> std::same_as<int64_t>;
	};

	template <typename t_MDP>
	concept HasIsAllowedAction = requires(const t_MDP mdp, const typename t_MDP::State state, int64_t action)
	{
		{ mdp.IsAllowedAction(state, action) } -> std::same_as<bool>;
	};

	template<typename t_MDP>
	concept HasGetStateCategory = requires(t_MDP a, const typename t_MDP::State & s) {
		{ a.GetStateCategory(s) } -> std::same_as<StateCategory>;
	};

	template<typename t_MDP>
	concept HasEvent = requires{
		typename t_MDP::Event;
	};


	template<typename t_MDP>
	concept HasState = requires{
		typename t_MDP::State;
	};

	template <typename t_MDP,typename t_Registry>
	concept HasRegisterPolicies = requires(t_MDP t, t_Registry& registry) {
		{ t.RegisterPolicies(registry) } -> std::same_as<void>;
	};

	template<typename t_MDP>
	concept HasStateConvertibleToVarGroup = requires{
		HasState<t_MDP>;
		{ DynaPlex::Concepts::ConvertibleToVarGroup<typename t_MDP::State> };
	};

	template<typename t_MDP>
	concept HasGetStaticInfo = requires(const t_MDP & mdp) {
		{ mdp.GetStaticInfo() } -> std::same_as<DynaPlex::VarGroup>;
	};

	template<typename t_MDP>
	concept HasModifyStateWithAction = requires(const t_MDP & mdp, typename t_MDP::State & state, int64_t action) {
		{ mdp.ModifyStateWithAction(state, action) };
	};

	template <typename t_MDP>
	concept HasGetInitialState = requires(const t_MDP & mdp)
	{
		{ mdp.GetInitialState() } -> std::same_as<typename t_MDP::State>;
	};

	
	template <typename t_MDP, typename t_RNG>
	concept HasGetInitialRandomState = requires(const t_MDP & mdp, t_RNG & rng)
	{
		{ mdp.GetInitialState(rng) } -> std::same_as<typename t_MDP::State>;
	};
		


}