#include <memory>
#include <stdexcept>
#include "dynaplex/error.h"
#include "erasure/stateadapter.h"  

namespace DynaPlex {

    template <typename t_State>
    t_State& RetrieveState(std::unique_ptr<StateBase>& dp_state) {
        // Attempt to dynamic cast to StateAdapter<t_State>
        DynaPlex::Erasure::StateAdapter<t_State>* adapter = dynamic_cast<DynaPlex::Erasure::StateAdapter<t_State>*>(dp_state.get());

        if (adapter) {
            // If cast is successful, return the state
            return adapter->state;
        }
        else {
            // If cast fails, handle the error (throwing an exception here)
            throw DynaPlex::Error("RetrieveState: Dynamic cast failed, dp_state does not point to a StateAdapter<t_State>. You are trying to convert to a state that is not contained in dp_State.");
        }
    }
}