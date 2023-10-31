#include "dynaplex/dcl.h"
#include "dynaplex/parallel_execute.h"
#include "dynaplex/policytrainer.h"


namespace DynaPlex::Algorithms {

	using sample_vec = std::vector<DynaPlex::NN::Sample>;


	DCL::DCL(const DynaPlex::System& system, DynaPlex::MDP mdp, const VarGroup& config, DynaPlex::Policy policy_0)
		: system{ system }, mdp{ mdp }, policy_0{ nullptr }
	{
		if (!mdp)
			throw DynaPlex::Error("DCL: mdp should not be null");
		//default to 24*60*60=86400 seconds/24 hours:
		config.GetOrDefault("sampling_time_out", sampling_time_out, 86400);
		if (mdp->IsInfiniteHorizon())
			config.GetOrDefault("H", H, 40);
		else
			config.GetOrDefault("H", H, 256);

		config.GetOrDefault("silent", silent, false);
		config.GetOrDefault("retrain_lastgen_only", retrain_lastgen_only, false);
		config.GetOrDefault("M", M, 1000);
		config.GetOrDefault("N", N, 5000);
		if (N<1 || N > static_cast<int64_t>(std::numeric_limits<int32_t>::max() - 1))
			throw DynaPlex::Error("Value of N is invalid: " + std::to_string(N) + ". Must be positive and smaller than 2147483646");
		config.GetOrDefault("num_gens", num_gens, 1);


		config.GetOrDefault("json_save_format", json_save_format, -1);
		int64_t rng_seed_base;
		config.GetOrDefault("rng_seed", rng_seed_base, 14112017);
		rng_seed = RNG::ToSeed(rng_seed_base, "DCL");
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
		//initiate policy_0, defaulting to random. 
		if (policy_0)
			this->policy_0 = policy_0;
		else
			this->policy_0 = mdp->GetPolicy("random");


		VarGroup nn_training;
		if (config.HasKey("nn_training"))
			config.Get("nn_training", nn_training);

		trainer = DynaPlex::NN::PolicyTrainer(system, mdp, nn_training);

		if (config.HasKey("nn_architecture"))
			config.Get("nn_architecture", nn_architecture);
		else
		{
			nn_architecture = DynaPlex::VarGroup{};
			nn_architecture.Add("type", "mlp");
			nn_architecture.Add("hidden_layers", DynaPlex::VarGroup::Int64Vec{});
		}

	}


	void DCL::TrainPolicy() {

		if (retrain_lastgen_only)
		{
			trainer.TrainPolicy(nn_architecture, num_gens, GetPathOfSampleFile(num_gens - 1),silent);
		}
		else
		{
			DynaPlex::Policy policy = policy_0;
			for (int64_t generation = 0; generation < num_gens; generation++) {
				GenerateSamples(policy, generation);
				if(!silent)
					system << "elapsed time: " << system.Elapsed() << std::endl;
				trainer.TrainPolicy(nn_architecture, generation + 1, GetPathOfSampleFile(generation),silent);
		
				policy = trainer.LoadPolicy(nn_architecture, generation + 1);
			}
		}
	}


	std::vector<DynaPlex::Policy> DCL::GetPolicies() {
		std::vector<DynaPlex::Policy> policies;
		for (int64_t generation = 0; generation <= num_gens; generation++)
		{
			policies.push_back(GetPolicy(generation));
		}
		return policies;
	}

	DynaPlex::Policy DCL::GetPolicy(int64_t generation)
	{
		if (generation == -1)
			generation = num_gens;

		if (generation < 0 || generation>num_gens)
			throw DynaPlex::Error("Invalid generation requested: " + std::to_string(generation) + "/" + std::to_string(generation));

		if (generation == 0)
			return policy_0;

		return trainer.LoadPolicy(nn_architecture, generation);
	}

	void DCL::GenerateSamplesOnThread(std::span<DynaPlex::NN::Sample> somesamples, DynaPlex::Policy policy, int32_t thread_offset)
	{
		int32_t offset = thread_offset + node_sampling_offset;
		int32_t num_samples_added = 0;
		Trajectory trajectory{ mdp->NumEventRNGs() };
		trajectory.SeedRNGProvider(false, -offset, offset);

	
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
					if (mdp->IsInfiniteHorizon() && trajectory.PeriodCount == reinitiate_counter)
					{
						break;//the while(!final_reached) loop, to start trajectory afresh. 
					}
					auto allowed = mdp->AllowedActions(trajectory.GetState());

					if (allowed.size() == 1)
					{
						trajectory.NextAction = allowed.front();
					}
					else {
					//	if (M > ceil(log(allowed.size()) / log(2))) {
					//		sequentialhalving_action_selector.SetAction(trajectory, offset + num_samples_added);
					//	}
					//	else {
							uniform_action_selector.SetAction(trajectory, offset + num_samples_added);
					//	}
						auto& sample = somesamples[num_samples_added];
						sample.state = trajectory.GetState()->Clone();
						sample.sample_number = offset + num_samples_added;
						sample.action_label = trajectory.NextAction;
						if constexpr (std::atomic<int64_t>::is_always_lock_free)
						{
							total_samples_collected++;
						}
						if (++num_samples_added == somesamples.size())
							break;//the while(!final_reached) loop, to eventually stop executution. 
					}
					mdp->IncorporateAction({ &trajectory,1 });
				}
				else
				{
					if (trajectory.Category.IsFinal())
					{
						final_reached = true;
						if (mdp->IsInfiniteHorizon())
							throw DynaPlex::Error("DCL: GenerateSamplesOnThread - trajectory has Category.IsFinal() but mdp IsInfiniteHorizon(). ");
					}
					else
						if (trajectory.Category.IsAwaitEvent())
							throw DynaPlex::Error("DCL: GenerateSamplesOnThread - trajectory is AwaitEvent after calling mdp->IncorporateUntilAction (without MaxPeriodCount.)");
				}
			}
		}
		return;
	}

	std::string DCL::GetPathOfSampleFile(int64_t generation, int rank)
	{
		std::string filename = "samples_gen" + std::to_string(generation);
		if (rank >= 0) // for specific nodes
			filename += "_node" + std::to_string(rank);
		filename += ".json";
		return this->system.filepath("dcl", this->mdp->Identifier(), filename);
	}

	void DCL::GenerateSamples(DynaPlex::Policy policy, int64_t generation)
	{
		if (!silent)
		{
			system << "Generating " << N << " samples based on policy type: " << policy->TypeIdentifier() << std::endl;
		}

		uniform_action_selector = DynaPlex::DCL::UniformActionSelector(rng_seed,H,M,mdp,policy);
		sequentialhalving_action_selector = DynaPlex::DCL::SequentialHalving(rng_seed, H, M, mdp, policy);
		//Get the samples that must be collected for this specific node 
		auto splits = DynaPlex::Parallel::get_splits(N, system.WorldSize());
		auto& [start_for_node, end_for_node] = splits[system.WorldRank()];

		node_sampling_offset = static_cast<int32_t>(start_for_node);
		//Create space for the samples collected on this node, and collect the samples:
		std::vector<DynaPlex::NN::Sample> sample_vec(end_for_node - start_for_node);
		auto work = [this, &policy](std::span<DynaPlex::NN::Sample> somesamples, int64_t thread_offset) {
			this->GenerateSamplesOnThread(somesamples, policy, thread_offset); };

	
		//for reporting progress:
		DynaPlex::Parallel::ProgressReporter reporter;
		//Default option, used unless we can have an lock_free sample counter.
		reporter = [this](const std::atomic<bool>&) { std::cout << "Collecting samples: progress reporting not possible" << std::endl; };
		//if appropriate, set specific progress reporter. 
		//if constexpr (std::atomic<int64_t>::is_always_lock_free && std::atomic<bool>::is_always_lock_free)
		{
			if (system.WorldRank() == 0)
			{//only enable progress reporting on a single node. 
				total_samples_collected = 0;
				if (system.WorldSize() == 1)
					if (!silent)
						system << "Progress:" << std::endl;
				else
					if (!silent)
						system << "Progress (node 0 only):" << std::endl;
				reporter = [this](const std::atomic<bool>& error_occurred) {
					int64_t chars_printed = 0;
					int64_t max_chars_to_print = 50;
					int64_t num_ms = 1;
					while (!error_occurred && total_samples_collected < N) {
						std::this_thread::sleep_for(std::chrono::milliseconds(num_ms));
						if (num_ms < 1000)
							num_ms *= 4;
						int64_t to_print = (max_chars_to_print * total_samples_collected) / N;
						while ( chars_printed < to_print)
						{
							if (!silent)
								system << '>';
							chars_printed++;
							if (!silent)
								if (chars_printed % 5 == 0)
									system << 2 * chars_printed;
						}
						system << std::flush;
					}
					system << std::endl;
				};
			}
		}

		DynaPlex::Parallel::parallel_compute<DynaPlex::NN::Sample>(sample_vec, work, system.HardwareThreads(), reporter);


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
				throw DynaPlex::Error("Logical error: Sample numbers are not strictly increasing.");
			old_number = sample.sample_number;
		}

		//nodes other than 0 save their samples
		if (system.WorldRank() > 0)
			sample_data.SaveToFile(mdp, GetPathOfSampleFile(generation, system.WorldRank()));
		//wait until saving on all nodes completes. 
		system.AddBarrier();
		//load the collected samples by this and other nodes, and save the combined samples.
		if (system.WorldRank() == 0)
		{//let node 0 do the gathering and saving.
			sample_data.Samples.reserve(N);
			for (size_t rank = 1; rank < system.WorldSize(); rank++)
			{
				sample_data.AddFromFile(mdp, GetPathOfSampleFile(generation, rank));
				system.remove_file(GetPathOfSampleFile(generation, rank));
			}
			sample_data.SaveToFile(mdp, GetPathOfSampleFile(generation), json_save_format);
		}
	}
}