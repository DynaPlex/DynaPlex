#include "dynaplex/mdpregistry.h"
#include "dynaplex/error.h"
namespace DynaPlex::Collections {

    void MDPRegistry::Register(const std::string& identifier, MDPFactoryFunction func) {
        GetRegistry()[identifier] = func;
    }

    DynaPlex::MDP MDPRegistry::GetMDP(const DynaPlex::VarGroup& vars) {
        std::string id;
        vars.Get_Into("id", id); 

        auto it = GetRegistry().find(id);
        if (it != GetRegistry().end()) {
            return it->second(vars);
        }
        throw DynaPlex::Error("Unknown identifier: " + id);
    }

    std::unordered_map<std::string, MDPFactoryFunction>& MDPRegistry::GetRegistry() {
        static std::unordered_map<std::string, MDPFactoryFunction> registry;
        return registry;
    }
}
