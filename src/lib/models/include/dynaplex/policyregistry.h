#pragma once
#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <memory>
#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include "dynaplex/policyadapter.h"
//#include "dynaplex/mdpadapter.h"

namespace DynaPlex {

    template <typename t_MDP>
    class PolicyRegistry {
        using PolicyFactoryFunction = std::function<DynaPlex::Policy(std::shared_ptr<const t_MDP> mdp, const DynaPlex::VarGroup& vars, const int64_t& int_hash)>;

  //      friend class DynaPlex::Erasure::MDPAdapter<t_MDP>;
    public:
      
        template <typename t_Policy>
        void Register(const std::string& identifier, const std::string& description="") {
            if (registry_.find(identifier) != registry_.end()) {
                std::string error = "A policy with id \"" + identifier + "\" is already registered. ";
                if (identifier == "random" || identifier == "greedy")
                {
                    error += "\nNote: the policy identifier \"" + identifier + "\" is reserved for a standard policy. ";
                }
                throw DynaPlex::Error(error);

            }
            registry_[identifier] = {
                [](std::shared_ptr<const t_MDP> mdp, const DynaPlex::VarGroup& vars, const int64_t& int_hash) {
                    return std::make_shared<DynaPlex::Erasure::PolicyAdapter<t_MDP, t_Policy>>(mdp, vars,int_hash);
                },
                description
            };
        }       
        DynaPlex::VarGroup ListPolicies() const {
            DynaPlex::VarGroup vars;

            for (const auto& pair : registry_) {
                vars.Add(pair.first, pair.second.description);
            }
            vars.SortTopLevel();
            return vars;
        }
      

   // private:
        DynaPlex::Policy GetPolicy(std::shared_ptr<const t_MDP> mdp, const DynaPlex::VarGroup& vars, const int64_t& int_hash) const {
            std::string id;
            vars.Get("id", id);

            auto it = registry_.find(id);
            if (it != registry_.end()) {
                return it->second.function(mdp, vars,int_hash);
            }
            throw DynaPlex::Error("No policy available with identifier \"" + id + "\". Use ListPolicies() to obtain available policies.");
        }

       

        struct PolicyInfo {
            PolicyFactoryFunction function;
            std::string description;
        };

        std::unordered_map<std::string, PolicyInfo> registry_;
    };

} // namespace DynaPlex
