#pragma once
namespace DynaPlex::Erasure::Helpers
{
    template <typename T>
    concept HasIsAllowedAction = requires(const T mdp, const typename T::State state, int64_t action)
    {
        { mdp.IsAllowedAction(state, action) } -> std::same_as<bool>;
    };

    template<typename t_MDP>
    class ActionRange;  // Forward declaration of ActionRange

    template<typename t_MDP>
    class ActionIterator
    {
        friend class ActionRange<t_MDP>;  // Allow ActionRange to directly access private members of ActionIterator
        using State = typename t_MDP::State;
        const t_MDP& mdp_;
        const State& state_;
        int64_t current_action;
        int64_t max_action;

    public:
        ActionIterator(const t_MDP& mdp, const State& state, int64_t current, int64_t max)
            : mdp_(mdp), state_(state), current_action(current), max_action(max)
        {}

        int64_t operator*() const
        {
            return current_action;
        }

        ActionIterator& operator++()
        {
            do
            {
                ++current_action;
            } while (current_action < max_action && !IsAllowedAction(current_action));
            return *this;
        }

        bool operator!=(const ActionIterator& other) const
        {
            return current_action != other.current_action;
        }

    private:
        bool IsAllowedAction(int64_t action) const
        {
            if constexpr (HasIsAllowedAction<t_MDP>)
            {
                return mdp_.IsAllowedAction(state_, action);
            }
            return true;
        }
    };

    template<typename t_MDP>
    class ActionRange
    {
    public:
        using State = typename t_MDP::State;

        ActionRange(const t_MDP& mdp, const State& state, int64_t max_action)
            : mdp_(mdp), state_(state), max_action_(max_action)
        {}

        ActionIterator<t_MDP> begin() const
        {
            return { mdp_, state_, 0, max_action_ };
        }

        ActionIterator<t_MDP> end() const
        {
            return { mdp_, state_, max_action_, max_action_ };
        }

    private:
        const t_MDP& mdp_;
        const State& state_;
        int64_t max_action_;
    };

    template<typename t_MDP>
    class ActionRangeProvider
    {
    public:
        ActionRangeProvider(const t_MDP& mdp, const DynaPlex::VarGroup& vars)
            : mdp_(mdp)
        {
            vars.Get("num_valid_actions", max_action_);
        }

        ActionRange<t_MDP> operator()(const typename t_MDP::State& state) const
        {
            return ActionRange<t_MDP>{ mdp_, state, max_action_ };
        }

    private:
        const t_MDP& mdp_;
        int64_t max_action_;
    };
}
