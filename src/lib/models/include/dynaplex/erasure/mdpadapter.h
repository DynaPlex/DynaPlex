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
		static_assert(HasGetStateCategory<t_MDP>, "MDP must publicly define a function GetStateCategory(const MDP::State) const that returns a StateCategory");
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
		int64_t num_flat_features;


		int64_t NumValidActions() const override {
			return provider.NumValidActions();
		}
		bool ProvidesFlatFeatures() const override {
			return HasGetFlatFeatures<t_MDP, t_State>;
		}

		bool ProvidesEventProbs() const override {
			return HasEventProbabilities<t_MDP, t_Event> || HasStateDependendentEventProbabilities<t_MDP, t_State, t_Event>;
		}

		double AllEventTransitions(const DynaPlex::dp_State& dp_state, std::vector<std::tuple<double, DynaPlex::dp_State>>& transitions) const override {
			if (HasHiddenStateVariables())
				throw DynaPlex::Error("MDP::AllEventTransitions : Cannot return event transitions as state has hidden variables.");
			try {
				std::vector<std::tuple<t_Event, double>>  eventProbs;
		

				auto& t_state = ToState(dp_state);
				const StateCategory cat = mdp->GetStateCategory(t_state);
				if (!cat.IsAwaitEvent())
					throw DynaPlex::Error("MDP::AllTransitions - called with state argument that does not await event.");

				if constexpr (HasEventProbabilities<t_MDP, t_Event>)
				{
					eventProbs = mdp->EventProbabilities();
				}
				else if constexpr (HasStateDependendentEventProbabilities<t_MDP,t_State,t_Event>)
				{
					eventProbs = mdp->EventProbabilities(t_state);
				}
				else {
					throw DynaPlex::Error("MDP does not implement EventProbabilities");
				}
				transitions.reserve(eventProbs.size());
				double expected_cost{ 0.0 };
				for (auto& [Event, prob] : eventProbs)
				{
					if (prob > 0.0)
					{
						if constexpr (HasModifyStateWithEvent<t_MDP, t_State, t_Event>)
						{
							auto clone = dp_state->Clone();
							auto& t_state = ToState(clone);
							expected_cost+= mdp->ModifyStateWithEvent(t_state, Event)*prob;
							transitions.push_back(std::move(std::make_tuple(prob, std::move(clone))));
						}
						else
						{
							throw DynaPlex::Error("MDP does not implement ModifyStateWithEvent");
						}
					}
				}			
				return expected_cost;				
			}
			catch (const DynaPlex::Error& e) {
				throw DynaPlex::Error(std::string("Error in MDPAdapter::GetAllTransitions: ") + e.what());
			}
		}

		int64_t NumFlatFeatures() const override {
			if constexpr (HasGetFlatFeatures<t_MDP, t_State>)
				return num_flat_features;
			throw DynaPlex::Error("MDP::NumFlatFeatures: mdp " + mdp_type_id + " does not have num_flat_features; underlying mdp does not define void GetFeatures(const DynaPlex::State&, DynaPlex::Features&) const ");
		}




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

		bool HasHiddenStateVariables() const override {
			return HasResetHiddenStateVariables<t_MDP, t_State, DynaPlex::RNG>;
		}


		void InitializeFeatureMetaInfo(const DynaPlex::VarGroup& static_vars)
		{
			if constexpr (HasGetFlatFeatures<t_MDP, t_State>)
			{
				if (static_vars.HasKey("num_flat_features"))
				{
					static_vars.Get("num_flat_features", num_flat_features);
				}
				else
				{
					//try to automatically determine the number of flat features, by finding an actions state
					//and calling GetFeatures to get the features, and counting the number of features returned. 
					DynaPlex::Trajectory traj{};
					std::span<DynaPlex::Trajectory> span = { &traj,1 };
					traj.RNGProvider.SeedEventStreams(false);
					try
					{
						InitiateState(span);
						IncorporateUntilAction(span, 256);
					}
					catch (const DynaPlex::Error& e) {
						throw DynaPlex::Error(std::string("Error in mdp initialization for " + mdp_type_id + ". Could not automatically determine NumFlatFeatures. (Consider defining \"num_flat_features\" on the VarGroup returned from GetStaticInfo.) Error: \n") + e.what());
					}
					if (!traj.Category.IsAwaitAction())
					{
						throw DynaPlex::Error(std::string("Error in mdp initialization for " + mdp_type_id + ". Could not automatically determine NumFlatFeatures: \n Failed to automatically determine a state that awaits an action. \n Consider defining \"num_flat_features\" on the VarGroup returned from GetStaticInfo. "));
					}
					else
					{
						std::vector<float> feature_store(0, 0.0);
						DynaPlex::Features feats(feature_store);
						auto& t_state = ToState(traj.GetState());
						mdp->GetFeatures(t_state, feats);
						num_flat_features = feats.NumFeatsAdded();
					}
				}
			}
			else
			{
				num_flat_features = 0;
			}

		}

		void InitializeVariables()
		{
			try {
				auto static_vars = GetStaticInfo();
				//flat_features				
				InitializeFeatureMetaInfo(static_vars);
				//discount_factor
				if (static_vars.HasKey("discount_factor"))
					static_vars.Get("discount_factor", discount_factor);
				if (discount_factor > 1.0 || discount_factor <= 0.0)
				{
					throw DynaPlex::Error("MDP, id \"" + mdp_type_id + "\" :  GetStaticInfo returns VarGroup with key discount_factor that is invalid: " + std::to_string(discount_factor) + ". Must be in (0.0,1.0].");
				}
				//horizon_type
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
						throw DynaPlex::Error("MDP, id \"" + mdp_type_id + "\" : GetStaticInfo returns VarGroup with key horizon_type that is invalid: " + s + ". Must be either \"finite\" or \"infinite\"");
				}
				//num_flat_features


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

		bool SupportsGetStateFromVarGroup() const override
		{
			return HasGetStateFromVars<t_MDP, t_State>;
		}

		bool SupportsEqualityTest() const override
		{
			return std::equality_comparable<t_State>;
		}

	

		void GetFlatFeatures(const DynaPlex::dp_State& state, std::span<float> feats) const override
		{
			if constexpr (HasGetFlatFeatures<t_MDP, t_State>)
			{
				if (num_flat_features != feats.size())
					throw DynaPlex::Error("MDP->GetFlatFeatures(state,feats): size of feats argument does not equal NumFlatFeatures");

				auto& t_state = ToState(state);

				auto cat = mdp->GetStateCategory(t_state);
				if (!cat.IsAwaitAction())
					throw DynaPlex::Error("MDP->GetFlatFeatures(state,feats): state Category does not satisfy Category.IsAwaitAction().");

				DynaPlex::Features traj_feats(feats);
				mdp->GetFeatures(t_state, traj_feats);
				if (!traj_feats.IsFilled())
					throw DynaPlex::Error("MDP->GetFlatFeatures(state,feats): mdp->GetFeatures(const State&, DynaPlex::Features&) const for mdp type " + mdp_type_id + " returns number of features that is inconsistent with num_flat_features. Possible causes: \n1) GetFeatures returns different numbers of features for different states; ensure consistency. \n2) num_flat_features as returned by GetStaticInfo is inconsistent with the number of features actually returned by GetFeatures(const State&, DynaPlex::Features&) const.");
			}
			else
				throw DynaPlex::Error("MDP->GetFlatFeatures(state,feats): MDP must publicly define MDP::GetFeatures(const State&, DynaPlex::Features&) const returning void.");

		}


		void SetArgMaxAction(std::span<Trajectory> trajectories, std::span<float> values_per_valid_action) const override
		{
			size_t num_valid_actions = static_cast<size_t>(provider.NumValidActions());
			if ( trajectories.size() * num_valid_actions != values_per_valid_action.size())
				throw DynaPlex::Error("MDP->SetArgMaxAction - nonconformant dimensions of values_per_valid_action and trajectories.  ");

			size_t offset = 0;
			for (auto& traj : trajectories)
			{
				auto values_for_traj = values_per_valid_action.subspan(offset, num_valid_actions);
				auto& t_state = ToState(traj.GetState());
				float best_val = -std::numeric_limits<float>::infinity();
				for (const auto& action : provider(t_state))
				{
					if (values_for_traj[action] > best_val)
					{
						traj.NextAction = action;
						best_val = values_for_traj[action];
					}
				}				
				offset += num_valid_actions;
			}
		}

		void GetMask(const std::span<DynaPlex::Trajectory> trajectories, std::span<bool> mask) const override
		{
			auto num_valid_actions = provider.NumValidActions();
			if (num_valid_actions * trajectories.size() != mask.size())
				throw DynaPlex::Error("MDP->GetMask: size of mask argument does not equal NumAllowedActions* trajectories.size()");
			size_t offset = 0;
			for (const auto& trajectory : trajectories)
			{
				if (!trajectory.Category.IsAwaitAction())
					throw DynaPlex::Error("MDP->GetMask: trajectory in trajectories does not satisfy Category.IsAwaitAction().");
				auto& t_state = ToState(trajectory.GetState());
				for (auto action : provider(t_state))
				{
					mask[offset + action] = true;
				}
				offset += num_valid_actions;
			}
		}

		void GetFlatFeatures(const std::span<DynaPlex::Trajectory> trajectories, std::span<float> feats) const override
		{
			if constexpr (HasGetFlatFeatures<t_MDP, t_State>)
			{
				if (num_flat_features * trajectories.size() != feats.size())
					throw DynaPlex::Error("MDP->GetFlatFeatures(trajectories,feats): size of feats argument does not equal NumFlatFeatures*trajectories.size()");

				size_t offset = 0;
				for (const auto& trajectory : trajectories)
				{
					if (!trajectory.Category.IsAwaitAction())
						throw DynaPlex::Error("MDP->GetFlatFeatures(trajectories,feats): trajectory in trajectories does not satisfy Category.IsAwaitAction().");

					auto sub_feats = feats.subspan(offset, num_flat_features);
					DynaPlex::Features traj_feats(sub_feats);
					auto& t_state = ToState(trajectory.GetState());
					mdp->GetFeatures(t_state, traj_feats);
					if (!traj_feats.IsFilled())
						throw DynaPlex::Error("MDP->GetFlatFeatures(trajectories,feats): mdp->GetFeatures(const State&, DynaPlex::Features&) const for mdp type " + mdp_type_id + " returns number of features that is inconsistent with num_flat_features. Possible causes: \n1) GetFeatures returns different numbers of features for different states; ensure consistency. \n2) num_flat_features as returned by GetStaticInfo is inconsistent with the number of features actually returned by GetFeatures(const State&, DynaPlex::Features&) const.");

					offset += num_flat_features;
				}
			}
			else
				throw DynaPlex::Error("MDP->GetFlatFeatures(trajectories,feats): MDP must publicly define MDP::GetFeatures(const State&, DynaPlex::Features&) const returning void.");
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

		bool CheckConformant(const DynaPlex::dp_State& state) const override
		{
			return state->mdp_int_hash == mdp_int_hash;
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

		bool IsAllowedAction(const DynaPlex::dp_State& dp_state, int64_t action) const override
		{
			auto& t_state = ToState(dp_state);
			return provider.IsAllowedAction(t_state, action);
		}

		int64_t CountAllowedActions(const DynaPlex::dp_State& dp_state) const override {
			auto& t_state = ToState(dp_state);
			return provider.CountAllowedActions(t_state);
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
				auto& t_state = ToState(traj.GetState());
				if constexpr (HasResetHiddenStateVariables<t_MDP, t_State, DynaPlex::RNG>)
				{
					mdp->ResetHiddenStateVariables(t_state, traj.RNGProvider.GetInitiationRNG());
				}
				traj.Category = mdp->GetStateCategory(t_state);
			}
		}

		double Objective(const DynaPlex::dp_State& state) const override
		{//currently, only minimization is supported. 
			return -1.0;
		}
		double Objective() const override
		{//currently, only minimization is supported. 
			return -1.0;
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



