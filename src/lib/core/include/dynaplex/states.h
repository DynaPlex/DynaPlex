#pragma once
#include <memory>
namespace DynaPlex
{
    class StatesInterface {
    public:
        virtual ~StatesInterface() = default;
        virtual std::unique_ptr<StatesInterface> clone() const = 0;
        const int64_t mdp_int_hash;
    protected:
        explicit StatesInterface(int64_t mdp_int_hash) : mdp_int_hash{ mdp_int_hash } {}
    };

    using States = std::unique_ptr<StatesInterface>;
}