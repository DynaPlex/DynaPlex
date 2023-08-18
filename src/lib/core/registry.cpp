#include "dynaplex/registry.h"
#include "dynaplex/error.h"
namespace DynaPlex::Models {

    void Registry::Register(const std::string& identifier, MDPFactoryFunction func) {
        GetRegistry()[identifier] = func;
    }

    DynaPlex::MDP Registry::GetMDP(const DynaPlex::VarGroup& vars) {
        std::string id;
        vars.Get_Into("id", id); 

        auto it = GetRegistry().find(id);
        if (it != GetRegistry().end()) {
            return it->second(vars);
        }
        throw DynaPlex::Error("Unknown identifier: " + id);
    }

    std::unordered_map<std::string, MDPFactoryFunction>& Registry::GetRegistry() {
        static std::unordered_map<std::string, MDPFactoryFunction> registry;
        return registry;
    }
}
