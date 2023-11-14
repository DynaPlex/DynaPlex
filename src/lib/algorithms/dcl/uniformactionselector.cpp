#include "dynaplex/uniformactionselector.h"
#include "dynaplex/trajectory.h"
#include "dynaplex/parallel_execute.h"
#include "dynaplex/policycomparison.h"
namespace DynaPlex::DCL {


	struct experiment_info
	{
		int64_t action_id;
		int64_t experiment_number;
	};

	UniformActionSelector::UniformActionSelector(uint32_t rng_seed, int64_t H, int64_t M, DynaPlex::MDP& mdp, DynaPlex::Policy& policy)
		: rng_seed{ rng_seed }, H{ H }, M{ M }, mdp{ mdp }, policy{ policy }
	{

	}

	bool adopt_crn = true;
	int64_t max_chunk_size = 256;
	int64_t max_steps_until_completion_expected = 1000000;
	void UniformActionSelector::SetAction(DynaPlex::Trajectory& traj, DynaPlex::NN::Sample& sample, const int32_t seed) const
	{
		if (!traj.Category.IsAwaitAction())
			throw DynaPlex::Error("UniformActionSelector::SetAction - called for trajectory which is not await_action.");

		auto root_state = traj.GetState()->Clone();
		auto root_actions = mdp->AllowedActions(root_state);

		policy->SetAction({ &traj,1 });
		auto prescribed_action_initial_policy = traj.NextAction;
		auto it = std::lower_bound(root_actions.begin(), root_actions.end(), prescribed_action_initial_policy);
		if (it != root_actions.end() && *it == prescribed_action_initial_policy) {
			prescribed_action_initial_policy = it - root_actions.begin();
		}

		if (root_actions.size() <= 1)
			throw DynaPlex::Error("UniformActionSelector::SetAction - called for state with only single<=1 allowed actions.");

		std::vector<experiment_info> experiment_information{};
		std::vector<DynaPlex::Trajectory> trajectories{};
		experiment_information.reserve(root_actions.size() * M);
		trajectories.reserve(root_actions.size() * M);

		//Create M replications for each root_action, with appropriate random seed.
		for (int64_t replication = 0; replication < M; replication++)
		{
			for (int64_t action_id = 0; action_id < root_actions.size(); action_id++)
			{
				auto root_action = root_actions[action_id];
				trajectories.emplace_back(mdp->NumEventRNGs(), experiment_information.size());
				int64_t traj_seed = adopt_crn ? replication : experiment_information.size() + 1;
				trajectories.back().SeedRNGProvider(false, traj_seed, seed ^ rng_seed);
				trajectories.back().NextAction = root_action;
				experiment_information.emplace_back(action_id, replication);
			}
		}

		//iterate over chunks of the trajectories list, and process those for H steps or until final state. 
		auto chunks = DynaPlex::Parallel::get_chunks(trajectories.size(), max_chunk_size);
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
					if (++count > max_steps_until_completion_expected)
						throw DynaPlex::Error("UniformActionSelector::SetAction"
							"- expected completion of simulation run after max_steps_until_completion_expected: "
							+ std::to_string(max_steps_until_completion_expected) +
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
						throw DynaPlex::Error("UniformActionSelector::SetAction - state has Final Category but MDP is infinite horizon()");
					if (traj.PeriodCount != H)
						throw DynaPlex::Error("UniformActionSelector::SetAction - unexpected value of PeriodCount after rollout");
				}
			}
			else
			{
				for (auto& traj : span)
					if (!(traj.Category.IsFinal() || traj.PeriodCount == H))
						throw DynaPlex::Error("UniformActionSelector::SetAction - unexpected trajectory status after rollout");
			}
			//freeing resources
			for (auto& traj : span)
				traj.DeleteState();
		}
		std::vector<std::vector<double>> return_results(root_actions.size(), std::vector<double>(M, 0.0));
		double objective = mdp->Objective(root_state);

		//Collect results and draw conclusion:
		for (auto& traj : trajectories)
		{
			//Note that trajectories were possibly reshuffled; recover experiment information safely:
			auto& info = experiment_information[traj.ExternalIndex];
			//Since we did not implement sequential halving, we have results for every action and every replication. 
			return_results.at(info.action_id).at(info.experiment_number) = traj.CumulativeReturn * objective;
		}

		DynaPlex::PolicyComparison comp(std::move(return_results));
		//find arg_max, which because of objective will correspond to minimum or maximum costs as appropriate
		double best_reward = -std::numeric_limits<double>::infinity();
		int64_t best_action = 0;
		int64_t best_action_id = 0;
		for (int64_t action_id = 0; action_id < root_actions.size(); action_id++)
		{
			if (comp.mean(action_id) > best_reward)
			{
				best_reward = comp.mean(action_id);
				best_action = root_actions.at(action_id);
				best_action_id = action_id;
			}
		}
		if (best_reward == -std::numeric_limits<double>::infinity())
			throw DynaPlex::Error("UniformActionSelector::SetAction - error in logic");

		traj.NextAction = best_action;

		sample.state = traj.GetState()->Clone();
		sample.sample_number = seed;
		sample.action_label = traj.NextAction;
		sample.cost_improvement.reserve(root_actions.size());
		sample.q_hat_vec.reserve(root_actions.size());
		sample.probabilities.reserve(root_actions.size());
		sample.q_hat = best_reward * objective;

		bool ValueBasedProbability = true;
		if (M > 1){
			comp.ComputeZstatistics(best_action_id);
			comp.ComputeProbabilities(ValueBasedProbability);
		}
		else{
			comp.ComputeProbabilities(false);
			sample.z_stat = 0.0;
		}
		double zValueForBestAlternative = 100.0;
		for (int64_t action_id = 0; action_id < root_actions.size(); action_id++) {
			sample.cost_improvement.push_back(comp.mean(action_id, prescribed_action_initial_policy) * objective);
			sample.q_hat_vec.push_back(comp.mean(action_id) * objective);
			sample.probabilities.push_back(comp.GetProbability(action_id));
			if (action_id != best_action_id && M > 1)
			{
				double zValue = comp.GetZstatistic(action_id);
				zValueForBestAlternative = std::min(zValue, zValueForBestAlternative);
			}
		}
		if (M > 1){
			sample.z_stat = zValueForBestAlternative;
		}
	}

}  // namespace DynaPlex::DCL
