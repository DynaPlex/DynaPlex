#include "dynaplex/trajectory.h"

namespace DynaPlex::Erasure {

    template <typename State>
    class TrajectoryAdapter : public TrajectoryBase {
        State state;

    public:
        TrajectoryAdapter() : TrajectoryBase(0) {}

        TrajectoryAdapter(int64_t hash_value, const State& s)
            : TrajectoryBase(hash_value), state(s) {}

        void SetState(const State& new_state) {
            state = new_state;
        }

        virtual std::unique_ptr<TrajectoryBase> Clone() const override {
            return std::make_unique<TrajectoryAdapter>(mdp_int_hash, state);
        }
    };

}  // namespace Erasure