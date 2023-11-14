#include "gymemulator.h"
#include "dynaplex/mdp.h"



namespace DynaPlex {

    GymEmulator::GymEmulator(DynaPlex::System system, MDP mdp, VarGroup& vars)
        : mdp{ mdp }, trajectory{ mdp->NumEventRNGs()} {
        num_valid_actions = mdp->NumValidActions();
        num_feats = mdp->NumFlatFeatures();
        if (mdp->DiscountFactor() != 1.0)
            throw DynaPlex::Error("GymEmulator - discountfactor of mdp should be 1.0, to avoid surprises. Note that discounting in model-free DRL is typically accounted for in the algorithms rather than the models.");
        
        if (mdp->IsInfiniteHorizon())
        {
            vars.GetOrDefault("num_actions_until_done", num_actions_until_done,0);
        }
        else
        {
            if (vars.HasKey("num_actions_until_done"))
                system << "keyword argument num_actions_until_done ignored: mdp is finite horizon and trajectories last until final state reached.";
            //unused:
            num_actions_until_done = 0;
        }       
        if (vars.HasKey("seed"))
        {
            int64_t seed;
            vars.Get("seed", seed);
            trajectory.SeedRNGProvider(false, seed);
            seeded = true;
        }
    }

    using obs_type = std::tuple<std::vector<float>, std::vector<float>>;


    GymEmulator::obs_type GymEmulator::Reset(VarGroup& vars) {
        // Reset the trajectory
        if (vars.HasKey("seed"))
        {
            int64_t seed;
            vars.Get("seed", seed);
            trajectory.SeedRNGProvider(false, seed);
            seeded = true;
        }
        if (!seeded)
        {
            throw DynaPlex::Error("GymEmulator::Reset - seed was never provided");
        }
        int64_t tries=0;
        do {
            if (++tries > 100) 
                throw DynaPlex::Error("GymEmulator::reset - cannot find initial actions state after "+std::to_string(tries) + " tries.");
            events_since_reset = 0;            
            mdp->InitiateState({ &trajectory,1 });
            while (trajectory.Category.IsAwaitEvent())
                mdp->IncorporateEvent({ &trajectory,1 });
        } while (!trajectory.Category.IsAwaitAction());
        actions_taken_since_reset = 0;

        if (!trajectory.Category.IsAwaitAction())
            throw DynaPlex::Error("GymEmulator::reset - error in logic; state is not awaitaction but it should be.");
 
        last_cumulative_return = trajectory.CumulativeReturn;
        return GetNextObservation();
    }

    std::tuple<GymEmulator::obs_type, double, bool, py::dict> GymEmulator::Step(int64_t action) {
        if (!mdp->IsAllowedAction(trajectory.GetState(), action))
            throw DynaPlex::Error("GymEmulator::step - action is not allowed.");
        if (!seeded)
        {
            throw DynaPlex::Error("GymEmulator::Step - seed was never provided");
        }


        trajectory.NextAction = action;
        mdp->IncorporateAction({ &trajectory,1 });
        actions_taken_since_reset++;

        while (trajectory.Category.IsAwaitEvent())
            mdp->IncorporateEvent({ &trajectory,1 });

        bool done = trajectory.Category.IsFinal();
        //num_actions_until_done
        if (actions_taken_since_reset >= num_actions_until_done && num_actions_until_done != 0)
            done = true;
        
        double additional_return_obtained = trajectory.CumulativeReturn - last_cumulative_return;
        last_cumulative_return = trajectory.CumulativeReturn;
        double as_reward = additional_return_obtained * mdp->Objective();
        py::dict info{};
        return std::make_tuple(std::move(GetNextObservation()), as_reward, done, info);
    }

    DynaPlex::VarGroup GymEmulator::CurrentStateAsObject() const{
        if(!trajectory.HasState())
            throw DynaPlex::Error("GymEmulator::CurrentStateAsVarGroup - cannot convery non-initialized state to vargroup.");
        return std::move(trajectory.GetState()->ToVarGroup());
    }

    void GymEmulator::Close() {}

    int64_t GymEmulator::ActionSpaceSize() const {
        return num_valid_actions;
    }

    int64_t GymEmulator::ObservationSpaceSize() const {
        return num_feats;
    }

    GymEmulator::~GymEmulator() {}

} // namespace DynaPlex
