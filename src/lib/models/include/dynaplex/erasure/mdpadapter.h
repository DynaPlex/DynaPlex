#pragma once
#include <vector>
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
#include <cassert>

namespace DynaPlex::Erasure
{
	template<typename t_MDP>
	class MDPAdapter final : public MDPInterface
	{
		static_assert(DynaPlex::Concepts::ConvertibleFromVarGroup<t_MDP>, "MDP must define public constructor with const VarGroup& parameter");
		static_assert(HasState<t_MDP>, "MDP must publicly define a nested type or using declaration for State");
		static_assert(HasGetStaticInfo<t_MDP>, "MDP must publicly define GetStaticInfo() const returning DynaPlex::VarGroup.");
		using t_State = typename t_MDP::State;
		using t_Event = std::conditional_t<HasEvent<t_MDP>, typename t_MDP::Event, int64_t>;

		static_assert(DynaPlex::Concepts::ConvertibleToVarGroup<t_State>, "MDP::State must define VarGroup ToVarGroup() const");

		std::string unique_id;
		int64_t mdp_int_hash;
		std::shared_ptr<const t_MDP> mdp;
		std::string mdp_type_id;
		PolicyRegistry<t_MDP> policy_registry;
		ActionRangeProvider<t_MDP> provider;
		double discount_factor;
		bool is_infinite_horizon;

		void RegisterPolicies()
		{
			try {
				// Register built-in policies
				policy_registry.template Register<RandomPolicy<t_MDP>>("random", "makes a random choice between the allowed actions");

				// Register client-provided policies. 
				if constexpr (HasRegisterPolicies<t_MDP, PolicyRegistry<t_MDP>>) {
					mdp->RegisterPolicies(policy_registry);
				}
			}
			catch (const DynaPlex::Error& e) {
				throw DynaPlex::Error(std::string("Error in MDPAdapter::RegisterPolicies: ") + e.what());
			}
		}

		void InitializeVariables()
		{
			try {
				auto static_vars = GetStaticInfo();
				if (static_vars.HasKey("discount_factor"))
					static_vars.Get("discount_factor", discount_factor);
				if (discount_factor > 1.0 || discount_factor <= 0.0)
				{
					throw DynaPlex::Error("MDP, id \"" + mdp_type_id + "\" :  GetStaticInfo returns VarGroup with key discount_factor that is invalid: " + std::to_string(discount_factor) + ". Must be in (0.0,1.0].");
				}
				if (static_vars.HasKey("horizon_type"))
				{
					std::string s;
					static_vars.Get("horizon_type", s);
					if (s == "infinite")
					{
						is_infinite_horizon = true;
					}
					else if (s == "finite")
					{
						is_infinite_horizon = false;
					}
					else
					{
						throw DynaPlex::Error("MDP, id \"" + mdp_type_id + "\" : GetStaticInfo returns VarGroup with key horizon_type that is invalid: " + s + ". Must be either \"finite\" or \"infinite\"");
					}
				}

			}
			catch (const DynaPlex::Error& e) {
				// Catch the error, append or modify the message, and rethrow
				throw DynaPlex::Error(std::string("Error in MDPAdapter::RegisterPolicies: ") + e.what());
			}
		}



	public:
		MDPAdapter(const DynaPlex::VarGroup& config) :
			mdp{ std::make_shared<const t_MDP>(config) },
			unique_id{ config.UniqueIdentifier() },
			mdp_int_hash{ config.Int64Hash() },
			mdp_type_id{ config.Identifier() },
			policy_registry{},
			provider{ mdp },
			discount_factor{ 1.0 },
			is_infinite_horizon{ true }
		{
			InitializeVariables();
			RegisterPolicies();
		}

		bool IsInfiniteHorizon() const override
		{
			return is_infinite_horizon;
		}

		double DiscountFactor() const override
		{
			return discount_factor;
		}

		int64_t NumEventRNGs() const override
		{
			//At present, multiple event streams not supported.
			return 1;
		}


		/// Returns bool indicating whether the underlying mdp supports converting a VarGroup to a state. 
		bool SupportsGetStateFromVarGroup() const override
		{
			return HasGetStateFromVars<t_MDP, t_State>;
		}

		/// Returns bool indicating whether the underlying mdp supports equality tests for states. 
		bool SupportsEqualityTest() const override
		{
			return std::equality_comparable<t_State>;

		}



		DynaPlex::dp_State GetState(const VarGroup& vars) const override
		{
			if constexpr (HasGetStateFromVars<t_MDP, t_State>)
			{
				t_State state = mdp->GetState(vars);
				return std::make_unique<StateAdapter<t_State>>(mdp_int_hash, state);
			}
			else
				throw DynaPlex::Error("MDP->GetState(const VarGroup&): MDP must publicly define MDP::GetState(const VarGroup&) const returning MDP::State. ");
		}
		bool StatesAreEqual(const DynaPlex::dp_State& state1, const DynaPlex::dp_State& state2) const override
		{
			if constexpr (std::equality_comparable<t_State>)
			{
				auto& t_state1 = ToState(state1);
				auto& t_state2 = ToState(state2);
				return t_state1 == t_state2;
			}
			else
				throw DynaPlex::Error("MDP->StatesAreEqual(dp_State&,dp_State&) : MDP does not support comparing states for equality. Are any State member variables not equality comparable? ");

		}

		//Defined in mdpadapter_tostate.h included below
		const t_State& ToState(const DynaPlex::dp_State& state) const;
		t_State& ToState(DynaPlex::dp_State& state) const;

		std::vector<int64_t> AllowedActions(const DynaPlex::dp_State& dp_state) const override
		{
			auto& t_state = ToState(dp_state);
			auto actions = provider(t_state);
			std::vector<int64_t> vec;
			vec.reserve(actions.Count());
			for (int64_t action : actions)
			{
				vec.push_back(action);
			}
			return vec;
		}

		std::string TypeIdentifier() const override
		{
			return mdp_type_id;
		}

		std::string Identifier() const override
		{
			return unique_id;
		}

		DynaPlex::VarGroup GetStaticInfo() const override
		{	//guaranteed to exist by static_assert:
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
				throw DynaPlex::Error("MDP->GetInitialStateVec in MDP: " + mdp_type_id + "\nMDP must publicly define GetInitialState() const returning MDP::State.");
		}



		void IncorporateAction(std::span<DynaPlex::Trajectory> trajectories) const override
		{
			if constexpr (HasModifyStateWithAction<t_MDP>)
			{
				for (DynaPlex::Trajectory& traj : trajectories)
				{
					auto& state = ToState(traj.GetState());

					if (traj.Category.IsAwaitAction())
					{
						traj.CumulativeReturn += mdp->ModifyStateWithAction(state, traj.NextAction) * traj.EffectiveDiscountFactor;
						traj.Category = mdp->GetStateCategory(state);
					}
					else
					{
						throw DynaPlex::Error("MDP->IncorporateActions: Cannot incorporate action if Trajectory.Category is not AwaitAction");
					}
				}
			}
			else
				throw DynaPlex::Error("MDP->IncorporateActions: " + mdp_type_id + "\nMDP does not publicly define ModifyStateWithAction(MDP::State,int64_t) const returning double");

		}
		void IncorporateAction(std::span<DynaPlex::Trajectory> trajectories, const DynaPlex::Policy& policy) const override
		{
			policy->SetAction(trajectories);
			IncorporateAction(trajectories);
		}
		template <bool SkipTrivial>
		bool IncorporateUntilSomeAction(std::span<DynaPlex::Trajectory> trajectories, int64_t MaxPeriodCount) const
		{
			bool AllAwaitAction = true;

			for (DynaPlex::Trajectory& traj : trajectories)
			{
				auto& t_state = ToState(traj.GetState());

				while (traj.PeriodCount < MaxPeriodCount && traj.Category.IsAwaitEvent())
				{
					auto event_stream = traj.Category.Index();
					if (event_stream == 0)
					{
						traj.PeriodCount++;
						traj.EffectiveDiscountFactor *= discount_factor;
					}
					if constexpr (HasGetEvent<t_MDP, t_Event, DynaPlex::RNG>)
					{
						t_Event Event = mdp->GetEvent(traj.RNGProvider.GetEventRNG(event_stream));
						traj.CumulativeReturn += mdp->ModifyStateWithEvent(t_state, Event) * traj.EffectiveDiscountFactor;
					}
					else if constexpr (HasGetStateDependentEvent<t_MDP, t_State, t_Event, DynaPlex::RNG>)
					{
						t_Event Event = mdp->GetEvent(t_state, traj.RNGProvider.GetEventRNG(event_stream));
						traj.CumulativeReturn += mdp->ModifyStateWithEvent(t_state, Event) * traj.EffectiveDiscountFactor;
					}
					else
						throw DynaPlex::Error("MDP->IncorporateEvent: " + mdp_type_id + "\nMDP does not publicly define function GetEvent(DynaPlex::RNG&) returning MDP::Event. ");
					traj.Category = mdp->GetStateCategory(t_state);

					int64_t action_count;
					if constexpr (SkipTrivial)
					{
						while (traj.Category.IsAwaitAction())
						{
							auto actions = provider(t_state);
							if (actions.Count() == 1)
							{//trivial action:	
								traj.NextAction = *(actions.begin());
								if constexpr (HasModifyStateWithAction<t_MDP>)
								{
									traj.CumulativeReturn += mdp->ModifyStateWithAction(t_state, traj.NextAction) * traj.EffectiveDiscountFactor;
									traj.Category = mdp->GetStateCategory(t_state);
								}
								else
									throw DynaPlex::Error("MDP->IncorporateUntilNonTrivialAction: " + mdp_type_id + "\nMDP does not publicly define ModifyStateWithAction(MDP::State,int64_t) const returning double");
							}
							else
							{//nontrivial action:
								break;//the inner while loop. 
							}
						}
					}
				}
				if (!traj.Category.IsAwaitAction())
				{
					AllAwaitAction = false;
				}
				assert(traj.Category.IsAwaitAction() || traj.Category.IsFinal() || traj.PeriodCount == MaxPeriodCount);

			}
			return AllAwaitAction;
		}

		bool IncorporateUntilAction(std::span<DynaPlex::Trajectory> trajectories, int64_t MaxPeriodCount) const override
		{
			return IncorporateUntilSomeAction<false>(trajectories, MaxPeriodCount);
		}

		
		bool IncorporateUntilNonTrivialAction(std::span<DynaPlex::Trajectory> trajectories, int64_t MaxPeriodCount) const override
		{
			return IncorporateUntilSomeAction<true>(trajectories, MaxPeriodCount);
		}


		bool IncorporateEvent(std::span<DynaPlex::Trajectory> trajectories) const override
		{

			bool EventsRemaining = false;
			for (DynaPlex::Trajectory& traj : trajectories)
			{
				if (traj.Category.IsAwaitEvent())
				{
					auto& state = ToState(traj.GetState());
					auto event_stream = traj.Category.Index();
					if (event_stream == 0)
					{
						traj.PeriodCount++;
						traj.EffectiveDiscountFactor *= discount_factor;
					}
					if constexpr (HasModifyStateWithEvent<t_MDP, t_State, t_Event>)
					{
						if constexpr (HasGetEvent<t_MDP, t_Event, DynaPlex::RNG>)
						{
							t_Event Event = mdp->GetEvent(traj.RNGProvider.GetEventRNG(event_stream));
							traj.CumulativeReturn += mdp->ModifyStateWithEvent(state, Event) * traj.EffectiveDiscountFactor;
						}
						else if constexpr (HasGetStateDependentEvent<t_MDP, t_State, t_Event, DynaPlex::RNG>)
						{
							t_Event Event = mdp->GetEvent(state, traj.RNGProvider.GetEventRNG(event_stream));
							traj.CumulativeReturn += mdp->ModifyStateWithEvent(state, Event) * traj.EffectiveDiscountFactor;
						}
						else
							throw DynaPlex::Error("MDP->IncorporateEvent: " + mdp_type_id + "\nMDP does not publicly define function GetEvent(DynaPlex::RNG&) returning MDP::Event. ");

					}
					else //if constexpr 
						throw DynaPlex::Error("MDP->IncorporateEvent: " + mdp_type_id + "\nMDP does not publicly define ModifyStateWithEvent(MDP::State&, const MDP::Event&) returning double.");


					traj.Category = mdp->GetStateCategory(state);
					if (traj.Category.IsAwaitEvent())
					{
						EventsRemaining = true;
					}
				}
			}
			return EventsRemaining;
		}
		virtual void InitiateState(std::span<DynaPlex::Trajectory> trajectories) const override
		{
			if constexpr (HasGetInitialRandomState<t_MDP, DynaPlex::RNG>)
			{
				for (DynaPlex::Trajectory& traj : trajectories)
				{
					t_State state = mdp->GetInitialState(traj.RNGProvider.GetInitiationRNG());
					traj.Category = mdp->GetStateCategory(state);
					traj.Reset(std::move(std::make_unique<StateAdapter<t_State>>(mdp_int_hash, state)));
				}
			}
			else if constexpr (HasGetInitialState<t_MDP>)
			{
				for (DynaPlex::Trajectory& traj : trajectories)
				{
					t_State state = mdp->GetInitialState();
					traj.Category = mdp->GetStateCategory(state);
					traj.Reset(std::move(std::make_unique<StateAdapter<t_State>>(mdp_int_hash, state)));
				}
			}
			else
				throw DynaPlex::Error("MDP->InitiateState: " + mdp_type_id + "\nMDP does not publicly define GetInitialState() const or GetInitialState(DynaPlex::RNG&) const returning MDP::State");
		}
		virtual void InitiateState(std::span<DynaPlex::Trajectory> trajectories, const DynaPlex::dp_State& state) const override
		{
			for (DynaPlex::Trajectory& traj : trajectories)
			{
				traj.Reset(std::move(state->Clone()));
				auto& t_state = ToState(state);
				traj.Category = mdp->GetStateCategory(t_state);
			}
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



