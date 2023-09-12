#pragma once
#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include "dynaplex/rng.h"
#include "dynaplex/mdp.h"
#include "dynaplex/state.h"
#include "dynaplex/policy.h"

#include "erasure_concepts.h"
#include "randompolicy.h"
#include "policyregistry.h"
#include "stateadapter.h"


namespace DynaPlex::Erasure
{
	template<typename t_MDP>
	class MDPAdapter final : public MDPInterface
	{
		static_assert(DynaPlex::Concepts::ConvertibleFromVarGroup<t_MDP>, "MDP must define public constructor with const VarGroup& parameter");
		static_assert(HasState<t_MDP>, "MDP must publicly define a nested type or using declaration for State");
		static_assert(HasGetStaticInfo<t_MDP>, "MDP must publicly define GetStaticInfo() const returning DynaPlex::VarGroup.");
		using t_State = typename t_MDP::State;

		std::string unique_id;
		int64_t mdp_int_hash;
		std::shared_ptr<const t_MDP> mdp;
		std::string mdp_id;
		PolicyRegistry<t_MDP> policy_registry;
		ActionRangeProvider<t_MDP> provider;

		//Registers the policies with the internal registry:
		void RegisterPolicies()
		{
			//Register built-in policies
			policy_registry.Register<RandomPolicy<t_MDP>>("random", "makes a random choice between the allowed actions");
			//register client-provided policies. 
			if constexpr (HasRegisterPolicies<t_MDP, PolicyRegistry<t_MDP>>) {
				mdp->RegisterPolicies(policy_registry);
			}
		}

	public:
		MDPAdapter(const DynaPlex::VarGroup& vars) :
			mdp{ std::make_shared<const t_MDP>(vars) },
			unique_id{ vars.UniqueIdentifier() },
			mdp_int_hash{ vars.Int64Hash() },
			mdp_id{ vars.Identifier() },
			policy_registry{},
			provider{ mdp }
		{
			RegisterPolicies();
		}		

		//Defined in mdpadapter_tostate.h included below
		const t_State& ToState(const DynaPlex::dp_State& state) const;		
		t_State& ToState(DynaPlex::dp_State& state) const;

		std::vector<int64_t> AllowedActions(const DynaPlex::dp_State& dp_state) const override
		{
			auto& state = ToState(dp_state);
			int64_t count{ 0 };
			for (int64_t action : provider(state))
			{
				count++;
			}
			std::vector<int64_t> vec;
			vec.reserve(count);
			for (int64_t action : provider(state))
			{
				vec.push_back(action);
			}
			return vec;
		}

		std::string Identifier() const override
		{
			return unique_id;
		}

		DynaPlex::VarGroup GetStaticInfo() const override
		{
			//guaranteed to exist by static_assert:
			return mdp->GetStaticInfo();
		}

		DynaPlex::dp_State GetInitialState() const override
		{
			if constexpr (HasGetInitialState<t_MDP>)
			{
				t_State state = mdp->GetInitialState();
				//adding hash to facilitate identifying this state as coming from current MDP later on.
				return std::make_unique<StateAdapter<t_State>>(mdp_int_hash, state);
			}
			else
				throw DynaPlex::Error("MDP.GetInitialStateVec in MDP: " + mdp_id + "\nMDP must publicly define GetInitialState() const returning MDP::State.");
		}
		
		void IncorporateAction(DynaPlex::dp_State& dp_state) const override
		{
			if constexpr (HasModifyStateWithAction<t_MDP>)
			{
				auto& state = ToState(dp_state);
				int64_t action = 123;
				mdp->ModifyStateWithAction(state, action);
			}
			else
				throw DynaPlex::Error("MDP.IncorporateActions in MDP: " + mdp_id + "\nMDP does not publicly define ModifyStateWithAction(MDP::State,int64_t) const returning double");
		}


		DynaPlex::VarGroup ListPolicies() const override
		{
			return policy_registry.ListPolicies();
		}

		DynaPlex::Policy GetPolicy(const std::string& id) const override
		{
			DynaPlex::VarGroup vars{};
			vars.Add("id", id);
			return GetPolicy(vars);
		}

		DynaPlex::Policy GetPolicy(const DynaPlex::VarGroup& varGroup) const override
		{
			return policy_registry.GetPolicy(mdp, varGroup, mdp_int_hash);
		}

	};

}

#include "mdpadapter_tostate.h"



	