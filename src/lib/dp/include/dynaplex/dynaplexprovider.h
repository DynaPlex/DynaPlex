#pragma once
#include "dynaplex/vargroup.h"
#include "dynaplex/models/registrationmanager.h"
#include "dynaplex/registry.h"
#include "dynaplex/systeminfo.h"  // Include SystemInfo header

namespace DynaPlex {

    class DynaPlexProvider {
    public:
        /// provides access the single instance of the class
        static DynaPlexProvider& get();

        /// gets an MDP based on the vargroup 
        MDP GetMDP(const VarGroup& vars);
        /// lists the MDPs available. 
        VarGroup ListMDPs();

        // If you want to expose SystemInfo to users:
        SystemInfo& getSystemInfo();

    private:
        DynaPlexProvider(); 
        ~DynaPlexProvider();
        // Delete the copy and assignment constructors to ensure singleton behavior
        DynaPlexProvider(const DynaPlexProvider&) = delete;
        DynaPlexProvider& operator=(const DynaPlexProvider&) = delete;

        Registry m_registry;          // private instance of Registry
        SystemInfo m_systemInfo;      // private instance of SystemInfo
    };

}  // namespace DynaPlex
