#pragma once
#include "dynaplex/mdp.h"
#include "dynaplex/states.h"
#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include "statesadapter.h"
#include <type_traits>

namespace DynaPlex::Concepts
{
	template<typename T>
	concept HasState = requires{
		typename T::State;
	};

	template<typename T>
	concept HasStateConvertibleToVarGroup = requires{
		HasState<T>;
		{ DynaPlex::Concepts::ConvertibleToVarGroup<typename T::State> };
	};

	template<typename T>
	concept HasGetStaticInfo = requires(const T& mdp){
		{ mdp.GetStaticInfo() } -> std::same_as<DynaPlex::VarGroup>;
	};

	template<typename T>
	concept HasModifyStateWithAction = requires(const T& mdp, typename T::State & state, int64_t action ){
			{ mdp.ModifyStateWithAction(state, action) };
	};

	template <typename T>
	concept HasGetInitialState = requires(const T & mdp)
	{
		{ mdp.GetInitialState() } -> std::same_as<typename T::State>;
	};
	
}

namespace DynaPlex::Erasure
{
	


	template<typename t_MDP>
	class MDPAdapter : public MDPInterface
	{
		static_assert(DynaPlex::Concepts::HasState<t_MDP>, "MDP must publicly define a nested type or using declaration for State");
		static_assert(DynaPlex::Concepts::ConvertibleFromVarGroup<t_MDP>, "MDP must define public constructor with const VarGroup& parameter");
		using State = typename t_MDP::State;

		std::string unique_id;
		int64_t mdp_int_hash;
		t_MDP model;
		std::string mdp_id;
	public:
		MDPAdapter(DynaPlex::VarGroup vars) :
			model{ vars }, unique_id{ vars.UniqueIdentifier() }, mdp_int_hash{ vars.Int64Hash() }, mdp_id{ vars.Identifier() }
		{
		}

		const std::vector<typename t_MDP::State>& ToVector(const DynaPlex::States& states) const
		{
			// Check that the states belong to this MDP
			if (states->mdp_int_hash != mdp_int_hash)
			{
				throw DynaPlex::Error("Error in MDP->ToVector: It seems you tried to call with states not created by this MDP");
			}

			// Cast to the specific StatesAdapter type and access the underlying data
			const StatesAdapter<typename t_MDP::State>* statesAdapter = static_cast<const StatesAdapter<typename t_MDP::State>*>(states.get());
			return statesAdapter->get();
		}
		std::vector<typename t_MDP::State>& ToVector(DynaPlex::States& states) const
		{
			// Check that the states belong to this MDP
			if (states->mdp_int_hash != mdp_int_hash)
			{
				throw DynaPlex::Error("Error in MDP->ToVector: It seems you tried to call with states not created by this MDP");
			}
			// Cast to the specific StatesAdapter type and access the underlying data
			StatesAdapter<typename t_MDP::State>* statesAdapter = static_cast<StatesAdapter<typename t_MDP::State>*>(states.get());
			return statesAdapter->get();
		}

		std::string Identifier() const override
		{
			return unique_id;
		}

		DynaPlex::VarGroup GetStaticInfo() const override
		{
			if constexpr (DynaPlex::Concepts::HasGetStaticInfo<t_MDP>)
			{
				return model.GetStaticInfo();
			}
			else
			{
				throw DynaPlex::Error("MDP.GetStaticInfo in MDP: " + mdp_id + "\nMDP must publicly define GetStaticInfo() const returning DynaPlex::VarGroup.");
			}
		}

		DynaPlex::States GetInitialStateVec(size_t NumStates) const override 
		{
			if constexpr (DynaPlex::Concepts::HasGetInitialState<t_MDP>)
			{
				auto state = model.GetInitialState();
				std::vector<State> statesVec(NumStates, state);
				//adding hash to facilitates identifying this vector as coming from current MDP later on. 			
				return std::make_unique<StatesAdapter<State>>(std::move(statesVec), mdp_int_hash);
			}
			else
			{
				throw DynaPlex::Error("MDP.GetInitialStateVec in MDP: " + mdp_id + "\nMDP must publicly define GetInitialState() const returning MDP::State.");
			}

		}
		DynaPlex::VarGroup ToVarGroup(const DynaPlex::States& dp_states, size_t index) const override
		{
			if constexpr (DynaPlex::Concepts::ConvertibleToVarGroup<State>)
			{
				auto& states = ToVector(dp_states);
				return states[index].ToVarGroup();
			}
			else 
			{
				throw DynaPlex::Error("MDP.ToVarGroup(StateVec,...) in MDP: " + mdp_id + "\nState is not ConvertibleToVarGroup.");
			}
		}
		void IncorporateActions(DynaPlex::States& states) const override
		{
			if constexpr (DynaPlex::Concepts::HasModifyStateWithAction<t_MDP>)
			{
				auto& statesVec = ToVector(states);
				for (auto& state : statesVec)
				{
					int64_t action = 123;
					model.ModifyStateWithAction(state, action);
				}
			}
			else
			{
				throw DynaPlex::Error("MDP.IncorporateActions in MDP: "+ mdp_id + "\nMDP does not publicly define ModifyStateWithAction(MDP::State,int64_t) const returning double");
			}
		}

	};
}
