#include "dynaplex/sequentialhalving.h"
#include "dynaplex/trajectory.h"
#include "dynaplex/parallel_execute.h"
#include "dynaplex/policycomparison.h"
#include <cmath>
namespace DynaPlex::DCL {


	struct experiment_info
	{
		int64_t action_id;
		int64_t experiment_number;
	};

	SequentialHalving::SequentialHalving(int64_t rng_seed, int64_t H, int64_t M, DynaPlex::MDP& mdp, DynaPlex::Policy& policy)
		: rng_seed{ rng_seed }, H{ H }, M{ M }, mdp{ mdp }, policy{ policy }
	{

	}

	bool adopt_crn_sh = true;
	int64_t max_chunk_size_sh = 256;
	int64_t max_steps_until_completion_expected_sh = 1000000;
	void SequentialHalving::SetAction(DynaPlex::Trajectory& traj, DynaPlex::NN::Sample& sample, int64_t seed) const
	{
		if (!traj.Category.IsAwaitAction())
			throw DynaPlex::Error("SequentialHalving::SetAction - called for trajectory which is not await_action.");

		auto root_state = traj.GetState()->Clone();
		auto root_actions = mdp->AllowedActions(root_state);

		policy->SetAction({ &traj,1 });
		auto prescribed_action_initial_policy = traj.NextAction;
		bool prescribed_action_allowed = true;
		auto it = std::lower_bound(root_actions.begin(), root_actions.end(), prescribed_action_initial_policy);
		if (it != root_actions.end() && *it == prescribed_action_initial_policy) {
			prescribed_action_initial_policy = it - root_actions.begin();
		}
		else {
			prescribed_action_allowed = false;
		}

		if (root_actions.size() <= 1)
			throw DynaPlex::Error("SequentialHalving::SetAction - called for state with only single<=1 allowed actions.");

		std::vector<experiment_info> experiment_information{};
		std::vector<DynaPlex::Trajectory> trajectories{};

		double objective = mdp->Objective(root_state);
		std::vector<double> accumulated_rewards(root_actions.size(), 0.0);
		std::vector<std::vector<double>> trajectory_costs(root_actions.size());
		int64_t total_budget_used_per_action{ 0 };
		int64_t seed_keeper{ 0 }; // used when disabling CRN
		int64_t total_budget = M * root_actions.size();
		int64_t total_rounds = ceil(log(root_actions.size()) / log(2));
		auto competing_actions = root_actions;

		for (int64_t iter = 0; iter < total_rounds; iter++)
		{
			//Number of scenarios assigned for each competing_action at this round.
			int64_t action_budget = std::floor(total_budget / (competing_actions.size() * std::ceil(std::log(root_actions.size()) / std::log(static_cast<double>(2)))));
			//Number of competing_actions to be kept at the end of this round.
			int64_t top_m = std::ceil(competing_actions.size() / static_cast<double>(2));

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
					trajectories.push_back(DynaPlex::Trajectory(experiment_information.size()));
					int64_t traj_seed = adopt_crn_sh ? (total_budget_used_per_action + replication) : seed_keeper + experiment_information.size() + 1;
					trajectories.back().RNGProvider.SeedEventStreams(false,rng_seed,seed, traj_seed);
					trajectories.back().NextAction = competing_action;
					experiment_information.push_back(DynaPlex::DCL::experiment_info(action_id, replication));
				}
			}
			seed_keeper += experiment_information.size(); 

			//iterate over chunks of the trajectories list, and process those for H steps or until final state. 
			auto chunks = DynaPlex::Parallel::get_chunks(trajectories.size(), max_chunk_size_sh);
			for (auto& [start, end] : chunks)
			{
				std::span<DynaPlex::Trajectory> span(&trajectories[start], end - start);
				mdp->InitiateState(span, root_state);
				mdp->IncorporateAction(span);
				int64_t count = 0;
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
						if (++count > max_steps_until_completion_expected_sh)
							throw DynaPlex::Error("SequentialHalving::SetAction"
								"- expected completion of simulation run after max_steps_until_completion_expected: "
								+ std::to_string(max_steps_until_completion_expected_sh) +
								" but completion was not reached.");
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
					if (action_id_keeper.at(info.action_id) == -1) {
						action_id_keeper.at(info.action_id) = action_original_id;
					}
				}
				else {
					throw DynaPlex::Error("SequentialHalving::SetAction - cannot find action_originial_id.");
				}
			}

			//Append the results
			for (int64_t action_id = 0; action_id < competing_actions.size(); action_id++) {
				int64_t action_original_id = action_id_keeper[action_id];
				trajectory_costs[action_original_id].insert(
					trajectory_costs[action_original_id].end(),
					return_results[action_id].begin(),
					return_results[action_id].end()
				);
			}

			total_budget_used_per_action += action_budget;
			// Pairing the competing actions and mean rewards
			std::vector<std::pair<int64_t, double>> paired;
			for (int64_t i = 0; i < competing_actions.size(); ++i) {
				double mean_reward = accumulated_rewards[action_id_keeper[i]] / total_budget_used_per_action;
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
			if (iter == 0 && !prescribed_action_allowed) {
				prescribed_action_initial_policy = competing_actions.back();
			}
			competing_actions.resize(top_m); // keep only top_m performing actions

			double best_reward = paired[0].second;
			if (best_reward == -std::numeric_limits<double>::infinity())
				throw DynaPlex::Error("SequentialHalving:SetAction - error in logic");

			if (iter == total_rounds - 1)
			{
				// Sequential halving algorithm ends, collect statistics 
				traj.NextAction = competing_actions.front();
				sample.state = traj.GetState()->Clone();
				sample.sample_number = seed;
				sample.action_label = traj.NextAction;
				sample.cost_improvement.reserve(root_actions.size());
				sample.q_hat_vec.reserve(root_actions.size());
				sample.probabilities.reserve(root_actions.size());
				sample.q_hat = best_reward * objective;

				int64_t best_action_id = 0;
				auto it = std::lower_bound(root_actions.begin(), root_actions.end(), traj.NextAction);
				if (it != root_actions.end() && *it == traj.NextAction) {
					best_action_id = it - root_actions.begin();
				}
				else {
					throw DynaPlex::Error("SequentialHalving::SetAction - cannot find best_action_id.");
				}

				DynaPlex::PolicyComparison comp(std::move(trajectory_costs));
				bool ValueBasedProbability = true;
				int64_t least_action_budget = floor(total_budget / (root_actions.size() * ceil(log(root_actions.size()) / log(static_cast<double>(2)))));
				if (least_action_budget > 1) {
					comp.ComputeZstatistics(best_action_id);
					comp.ComputeProbabilities(ValueBasedProbability);
				}
				else {
					comp.ComputeProbabilities(false);
					sample.z_stat = 0.0;
				}

				double zValueForBestAlternative = 100.0;
				for (int64_t action_id = 0; action_id < root_actions.size(); action_id++) {
					sample.cost_improvement.push_back(comp.mean(action_id, prescribed_action_initial_policy, true) * objective);
					sample.q_hat_vec.push_back(comp.mean(action_id) * objective);
					sample.probabilities.push_back(comp.GetProbability(action_id));
					if (action_id != best_action_id && least_action_budget > 1)
					{
						double zValue = comp.GetZstatistic(action_id);
						zValueForBestAlternative = std::min(zValue, zValueForBestAlternative);
					}
				}
				if (least_action_budget > 1) {
					sample.z_stat = zValueForBestAlternative;
				}
			}
		}
	}

}  // namespace DynaPlex::DCL
