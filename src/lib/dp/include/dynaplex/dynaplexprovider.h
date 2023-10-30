#pragma once
#include "dynaplex/vargroup.h"
#include "dynaplex/models/registrationmanager.h"
#include "dynaplex/registry.h"
#include "dynaplex/system.h"
#include "dynaplex/demonstrator.h"
#include "dynaplex/policycomparer.h"
#include "dynaplex/dcl.h"
namespace DynaPlex {
    class DynaPlexProvider {
        
    public:
        /// provides access the single instance of the class
        static DynaPlexProvider& Get();

        /**
         * sets the root directory where a IO_DynaPlex subdirectory will be created, where all 
         * input and output from dynaplex will be nested. 
         */ 
        void SetIORootDirectory(std::string path);
        /// gets an MDP based on the vargroup 
        MDP GetMDP(const VarGroup& config);
        /// lists the MDPs available. 
        VarGroup ListMDPs();

        // If you want to expose System to users:
        const DynaPlex::System& System();


        void SavePolicy(DynaPlex::Policy policy, std::string file_path_without_extension);

        DynaPlex::Policy LoadPolicy(DynaPlex::MDP mdp, std::string file_path_without_extension);
        
        DynaPlex::Algorithms::DCL GetDCL(DynaPlex::MDP mdp, const VarGroup& config = VarGroup{}, DynaPlex::Policy policy = nullptr);

        /**
         * Config may include max_event_count (default:3)
         * it may also include rng_seed (default:0).
         */
        DynaPlex::Utilities::Demonstrator GetDemonstrator(const VarGroup& config = VarGroup{});

        /**
         * Gets a policy evaluator for a specific mdp. A algorithm config may also be provided.
         * Config may include number_of_trajectories (default:4096 for infinite horizon mdps; 16384 for finite horizon mdps).
         * If mdp is infinite horizon, undiscounted: config may include warmup_periods (default: 128), periods_per_trajectory (default: 1024).
         * If mdp is infinite horizon, discounted: config may include periods_per_trajectory (default: 1024).
         * If mdp is finite horizon: config may include max_periods_until_error (default: 16384), this is the maximum number of steps in a trajectory until mdp is expected to terminate by reaching final state.
         * Config may also include rng_seed (default 0).
         */
        DynaPlex::Utilities::PolicyComparer GetPolicyComparer(DynaPlex::MDP mdp, const VarGroup& config = VarGroup{});


    private:
        void AddBarrier();
        DynaPlexProvider(); 
        ~DynaPlexProvider();
        // Delete the copy and assignment constructors to ensure singleton behavior
        DynaPlexProvider(const DynaPlexProvider&) = delete;
        DynaPlexProvider& operator=(const DynaPlexProvider&) = delete;

        Registry m_registry;          // private instance of Registry
        DynaPlex::System m_systemInfo;      // private instance of System
    };

}  // namespace DynaPlex
