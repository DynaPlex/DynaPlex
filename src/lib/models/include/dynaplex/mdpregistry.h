#pragma once
#include <unordered_map>
#include <string>
#include <functional>
#include "dynaplex/vargroup.h"
#include "dynaplex/mdp.h"

namespace DynaPlex::Collections {

    using MDPFactoryFunction = std::function<DynaPlex::MDP(const DynaPlex::VarGroup&)>;

    class MDPRegistry {
    public:
        static void Register(const std::string& identifier, MDPFactoryFunction func);

        static DynaPlex::MDP GetMDP(const DynaPlex::VarGroup& vars);

    private:
        static std::unordered_map<std::string, MDPFactoryFunction>& GetRegistry();
    };
}
