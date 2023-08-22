#pragma once
#include <memory>
#include "dynaplex/states.h"


namespace DynaPlex::Erasure
{
    template <typename t_state>
    class StatesAdapter : public StatesInterface {
    private:
        std::vector<t_state> data;

    public:
        StatesAdapter(const std::vector<t_state>& vec, int64_t id)
            : StatesInterface(id), data(vec) {}

        StatesAdapter(std::vector<t_state>&& vec, int64_t id)
            : StatesInterface(id), data(std::move(vec)) {}

        std::unique_ptr<StatesInterface> clone() const override {
            return std::make_unique<StatesAdapter<t_state>>(data, mdp_identifier);
        }

        const std::vector<t_state>& get() const {
            return data;
        }
    };
}