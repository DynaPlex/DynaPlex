#pragma once
#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include "vargroup.h"
#include "mdp.h"

namespace DynaPlex {

    using MDPFactoryFunction = std::function<DynaPlex::MDP(const DynaPlex::VarGroup&)>;
    struct MDPEntry {
        std::string identifier;
        std::string description;
    };
    class Registry {
    public:
        void Register(const std::string& identifier, const std::string& description, MDPFactoryFunction func);

        DynaPlex::MDP GetMDP(const DynaPlex::VarGroup& vars);

        DynaPlex::VarGroup ListMDPs();

    private:
        struct MDPInfo {
            MDPFactoryFunction function;
            std::string description;
        };

        std::unordered_map<std::string, MDPInfo> m_registry;
    };
}
