#include "dynaplex/sequentialhalving.h"
#include "dynaplex/trajectory.h"
#include "dynaplex/parallel_execute.h"
#include "dynaplex/policycomparison.h"
namespace DynaPlex::DCL {


	struct experiment_info
	{
		int64_t action_id;
		int64_t experiment_number;
	};

	SequentialHalving::SequentialHalving(uint32_t rng_seed, int64_t H, int64_t M, DynaPlex::MDP& mdp, DynaPlex::Policy& policy)
		: rng_seed{ rng_seed }, H{ H }, M{ M }, mdp{ mdp }, policy{ policy }
	{

	}

	//See uniformactionselector.cpp for changing these values
	bool adopt_crn_sh = true;
	int64_t max_chunk_size_sh = 256;

	void SequentialHalving::SetAction(DynaPlex::Trajectory& traj, const int32_t seed) const
	{
		if (!traj.Category.IsAwaitAction())
			throw DynaPlex::Error("SequentialHalving::SetAction - called for trajectory which is not await_action.");

		auto root_state = traj.GetState()->Clone();
		auto root_actions = mdp->AllowedActions(root_state);
		if (root_actions.size() <= 1)
			throw DynaPlex::Error("SequentialHalving::SetAction - called for state with only single<=1 allowed actions.");

		std::vector<experiment_info> experiment_information{};
		std::vector<DynaPlex::Trajectory> trajectories{};

		auto competing_actions = root_actions;
		double objective = mdp->Objective(root_state);
		std::vector<double> accumulated_rewards(competing_actions.size(), 0.0);
		int64_t total_budget_used{ 0 };
		int64_t total_budget = M * root_actions.size();
		int64_t total_rounds = ceil(log(root_actions.size()) / log(2));

		for (int64_t iter = 0; iter < total_rounds; iter++)
		{
			//Number of scenarios assigned for each competing_action at this round.
			int64_t action_budget = floor(total_budget / (competing_actions.size() * ceil(log(root_actions.size()) / log(static_cast<double>(2)))));
			//Number of competing_actions to be kept at the end of this round.
			int64_t top_m = ceil(competing_actions.size() / static_cast<double>(2));

			//Clearing and reserving resourses.
			trajectories.clear();
			experiment_information.clear();
			trajectories.reserve(competing_actions.size() * action_budget);
			experiment_information.reserve(competing_actions.size() * action_budget);

			//Create action_budget replications for each competing_action, with appropriate random seed.
			for (int64_t replication = 0; replication < action_budget; replication++)
			{
				for (int64_t action_id = 0; action_id < competing_actions.size(); action_id++)
				{
					auto competing_action = competing_actions[action_id];
					trajectories.emplace_back(mdp->NumEventRNGs(), experiment_information.size());
					int64_t traj_seed = adopt_crn_sh ? replication : experiment_information.size() + 1;
					trajectories.back().SeedRNGProvider(false, traj_seed, seed ^ rng_seed);
					trajectories.back().NextAction = competing_action;
					experiment_information.emplace_back(action_id, replication);
				}
			}

			//iterate over chunks of the trajectories list, and process those for H steps or until final state. 
			auto chunks = DynaPlex::Parallel::get_chunks(trajectories.size(), max_chunk_size_sh);
			for (auto& [start, end] : chunks)
			{
				std::span<DynaPlex::Trajectory> span(&trajectories[start], end - start);
				mdp->InitiateState(span, root_state);
				mdp->IncorporateAction(span);
				while (true)
				{
					if (!mdp->IncorporateUntilAction(span, H))
					{
						//This "sorts" the trajectories, such that the trajectories that are IsAwaitAction are at the front.
						// Note that mdp->IncorporateAction only accepts set of adjacent trajectories that are all AwaitAction. 
						std::span<DynaPlex::Trajectory>::iterator new_partition_point = std::partition(span.begin(), span.end(),
							[](const DynaPlex::Trajectory& traj) {return traj.Category.IsAwaitAction(); }
						);
						//new_partition_point is the first element which does not require an action. 
						//Make the span refer to a set of trajectories each awaiting an action:
						span = std::span<DynaPlex::Trajectory>(span.begin(), new_partition_point);
					}
					//This means all trajectories are at period warmup_periods or final. 
					if (span.size() == 0)
						break;
					//other actions use roll-out policy.
					mdp->IncorporateAction(span, policy);
				}
				//reset span to original
				span = { &trajectories[start], static_cast<size_t>(end - start) };
				//some checks:
				if (mdp->IsInfiniteHorizon())
				{
					for (auto& traj : span)
					{
						if (traj.Category.IsFinal())
							throw DynaPlex::Error("SequentialHalving::SetAction - state has Final Category but MDP is infinite horizon()");
						if (traj.PeriodCount != H)
							throw DynaPlex::Error("SequentialHalving::SetAction - unexpected value of PeriodCount after rollout");
					}
				}
				else
				{
					for (auto& traj : span)
						if (!(traj.Category.IsFinal() || traj.PeriodCount == H))
							throw DynaPlex::Error("SequentialHalving::SetAction - unexpected trajectory status after rollout");
				}
				//freeing resources
				for (auto& traj : span)
					traj.DeleteState();
			}

			std::vector<std::vector<double>> return_results(competing_actions.size(), std::vector<double>(action_budget, 0.0));
			//A vector of tokens keeping track of the indices of competing_actions in the original root_actions
			std::vector<int64_t> action_id_keeper(competing_actions.size(), -1);
			//Collect results and draw conclusion:
			for (auto& traj : trajectories)
			{
				//Note that trajectories were possibly reshuffled; recover experiment information safely:
				auto& info = experiment_information[traj.ExternalIndex];
				//We have results for each competing action. 
				return_results.at(info.action_id).at(info.experiment_number) = traj.CumulativeReturn * objective;
				auto it = std::lower_bound(root_actions.begin(), root_actions.end(), competing_actions.at(info.action_id));
				if (it != root_actions.end() && *it == competing_actions.at(info.action_id)) {
					int64_t action_original_id = it - root_actions.begin();
					accumulated_rewards.at(action_original_id) += traj.CumulativeReturn * objective;
					if (action_id_keeper.at(info.action_id) == -1){
						action_id_keeper.at(info.action_id) = action_original_id;
					}
				}
				else {
					throw DynaPlex::Error("SequentialHalving::SetAction - cannot find action_originial_id.");
				}
			}

			total_budget_used += action_budget;
			// Pairing the competing actions and mean rewards
			std::vector<std::pair<int64_t, double>> paired;
			for (int64_t i = 0; i < competing_actions.size(); ++i) {
				double mean_reward = accumulated_rewards[action_id_keeper[i]] / total_budget_used;
				paired.push_back({ competing_actions[i], mean_reward });
			}
			// Sorting the competing actions based on mean rewards by arg_max
			// which because of objective will correspond to minimum or maximum costs as appropriate
			std::sort(paired.begin(), paired.end(), [](const auto& a, const auto& b) {
				return a.second > b.second;
			});
			// Extracting the top_m performing actions
			for (int i = 0; i < top_m; ++i) {
				competing_actions[i] = paired[i].first;
			}
			competing_actions.resize(top_m); // keep only top_m performing actions

			double best_reward = paired[0].second;
			if (best_reward == -std::numeric_limits<double>::infinity())
				throw DynaPlex::Error("SequentialHalving:SetAction - error in logic");

			if (iter == total_rounds - 1)
			{
				traj.NextAction = competing_actions.front();
			}
		}
	}


}  // namespace DynaPlex::DCL
