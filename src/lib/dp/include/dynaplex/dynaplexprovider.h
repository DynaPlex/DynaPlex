#pragma once
#include "dynaplex/vargroup.h"
#include "dynaplex/models/registrationmanager.h"
#include "dynaplex/registry.h"
#include "dynaplex/systeminfo.h"  // Include SystemInfo header

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
        MDP GetMDP(const VarGroup& vars);
        /// lists the MDPs available. 
        VarGroup ListMDPs();

        // If you want to expose SystemInfo to users:
        const SystemInfo& getSystemInfo();

    private:
        void AddBarrier();
        DynaPlexProvider(); 
        ~DynaPlexProvider();
        // Delete the copy and assignment constructors to ensure singleton behavior
        DynaPlexProvider(const DynaPlexProvider&) = delete;
        DynaPlexProvider& operator=(const DynaPlexProvider&) = delete;

        Registry m_registry;          // private instance of Registry
        SystemInfo m_systemInfo;      // private instance of SystemInfo
    };

}  // namespace DynaPlex
