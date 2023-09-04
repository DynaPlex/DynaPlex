#pragma once
#include <type_traits>

#include "dynaplex/vargroup.h"
#include "dynaplex/policyregistry.h"
namespace DynaPlex::Concepts
{
	template<typename T>
	concept HasState = requires{
		typename T::State;
	};

	template <typename T>
	concept HasRegisterPolicies = requires(T t, PolicyRegistry<T>&registry) {
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