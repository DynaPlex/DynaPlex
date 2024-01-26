#pragma once
#include "dynaplex/mdp.h"
#include "dynaplex/policy.h"
#include "dynaplex/system.h"
#include "dynaplex/vargroup.h"
#include "dynaplex/uniformactionselector.h"
#include "dynaplex/sequentialhalving.h"

namespace DynaPlex::DCL {
	class SampleGenerator
	{
	public:
		SampleGenerator(const DynaPlex::System&, DynaPlex::MDP, const DynaPlex::VarGroup& config = VarGroup{});

		/// This generates samples and stores the features alognside the collected information.  
		void GenerateSamples(DynaPlex::Policy,const std::string& file_path);
		/// This generates samples and stores the state alongside the collected information 
		void GenerateStateSamples(DynaPlex::Policy,const std::string& file_path);

	private:

		std::string GetPathOfTempSampleFile(int rank);

		void GenerateSamplesOnThread(std::span<DynaPlex::NN::Sample>, DynaPlex::Policy, int64_t);

		//for a progress count when generating samples accross threads. 
		std::shared_ptr<std::atomic<int64_t>> total_samples_collected;

		int64_t rng_seed;
		int64_t node_sampling_offset;
		int64_t sampling_time_out, H, M, N, L, reinitiate_counter, json_save_format;
		int64_t seed_offset;

		bool enable_sequential_halving,silent;
		//probability that a sample is taken on a specific action-awaiting state. 
		double sampling_probability;

		DynaPlex::MDP mdp;
		DynaPlex::System system;
		DynaPlex::DCL::UniformActionSelector uniform_action_selector;
		DynaPlex::DCL::SequentialHalving sequentialhalving_action_selector;

	};
}