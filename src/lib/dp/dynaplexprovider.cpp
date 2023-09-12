#include "dynaplex/dynaplexprovider.h"
namespace DynaPlex {

    DynaPlexProvider::DynaPlexProvider() {
        // Register all the MDPs upon startup.
        Models::RegistrationManager::RegisterAll(m_registry);
    }

    MDP DynaPlexProvider::GetMDP(const VarGroup& vars) {
        return m_registry.GetMDP(vars);
    }

    VarGroup DynaPlexProvider::ListMDPs() {
        return m_registry.ListMDPs();
    }

}  // namespace DynaPlex
