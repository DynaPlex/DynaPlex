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
		std::string id;
		int64_t mdp_identifier;
		t_MDP model;
	public:
		MDPAdapter(DynaPlex::VarGroup vars) :
			model{ vars }, id{ vars.Identifier() }, mdp_identifier{ vars.Int64Hash() }
		{
		}

		std::string Identifier() const override
		{
			return id;
		}

		DynaPlex::States GetInitialStateVec(size_t NumStates) const override 
		{
			auto state = model.GetInitialState();
			using State = decltype(state);
			static_assert(!std::is_void_v<State>, "Deduced type is void");

			std::vector<State> statesVec(NumStates, state);
			 //adding hash to facilitates identifying this vector as coming from current MDP. 			
			return std::make_unique<StatesAdapter<State>>(std::move(statesVec), mdp_identifier);
		}
		DynaPlex::VarGroup::VarGroupVec ToVarGroup(const DynaPlex::States& states) const override
		{
			if (states->mdp_identifier != mdp_identifier)
			{
				throw DynaPlex::Error("Error in MDP->ToVarGroup: It seems you tried to call with states not created by this MDP");
			}

		
			// Perform a static cast to access the underlying states using the get() method
			//Concern:Would this not make a copy?!
			auto statesAdapter = static_cast<StatesAdapter<typename t_MDP::State>*>(states.get());
			const auto& underlyingStates = statesAdapter->get();


			DynaPlex::VarGroup::VarGroupVec vec{};
			vec.reserve(underlyingStates.size());
			for (const auto& state : underlyingStates)
			{
				vec.emplace_back(state.ToVarGroup());
			}

			return vec;
		}

	};
}
