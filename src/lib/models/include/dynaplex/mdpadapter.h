#pragma once
#include "dynaplex/mdp.h"
#include "dynaplex/states.h"
#include "dynaplex/error.h"
#include "statesadapter.h"
#include "dynaplex/policy.h"
#include "mdp_adapter_helpers/mdpadapter_concepts.h"
#include "mdp_adapter_helpers/randompolicy.h"

#include "policyregistry.h"
#include "dynaplex/rng.h"
#include "dynaplex/vargroup.h"

namespace DynaPlex::Erasure
{
	template<typename t_MDP>
	class MDPAdapter : public MDPInterface
	{
		static_assert(DynaPlex::Concepts::HasState<t_MDP>, "MDP must publicly define a nested type or using declaration for State");
		static_assert(DynaPlex::Concepts::ConvertibleFromVarGroup<t_MDP>, "MDP must define public constructor with const VarGroup& parameter");
		static_assert(DynaPlex::Concepts::HasGetStaticInfo<t_MDP>, "MDP must publicly define GetStaticInfo() const returning DynaPlex::VarGroup.");
		using State = typename t_MDP::State;


		
		std::string unique_id;
		int64_t mdp_int_hash;
		std::shared_ptr<const t_MDP> mdp;
		std::string mdp_id;
		PolicyRegistry<t_MDP> policy_registry;
	public:
		MDPAdapter(const DynaPlex::VarGroup& vars) :
			mdp{ std::make_shared<const t_MDP>(vars) },
			unique_id{ vars.UniqueIdentifier() },
			mdp_int_hash{ vars.Int64Hash() },
			mdp_id{ vars.Identifier() } ,
			policy_registry{}
		{
			auto info = GetStaticInfo();

			RegisterPolicies();
		}

		void RegisterPolicies()
		{
			//add any generic policies like random. 
			//ensure that when the client registers additional policies,
			//they do not have a clash with the generic ones- give a custom warning.
			policy_registry.Register<Helpers::RandomPolicy<t_MDP>>("random", "makes a random choice between the allowed actions");
			if constexpr (DynaPlex::Concepts::HasRegisterPolicies<t_MDP>) {
				mdp->RegisterPolicies(policy_registry);
			}
		}

		//Defined in mdpadapter_tovector.h included below
		const std::vector<State>& ToVector(const DynaPlex::States& states) const;		
		std::vector<typename t_MDP::State>& ToVector(DynaPlex::States& states) const;

		int64_t RandomAction(DynaPlex::RNG& rng) const
		{
			throw "not yet implemented";
		}

		std::string Identifier() const override
		{
			return unique_id;
		}

		DynaPlex::VarGroup GetStaticInfo() const override
		{
			return mdp->GetStaticInfo();
		}

		DynaPlex::States GetInitialStateVec(size_t NumStates) const override
		{
			if constexpr (DynaPlex::Concepts::HasGetInitialState<t_MDP>)
			{
				auto state = mdp->GetInitialState();
				std::vector<State> statesVec(NumStates, state);
				//adding hash to facilitates identifying this vector as coming from current MDP later on. 			
				return std::make_unique<StatesAdapter<State>>(std::move(statesVec), mdp_int_hash);
			}
			else
				throw DynaPlex::Error("MDP.GetInitialStateVec in MDP: " + mdp_id + "\nMDP must publicly define GetInitialState() const returning MDP::State.");
		}
		DynaPlex::VarGroup ToVarGroup(const DynaPlex::States& dp_states, size_t index) const override
		{
			if constexpr (DynaPlex::Concepts::ConvertibleToVarGroup<State>)
			{
				auto& states = ToVector(dp_states);
				return states[index].ToVarGroup();
			}
			else
				throw DynaPlex::Error("MDP.ToVarGroup(StateVec,...) in MDP: " + mdp_id + "\nState is not ConvertibleToVarGroup.");
		}
		void IncorporateActions(DynaPlex::States& states) const override
		{
			if constexpr (DynaPlex::Concepts::HasModifyStateWithAction<t_MDP>)
			{
				auto& statesVec = ToVector(states);
				for (auto& state : statesVec)
				{
					int64_t action = 123;
					mdp->ModifyStateWithAction(state, action);
				}
			}
			else
				throw DynaPlex::Error("MDP.IncorporateActions in MDP: " + mdp_id + "\nMDP does not publicly define ModifyStateWithAction(MDP::State,int64_t) const returning double");
		}

		DynaPlex::Policy GetPolicy(const std::string& id) const override
		{
			DynaPlex::VarGroup vars{};
			vars.Add("id", id);
			return GetPolicy(vars);
		}

		DynaPlex::Policy GetPolicy(const DynaPlex::VarGroup& varGroup) const override
		{
			return policy_registry.GetPolicy(mdp,varGroup,mdp_int_hash);
		}

	};


#include "mdp_adapter_helpers/mdpadapter_tovector.h"


}
	