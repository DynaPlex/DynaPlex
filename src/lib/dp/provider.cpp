#include "dynaplex/provider.h"
namespace DynaPlex {

    Provider::Provider() {
        // Register all the MDPs upon startup.
        Models::RegistrationManager::RegisterAll(m_registry);
    }

    MDP Provider::GetMDP(const VarGroup& vars) {
        return m_registry.GetMDP(vars);
    }

    VarGroup Provider::ListMDPs() {
        return m_registry.ListMDPs();
    }

}  // namespace DynaPlex
