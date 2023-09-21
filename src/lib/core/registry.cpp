#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include "dynaplex/registry.h"
#include "dynaplex/error.h"

namespace DynaPlex {

    void Registry::Register(const std::string& identifier, const std::string& description, MDPFactoryFunction func) {
        if (m_registry.find(identifier) != m_registry.end()) {
            throw DynaPlex::Error("Error during construction of MDP Registry. You attempted to register two MDPs with the same identifier: \"" + identifier + "\". Each MDP must be registered under a unique name.");
        }
        m_registry[identifier] = { func, description };
    }

    DynaPlex::MDP Registry::GetMDP(const DynaPlex::VarGroup& config) {
        std::string id;
        config.Get("id", id);

        auto it = m_registry.find(id);
        if (it != m_registry.end()) {
            return it->second.function(config);
        }
        throw DynaPlex::Error("No MDP available with identifier \"" + id + "\". Use ListMDPs() / list_mdps() to obtain available MDPs.");
    }

    DynaPlex::VarGroup Registry::ListMDPs() {
        DynaPlex::VarGroup vars{};

        for (const auto& pair : m_registry) {
            vars.Add(pair.first, pair.second.description);
        }
        vars.SortTopLevel();
        return vars;
    }
}
