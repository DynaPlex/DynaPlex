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
		concept HasGetInitialState = requires(T t) {
			{ t.GetInitialState() };
		};
	}


	template<MDP_Concepts::HasGetInitialState t_MDP>
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

		std::string Identifier() override
		{
			return id;
		}

		DynaPlex::States GetInitialStateVec(size_t NumStates) override 
		{
			auto state = model.GetInitialState();
			using State = decltype(state);
			static_assert(!std::is_void_v<State>, "Deduced type is void");

			std::vector<State> statesVec(NumStates, state);
			//return type_erased version of vector. 
			//add hash to facilitate identifying this vector as coming from current MDP. 
			
			return std::make_unique<StatesAdapter<State>>(std::move(statesVec), mdp_identifier);
		}


		virtual DynaPlex::VarGroup::VarGroupVec ToVarGroup(DynaPlex::States& states) override
		{
			if (states->mdp_identifier!=mdp_identifier)
			{
				throw DynaPlex::Error("Error in MDP->ToVarGroup: It seems you tried to call with states not created by this MDP");
			}
			DynaPlex::VarGroup::VarGroupVec vec{};
			


			return DynaPlex::VarGroup::VarGroupVec{};
		}


	};
}
