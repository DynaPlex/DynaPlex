#include "dynaplex/registry.h"
#include "dynaplex/error.h"
#include "iostream"
#include <vector>
#include <algorithm>
#include <sstream>

namespace DynaPlex::Models {

    void Registry::Register(const std::string& identifier, const std::string& description, MDPFactoryFunction func) {
        auto& registry = GetRegistry();
        if (registry.find(identifier) != registry.end()) {
            // Log the error
            std::cerr << "DYNAPLEX WARNING: An MDP with id \"" + identifier + "\" is already registered. Overwriting previous registration.\n";
        }
        registry[identifier] = { func, description };
    }

    DynaPlex::MDP Registry::GetMDP(const DynaPlex::VarGroup& vars) {
        std::string id;
        vars.Get("id", id);

        auto it = GetRegistry().find(id);
        if (it != GetRegistry().end()) {
            return it->second.function(vars);
        }
        throw DynaPlex::Error("No MDP available with identifier \"" + id + "\". Use ListMDPs() / list_mdps() to obtain available MDPs.");

    }


    DynaPlex::VarGroup Registry::ListMDPs() {
        DynaPlex::VarGroup vars{};

        for (const auto& pair : GetRegistry()) {
            vars.Add(pair.first, pair.second.description);
        }
        vars.SortTopLevel();
        return vars;
    }


    std::unordered_map<std::string, Registry::MDPInfo>& Registry::GetRegistry() {
        static std::unordered_map<std::string, MDPInfo> registry;
        return registry;
    }
}
