#pragma once
#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include "dynaplex/vargroup.h"
#include "dynaplex/mdp.h"

namespace DynaPlex {

    using MDPFactoryFunction = std::function<DynaPlex::MDP(const DynaPlex::VarGroup&)>;
    struct MDPEntry {
        std::string identifier;
        std::string description;
    };
    class Registry {
    public:

        static void Register(const std::string& identifier, const std::string& description, MDPFactoryFunction func);

        static DynaPlex::MDP GetMDP(const DynaPlex::VarGroup& vars);

        static DynaPlex::VarGroup ListMDPs();

    private:
        struct MDPInfo {
            MDPFactoryFunction function;
            std::string description;
        };

        static std::unordered_map<std::string, MDPInfo>& GetRegistry();
    };
}
