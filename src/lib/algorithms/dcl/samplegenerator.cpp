#include "dynaplex/samplegenerator.h"
#include "dynaplex/parallel_execute.h"
#include "dynaplex/policytrainer.h"
#include "dynaplex/sampledata.h"
#include "dynaplex/sample.h"
#include <algorithm>
#include <cmath>
namespace DynaPlex::DCL {


	SampleGenerator::SampleGenerator(const DynaPlex::System& system, DynaPlex::MDP mdp, const VarGroup& config)
		: system{ system }, mdp{ mdp }, node_sampling_offset{}
	{
		if (!mdp)
			throw DynaPlex::Error("SampleGenerator: mdp should not be null");
		//default to 24*60*60=86400 seconds/24 hours:
		config.GetOrDefault("sampling_time_out", sampling_time_out, 86400);
		if (mdp->IsInfiniteHorizon())
			config.GetOrDefault("H", H, 40);
		else
			config.GetOrDefault("H", H, 256);

		config.GetOrDefault("enable_sequential_halving", enable_sequential_halving, true);
		config.GetOrDefault("silent", silent, false);
		config.GetOrDefault("M", M, 1000);
		config.GetOrDefault("N", N, 5000);
		config.GetOrDefault("sampling_probability", sampling_probability, 1.0);
		if (N < 1 || N >= (1ll << 30))
			throw DynaPlex::Error("Value of N is invalid: " + std::to_string(N) + ". Must be positive and smaller than " + std::to_string(1 << 30));


		config.GetOrDefault("json_save_format", json_save_format, -1);
		config.GetOrDefault("rng_seed", rng_seed, 15112017);
		if (rng_seed < 0)
			throw DynaPlex::Error("SampleGenerator :: Invalid rng_seed - should be non-negative");


		if (mdp->IsInfiniteHorizon())
		{
			config.GetOrDefault("L", L, 100);
			config.GetOrDefault("reinitiate_counter", reinitiate_counter, 1048576);
		}
		else
		{//unused
			L = 0;
			reinitiate_counter = 0;
		}
		seed_offset = 0;
	}

	void SampleGenerator::GenerateSamplesOnThread(std::span<DynaPlex::NN::Sample> somesamples, DynaPlex::Policy policy, int64_t thread_offset)
	{
		bool use_seed_offset = true; // setting it true will secure different seeding between generations 
		int64_t seed = use_seed_offset ? seed_offset : 0;
		int64_t offset = thread_offset + node_sampling_offset + 1 + seed;
		int64_t num_samples_added = 0;
		Trajectory trajectory{};
		trajectory.RNGProvider.SeedEventStreams(false, rng_seed, offset);
		DynaPlex::RNG rng(false, rng_seed, offset);

		bool final_reached_once = false;
		while (num_samples_added < somesamples.size())
		{
			mdp->InitiateState({ &trajectory,1 });

			if (mdp->IsInfiniteHorizon())
			{//do a warm-up of L steps. 
				int64_t actual_steps = 0;
				while (trajectory.PeriodCount < L)
				{
					if (mdp->IncorporateUntilAction({ &trajectory,1 }, L))
					{
						mdp->IncorporateAction({ &trajectory,1 }, policy);


						if (actual_steps++ > 10000 * L)
							throw DynaPlex::Error("DCL: GenerateSamplesOnThread - it seems that there are hardly any time-steps in this MDP. Aborting. ");
					}
					else
						if (trajectory.Category.IsFinal())
							throw DynaPlex::Error("DCL: GenerateSamplesOnThread - trajectory has Category.IsFinal() but mdp IsInfiniteHorizon(). ");
				}
			}
			bool final_reached = false;
			while (!final_reached) {
				if (mdp->IncorporateUntilAction({ &trajectory,1 }))
				{
					if (mdp->IsInfiniteHorizon() && trajectory.PeriodCount == reinitiate_counter + L)
					{
						break;//the while(!final_reached) loop, to start trajectory afresh. 
					}
					auto allowed = mdp->AllowedActions(trajectory.GetState());

					if (allowed.size() == 1)
					{
						trajectory.NextAction = allowed.front();
					}
					else {
						if (rng.genUniform() < sampling_probability)
						{
							auto& sample = somesamples[num_samples_added];
							if (enable_sequential_halving && (M > std::ceil(std::log(allowed.size()) / std::log(2)))) {
								sequentialhalving_action_selector.SetAction(trajectory, sample, offset + num_samples_added);
							}
							else {
								uniform_action_selector.SetAction(trajectory, sample, offset + num_samples_added);
							}

							if constexpr (std::atomic<int64_t>::is_always_lock_free)
							{
								(*total_samples_collected.get())++;
							}
							if (++num_samples_added == somesamples.size())
								break;//the while(!final_reached) loop, to eventually stop executution. 
						}
						else
						{
							policy->SetAction({ &trajectory,1 });
						}
					}
					mdp->IncorporateAction({ &trajectory,1 });
				}
				else
				{
					if (trajectory.Category.IsFinal())
					{
						final_reached = true;
						final_reached_once = true;
						if (mdp->IsInfiniteHorizon())
							throw DynaPlex::Error("DCL: GenerateSamplesOnThread - trajectory has Category.IsFinal() but mdp IsInfiniteHorizon(). ");
					}
					else
						if (trajectory.Category.IsAwaitEvent())
							throw DynaPlex::Error("DCL: GenerateSamplesOnThread - trajectory is AwaitEvent after calling mdp->IncorporateUntilAction (without MaxPeriodCount.)");
				}
			}
		}
		if (!silent)
			if (!mdp->IsInfiniteHorizon() && !final_reached_once && thread_offset == 0)
				system << std::endl << "WARNING possible data skew:  sampling collection did not reach the final state even once for this finite horizon MDP" << std::endl;
		return;
	}

	std::string SampleGenerator::GetPathOfTempSampleFile(int rank)
	{
		std::string filename = "samples_node";
		filename += std::to_string(rank) + ".json";
		return system.filepath(mdp->Identifier(), "temp", filename);
	}


	void SampleGenerator::GenerateSamples(DynaPlex::Policy policy, const std::string& path) {

		if (!policy)
			policy = mdp->GetPolicy("random");

		auto temp_path = system.filepath(mdp->Identifier(), "temp", "samples_complete.json");
		GenerateStateSamples(policy, temp_path);
		//Convert to samples that store features instead of the original states.
		if (system.WorldRank() == 0)
		{
			DynaPlex::NN::SampleData data{ mdp };
			data.AddFromFile(mdp, temp_path);
			std::vector<VarGroup> samples;
			samples.reserve(data.Samples.size());
			for (auto& sample : data.Samples)
				samples.push_back(std::move(sample.ToVarGroupWithFeats(mdp)));
			VarGroup samples_with_feats{
				{"samples", samples}
			};
			samples_with_feats.SaveToFile(path);
			system.remove_file(temp_path);
		}
	}




	void SampleGenerator::GenerateStateSamples(DynaPlex::Policy policy, const std::string& path)
	{
		if (!silent)
			system << "Generating " << N << " samples based on policy type: " << policy->TypeIdentifier() << std::endl;

		uniform_action_selector = DynaPlex::DCL::UniformActionSelector(rng_seed, H, M, mdp, policy);
		sequentialhalving_action_selector = DynaPlex::DCL::SequentialHalving(rng_seed, H, M, mdp, policy);
		//Get the samples that must be collected for this specific node 
		auto splits = DynaPlex::Parallel::get_splits(N, system.WorldSize());
		auto& [start_for_node, end_for_node] = splits[system.WorldRank()];

		node_sampling_offset = start_for_node;
		int64_t to_collect_on_node = end_for_node - start_for_node;
		//Create space for the samples collected on this node, and collect the samples:
		std::vector<DynaPlex::NN::Sample> sample_vec(to_collect_on_node);
		auto work = [this, &policy](std::span<DynaPlex::NN::Sample> somesamples, int64_t thread_offset) {
			this->GenerateSamplesOnThread(somesamples, policy, thread_offset); };


		//for reporting progress:
		DynaPlex::Parallel::ProgressReporter reporter;
		//Default option, used unless we can have an lock_free sample counter.
		reporter = [this](const std::atomic<bool>&) {
			if (!silent)
				system << "Collecting samples: progress reporting not possible" << std::endl;
			};
		//if appropriate, set specific progress reporter. 
		if constexpr (std::atomic<int64_t>::is_always_lock_free && std::atomic<bool>::is_always_lock_free)
		{
			total_samples_collected = std::make_shared<std::atomic<int64_t>>(0);
			if (system.WorldRank() == 0)
			{//only enable progress reporting on a single node. 
				if (system.WorldSize() == 1) {
					if (!silent)
						system << "Progress:" << std::endl;
				}
				else
					if (!silent)
						system << "Progress (node 0 only):" << std::endl;

				reporter = [this, to_collect_on_node](const std::atomic<bool>& error_occurred) {
					int64_t chars_printed = 0;
					int64_t max_chars_to_print = 50;
					int64_t num_ms = 1;
					while (!error_occurred && (*total_samples_collected.get()) < to_collect_on_node) {
						std::this_thread::sleep_for(std::chrono::milliseconds(num_ms));
						if (num_ms < 1000)
							num_ms *= 4;
						int64_t to_print = (max_chars_to_print * (*total_samples_collected.get())) / to_collect_on_node;
						while (chars_printed < to_print)
						{
							if (!silent)
								system << '>' << std::flush;
							chars_printed++;
							if (!silent)
								if (chars_printed % 5 == 0)
									system << 2 * chars_printed << std::flush;
						}
						system << std::flush;
					}
					if (!silent)
						system << std::endl;
					};

			}
		}

		DynaPlex::Parallel::parallel_compute<DynaPlex::NN::Sample>(sample_vec, work, system.HardwareThreads(), reporter);
		seed_offset += N;

		//gather all the collected samples over the threads into sample_data.
		DynaPlex::NN::SampleData sample_data{ mdp };
		for (auto& sample : sample_vec)
		{
			if (sample.state)
				sample_data.Samples.push_back(std::move(sample));
		}

		//check that each sample number is bigger than the previous. 
		int64_t old_number = -1;
		for (auto& sample : sample_data.Samples)
		{
			if (sample.sample_number <= old_number)
				throw DynaPlex::Error("Logical error in SampleGenerator: Sample numbers are not strictly increasing.");
			old_number = sample.sample_number;
		}

		//nodes other than 0 save their samples
		if (system.WorldRank() > 0)
			sample_data.SaveToFile(mdp, GetPathOfTempSampleFile(system.WorldRank()));
		//wait until saving on all nodes completes. 
		system.AddBarrier();
		//load the collected samples by this and other nodes, and save the combined samples.
		if (system.WorldRank() == 0)
		{//let node 0 do the gathering and saving.
			sample_data.Samples.reserve(N);
			for (size_t rank = 1; rank < system.WorldSize(); rank++)
			{
				sample_data.AddFromFile(mdp, GetPathOfTempSampleFile(rank));
				system.remove_file(GetPathOfTempSampleFile(rank));
			}
			DynaPlex::RNG rng(false, rng_seed);
			std::shuffle(sample_data.Samples.begin(), sample_data.Samples.end(), rng.gen());
			sample_data.SaveToFile(mdp, path, json_save_format, silent);
		}
		system.AddBarrier();
	}
}