#include "dynaplex/dynaplexprovider.h"
#include <iostream>
#ifdef DP_MPI_AVAILABLE
#include <mpi.h>
#endif


namespace DynaPlex {

    // Implementing the Singleton pattern for DynaPlexProvider
    DynaPlexProvider& DynaPlexProvider::get() {
        static DynaPlexProvider instance;  // Guaranteed to be lazy initialized and destroyed correctly
        return instance;
    }

    DynaPlexProvider::DynaPlexProvider() {
        // If MPI is available, initialize it and fetch world details
#ifdef DP_MPI_AVAILABLE
        int mpi_initialized;
        MPI_Initialized(&mpi_initialized);
        if (!mpi_initialized) {
            MPI_Init(nullptr, nullptr);
    }

        int world_rank;
        int world_size;
        MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &world_size);
#else
        int world_rank = 0;  // Default values
        int world_size = 1;
#endif
        m_systemInfo = SystemInfo(world_rank, world_size);

        

#ifdef DP_MPI_AVAILABLE
        std::cout << "DynaPlex: Hardware Threads: " << m_systemInfo.HardwareThreads()
            << ", MPI: Yes, Rank: " << world_rank
            << "/: " << world_size << std::endl;
#else
        std::cout << "DynaPlex: Hardware Threads: " << m_systemInfo.HardwareThreads()
            << ", MPI: No" << std::endl;
#endif
        // Register all the MDPs upon startup.
        Models::RegistrationManager::RegisterAll(m_registry);
    }


    // Destructor
    DynaPlexProvider::~DynaPlexProvider() {
        // If MPI is available, finalize it
#ifdef DP_MPI_AVAILABLE
        int mpi_finalized;
        MPI_Finalized(&mpi_finalized);
        if (!mpi_finalized) {
            MPI_Finalize();
        }
#endif
    }

    MDP DynaPlexProvider::GetMDP(const VarGroup& vars) {
        return m_registry.GetMDP(vars);
    }

    VarGroup DynaPlexProvider::ListMDPs() {
        return m_registry.ListMDPs();
    }

    // If you want to expose SystemInfo to users:
    SystemInfo& DynaPlexProvider::getSystemInfo() {
        return m_systemInfo;
    }

}  // namespace DynaPlex
