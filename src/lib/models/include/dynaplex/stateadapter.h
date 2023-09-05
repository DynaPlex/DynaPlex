#include "dynaplex/state.h"

namespace DynaPlex::Erasure {

    template <typename State>
    class StateAdapter : public StateBase {

        State state;
        StateAdapter(int64_t hash_value, const State& s)
            : StateBase(hash_value), 
            state(s) 
        {
        }      
        virtual std::unique_ptr<StateBase> Clone() const override {
            return std::make_unique<StateAdapter>(*this);
        }
    };

} 