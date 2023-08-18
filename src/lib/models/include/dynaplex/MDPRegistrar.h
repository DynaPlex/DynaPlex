#include "dynaplex/convert.h"          
#include "dynaplex/registry.h"         
#include "dynaplex/vargroup.h"         

namespace DynaPlex {
    namespace Models {
        template<typename SpecificMDP>
        class MDPRegistrar {
        public:
            MDPRegistrar(const std::string& modelName, const std::string& model_description="") {
                Registry::Register(modelName, model_description, &MDPRegistrar::CreateInstance);
            }

            static DynaPlex::MDP CreateInstance(const VarGroup& vars) {
                return Erasure::Convert(SpecificMDP(vars));
            }
        };
    }
}