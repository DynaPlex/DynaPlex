#pragma once
#include <type_traits>
#include "dynaplex/vargroup.h"
#include "dynaplex/statecategory.h"
#include "dynaplex/rng.h"
namespace DynaPlex::Erasure
{

	template<typename t_Policy, typename t_State>
	concept HasGetAction = requires(const t_Policy mdp, const t_State & state) {
		{ mdp.GetAction(state) } -> std::same_as<int64_t>;
	};

	template<typename t_Policy, typename t_State>
	concept HasGetActionRNG = requires(const t_Policy mdp, const t_State & state, DynaPlex::RNG & rng) {
		{ mdp.GetAction(state, rng) } -> std::same_as<int64_t>;
	};

	template <typename T>
	concept HasIsAllowedAction = requires(const T mdp, const typename T::State state, int64_t action)
	{
		{ mdp.IsAllowedAction(state, action) } -> std::same_as<bool>;
	};

	template<typename T>
	concept HasGetStateCategory = requires(T a, const typename T::State & s) {
		{ a.GetStateCategory(s) } -> std::same_as<StateCategory>;
	};

	template<typename T>
	concept HasEvent = requires{
		typename T::Event;
	};


	template<typename T>
	concept HasState = requires{
		typename T::State;
	};

	template <typename T,typename Registry>
	concept HasRegisterPolicies = requires(T t, Registry& registry) {
		{ t.RegisterPolicies(registry) } -> std::same_as<void>;
	};

	template<typename T>
	concept HasStateConvertibleToVarGroup = requires{
		HasState<T>;
		{ DynaPlex::Concepts::ConvertibleToVarGroup<typename T::State> };
	};

	template<typename T>
	concept HasGetStaticInfo = requires(const T & mdp) {
		{ mdp.GetStaticInfo() } -> std::same_as<DynaPlex::VarGroup>;
	};

	template<typename T>
	concept HasModifyStateWithAction = requires(const T & mdp, typename T::State & state, int64_t action) {
		{ mdp.ModifyStateWithAction(state, action) };
	};

	template <typename T>
	concept HasGetInitialState = requires(const T & mdp)
	{
		{ mdp.GetInitialState() } -> std::same_as<typename T::State>;
	};



}