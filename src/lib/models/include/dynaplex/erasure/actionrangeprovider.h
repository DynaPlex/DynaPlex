#pragma once
#include <concepts>
#include <memory>
#include "dynaplex/error.h"
#include "dynaplex/statecategory.h"
#include "erasure_concepts.h"


namespace DynaPlex::Erasure
{
    template<typename t_MDP>
    inline bool IsAllowedAction(const t_MDP& mdp, const typename t_MDP::State& state, int64_t action)
    {
        if constexpr (HasIsAllowedAction<t_MDP>)
        {
            return mdp.IsAllowedAction(state, action);
        }
        return true;
    }

    template<typename t_MDP>
    class ActionRange;  // Forward declaration

    template<typename t_MDP>
    class ActionIterator
    {
        friend class ActionRange<t_MDP>;  
        using State = typename t_MDP::State;
        const t_MDP& mdp;
        const State& state;
        int64_t current_action;
        int64_t max_action;

    public:
        ActionIterator(const t_MDP& mdp, const State& state, int64_t current, int64_t max)
            : mdp(mdp),state(state), current_action(current), max_action(max)
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
            } while (current_action < max_action && !IsAllowedAction<t_MDP>(mdp,state,current_action));
            return *this;
        }

        bool operator!=(const ActionIterator& other) const
        {
            return current_action != other.current_action;
        }
        bool operator==(const ActionIterator& other) const
        {
            return current_action == other.current_action;
        }
    };

    template<typename t_MDP>
    class ActionRange
    {
    public:
        using State = typename t_MDP::State;

        ActionRange(const t_MDP& mdp, const State& state,int64_t min_action, int64_t max_action)
            : mdp(mdp), state(state), max_action(max_action)
        {
            first_action = min_action;
            while (!IsAllowedAction<t_MDP>(mdp, state, first_action))
            {
                ++first_action;
                if (first_action == max_action)
                {
                    std::string extra{};
                    if constexpr (HasGetStateCategory<t_MDP>)
                    {
                        DynaPlex::StateCategory cat=mdp.GetStateCategory(state);
                        if (!cat.IsAwaitAction())
                        {
                            extra += "\nNOTE: State is not AwaitAction.";
                        }
                    }
                    if constexpr (DynaPlex::Concepts::ConvertibleToVarGroup<State>)
                    {
                        std::string s = "";
                        try
                        {
                            s = state.ToVarGroup().ToAbbrvString();
                        }
                        catch (...) {
                            s = "";
                        };
                        extra += "\n" + s;
                    }
                    throw DynaPlex::Error("ActionRange: State does not have a single valid action."+extra);
                }
            }        
        }

        ActionIterator<t_MDP> begin() const
        {

            return { mdp, state, first_action, max_action };
        }

        ActionIterator<t_MDP> end() const
        {
            return { mdp, state, max_action, max_action };
        }

        int64_t Count() const
        {
            int64_t count = 0;
            for (auto it = begin(); it != end(); ++it)
            {
                count++;
            }
            return count;
        }

    private:
        const t_MDP& mdp;
        const State& state;
        int64_t max_action;
        int64_t first_action;
    };


    //provides range-based access to the allowed actions for a state in an MDP.
    template<typename t_MDP>
    class ActionRangeProvider
    {
    public:
        ActionRangeProvider(std::shared_ptr<const t_MDP> mdp)
            : mdp(mdp)
        {
            try {
                auto vars = mdp->GetStaticInfo();
                //may become non-zero later
                min_action = 0;
                vars.Get("valid_actions", max_action);
            }
            catch (const DynaPlex::Error& e) {
                // Catch the error, append or modify the message, and rethrow
                throw DynaPlex::Error(std::string("Error in MDPAdapter::ActionRangeProvider: ") + e.what());
            }           
        }

        int64_t NumValidActions() const {
            return max_action - min_action;
        }

        bool IsAllowedAction(const typename t_MDP::State& state, int64_t action) const
        {
            return DynaPlex::Erasure::IsAllowedAction<t_MDP>(*mdp, state, action);
        }
        
        ActionRange<t_MDP> operator()(const typename t_MDP::State& state) const
        {
            return ActionRange<t_MDP>{ *(mdp.get()), state, min_action, max_action };
        }

    private:
        std::shared_ptr<const t_MDP> mdp;

        int64_t min_action;
        int64_t max_action;
    };
}
