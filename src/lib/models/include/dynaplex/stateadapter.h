#include "dynaplex/state.h"

namespace DynaPlex::Erasure {

    template <typename t_State>
    class StateAdapter : public StateBase {
    public:
        t_State state;
        StateAdapter(int64_t hash_value, const t_State& s)
            : StateBase(hash_value), 
            state(s) 
        {
        }      
        std::unique_ptr<StateBase> Clone() const override {
            return std::make_unique<StateAdapter>(*this);
        }
    };

} 