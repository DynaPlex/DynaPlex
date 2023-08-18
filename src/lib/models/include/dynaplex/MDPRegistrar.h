#include "dynaplex/convert.h"          
#include "dynaplex/registry.h"         
#include "dynaplex/vargroup.h"         

namespace DynaPlex {
    namespace Models {
        template<typename SpecificMDP, const char* modelName>
        class MDPRegistrar {
        public:
            MDPRegistrar() {
                Registry::Register(modelName, &MDPRegistrar::CreateInstance);
            }

            static DynaPlex::MDP CreateInstance(const VarGroup& vars) {
                return Erasure::Convert(SpecificMDP(vars));
            }
        };
    }
}