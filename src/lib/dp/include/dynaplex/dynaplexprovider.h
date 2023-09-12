#pragma once
#include "dynaplex/vargroup.h"
#include "dynaplex/models/registrationmanager.h"
#include "dynaplex/registry.h"

namespace DynaPlex {

    class DynaPlexProvider {
    public:
        DynaPlexProvider();

        MDP GetMDP(const VarGroup& vars);

        VarGroup ListMDPs();

    private:
        Registry m_registry;  // private instance of Registry
    };

}  // namespace DynaPlex
