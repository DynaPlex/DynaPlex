#include <memory>
#include <stdexcept>
#include "dynaplex/error.h"
#include "dynaplex/mdp.h"
#include "dynaplex/erasure/mdpadapter.h"

namespace DynaPlex {

    template <typename t_MDP>
    std::shared_ptr<const t_MDP> RetrieveMDP(MDP& mdp) {
        if (!mdp)
            throw DynaPlex::Error("RetrieveMDP: mdp is null");
        // Attempt to dynamic cast to MDPAdapter<t_MDP>
        DynaPlex::Erasure::MDPAdapter<t_MDP>* adapter = dynamic_cast<DynaPlex::Erasure::MDPAdapter<t_MDP>*>(mdp.get());

        if (adapter) {
            // If cast is successful, return the MDP instance
            return adapter->mdp;
        }
        else {
            //std::cout << "Trying to retrieve: " << typeid(t_MDP).name() << std::endl;
            //std::cout << "Actual type of object: " << typeid(*mdp.get()).name() << std::endl;
            throw DynaPlex::Error("RetrieveMDP: Dynamic cast failed, mdp does not point to a MDPAdapter<t_MDP>. You are trying to convert to an MDP type that is not contained in mdp.");
        }
    }
}
