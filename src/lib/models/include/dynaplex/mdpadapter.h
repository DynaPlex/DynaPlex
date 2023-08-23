#pragma once
#include "dynaplex/mdp.h"
#include "dynaplex/states.h"
#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include "statesadapter.h"
#include <type_traits>
namespace DynaPlex::Erasure
{
	namespace MDP_Concepts
	{

		template<typename T>
		concept ConformingMDPImplementation = requires(const T t) {
			typename T::State; // Ensure a nested type or using declaration for state
			{ t.GetInitialState() } -> std::same_as<typename T::State>; // Ensure the return type matches the nested type
			{ DynaPlex::ConvertibleToVarGroup<typename T::State> }; 
		};
	}



	template<MDP_Concepts::ConformingMDPImplementation t_MDP>
	class MDPAdapter : public MDPInterface
	{
		using State = typename t_MDP::State;
		std::string string_id;
		int64_t mdp_identifier;
		t_MDP model;
	public:
		MDPAdapter(DynaPlex::VarGroup vars) :
			model{ vars }, string_id{ vars.Identifier() }, mdp_identifier{ vars.Int64Hash() }
		{
		}

		const std::vector<typename t_MDP::State>& ToVector(const DynaPlex::States& states) const
		{
			// Check that the states belong to this MDP
			if (states->mdp_identifier != mdp_identifier)
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
			if (states->mdp_identifier != mdp_identifier)
			{
				throw DynaPlex::Error("Error in MDP->ToVector: It seems you tried to call with states not created by this MDP");
			}
			// Cast to the specific StatesAdapter type and access the underlying data
			StatesAdapter<typename t_MDP::State>* statesAdapter = static_cast<StatesAdapter<typename t_MDP::State>*>(states.get());
			return statesAdapter->get();
		}

		std::string Identifier() const override
		{
			return string_id;
		}

		DynaPlex::States GetInitialStateVec(size_t NumStates) const override 
		{
			auto state = model.GetInitialState();		
			std::vector<State> statesVec(NumStates, state);
			 //adding hash to facilitates identifying this vector as coming from current MDP later on. 			
			return std::make_unique<StatesAdapter<State>>(std::move(statesVec), mdp_identifier);
		}
		DynaPlex::VarGroup ToVarGroup(const DynaPlex::States& dp_states, size_t index) const override
		{
			auto& states = ToVector(dp_states);
			return states[index].ToVarGroup();
		}
		void IncorporateActions(DynaPlex::States& states) const override
		{
			auto& statesVec = ToVector(states);
			for (auto& state : statesVec)
			{
				model.ModifyState(state);
			}			
		}

	};
}
