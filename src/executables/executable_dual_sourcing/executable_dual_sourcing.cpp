#include <iostream>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/policy.h"
#include "dynaplex/retrievestate.h"
#include "dynaplex/models/dual_sourcing/mdp.h"
#include <numeric>
using namespace DynaPlex;

int runDCL() {

    for (int64_t k = 0; k < 1; k++)
    {
        std::cout << "new sru" << std::endl;
        for (int64_t i = 0; i < 10; i++)
        {
            std::cout << i << std::endl;
            //initialize
            auto& dp = DynaPlexProvider::Get();
            std::string model_name = "dual_sourcing";
            std::string mdp_config_name = "mdp_configs/mdp_config_"+ std::to_string(k) + ".json";
            //std::string mdp_config_name = "mdp_configs/mdp_config_8.json";
            std::string file_path = dp.System().filepath("mdp_config_examples", model_name, mdp_config_name);
            auto mdp_vars_from_json = DynaPlex::VarGroup::LoadFromFile(file_path);
            mdp_vars_from_json.Add("inst_no", i);
            auto mdp = dp.GetMDP(mdp_vars_from_json);


            //std::string policy_config_name = "policy_configs/policy_config_8.json";
            //std::string policy_file_path = dp.System().filepath("mdp_config_examples", model_name, policy_config_name);
            //VarGroup policy_vars_from_json = VarGroup::LoadFromFile(policy_file_path);
            //auto policy = mdp->GetPolicy(policy_vars_from_json);

            auto policy = mdp->GetPolicy("random");

            //set several DCL parameters
            DynaPlex::VarGroup nn_training{
                   {"early_stopping_patience",15},
                   {"mini_batch_size", 64},
                   {"max_training_epochs", 200},
                   {"train_based_on_probs", false}
            };

            //nn architecture parameters
            DynaPlex::VarGroup nn_architecture{
                    {"type","mlp"},
                    {"hidden_layers",DynaPlex::VarGroup::Int64Vec{64,64,64}}
            };
            //DCL specific parameters
            int64_t num_gens = 5;
            DynaPlex::VarGroup dcl_config{
                //just for illustration, so we collect only little data, so DCL will run fast but will not perform well.
                {"N",2000},//number of samples to be collected (small for debug purpose)
                {"num_gens",num_gens},
                {"M",1200},//number of exogenous scenarios per state-action pair
                {"H", 100},//horizon length of 1 rollout
                {"L", 20},//warmup period length
                {"nn_architecture",nn_architecture},
                {"nn_training",nn_training},
                {"retrain_lastgen_only",false},
                {"reinitiate_counter", 200}//ensures we do not get stuck in weird states
            };

            //storing the settings 
            VarGroup settings{};
            settings.Add("dcl_config", dcl_config);
            settings.Add("nn_architecture", nn_architecture);
            settings.Add("nn_training", nn_training);
            settings.Add("mdp_config_name", mdp_config_name);

            auto filename = dp.System().filepath(mdp->Identifier(), "settings.json");
            settings.SaveToFile(filename);

            try
            {
                //Create a trainer for the mdp, with appropriate configuratoin.
                auto dcl = dp.GetDCL(mdp, policy, dcl_config);
                //this trains the policy, and saves it to disk.
                dcl.TrainPolicy();
                //using a dcl instance that has same parameterization (i.e. same dcl_config, same mdp), we may recover the trained polciies.


                //This gets all trained policy, as well as the initial policy, in a vector:
               // std::string file_path = dp.System().filepath("mdp_config_examples", model_name, mdp_config_name);
                std::string file_path = dp.System().filepath("mdp_config_examples", model_name, mdp_config_name);
                auto policies = dcl.GetPolicies();

                //we load the benchmark here
                std::string policy_config_name = "policy_configs/policy_config_" + std::to_string(k) + ".json";
                //std::string policy_config_name = "policy_configs/policy_config_8.json";
                std::string policy_file_path = dp.System().filepath("mdp_config_examples", model_name, policy_config_name);
                VarGroup policy_vars_from_json = VarGroup::LoadFromFile(policy_file_path);
                policies.push_back(mdp->GetPolicy(policy_vars_from_json));
                //std::vector<DynaPlex::Policy> policies = { mdp->GetPolicy(policy_vars_from_json) };

                for (int64_t bsp = 0; bsp < 10; bsp++)
                {
                    VarGroup policy_config{};
                    policy_config.Add("id", "base_stock");
                    policy_config.Add("orderupto_cm", bsp);
                    policies.push_back(mdp->GetPolicy(policy_config));
                }

                //Compare the various trained policies with the benchmark:
                DynaPlex::VarGroup test_config;
                test_config.Add("warmup_periods", 50);
                test_config.Add("number_of_trajectories", 500);
                test_config.Add("periods_per_trajectory", 250);
                auto comparer = dp.GetPolicyComparer(mdp, test_config);
                auto comparison = comparer.Compare(policies);
                for (auto& VarGroup : comparison)
                {
                    std::cout << VarGroup.Dump() << std::endl;
                }

            }
            catch (const std::exception& e)
            {
                std::cout << "exception: " << e.what() << std::endl;
            }

        }
    }
    return 0;

}


int64_t findPossibleWays(int64_t coins[], int64_t n, int64_t sum)
{
    // If sum is 0 then there is 1 solution
    // (do not include any coin)
    if (sum == 0)
        return 1;

    // If sum is less than 0 then no
    // solution exists
    if (sum < 0)
        return 0;

    // If there are no coins and sum
    // is greater than 0, then no
    // solution exist
    if (n <= 0)
        return 0;

    // count is sum of solutions (i)
    // including coins[n-1] (ii) excluding coins[n-1]
    return findPossibleWays(coins, n - 1, sum)
        + findPossibleWays(coins, n, sum - coins[n - 1]);
}

std::vector<std::vector<int64_t>> actionspace(int64_t batchSizeCM, int64_t sMax)
{
    //decision space size
    int64_t orderOptions[] = { 1,batchSizeCM };
    int64_t n = sizeof(orderOptions) / sizeof(orderOptions[0]);
    int64_t sumDecision = 0;
    for (int64_t sum = 0; sum < 5 + 1; sum++)
    {
        sumDecision += findPossibleWays(orderOptions, n, sum);
    }
    int64_t decisionSpace = sumDecision;

    std::vector<std::vector<int64_t>> actionMatrix = std::vector<std::vector<int64_t>>(decisionSpace, std::vector<int64_t>(2));
    int64_t iter = 0;
    for (int64_t i = 0; i <= 5; i++)
    {
        for (int64_t j = 0; j <= 5; j++)
        {
            if (i * batchSizeCM + j <= 5)
            {
                actionMatrix[iter][0] = i;
                actionMatrix[iter][1] = j;
                iter++;
            }
        }
    }

    return actionMatrix;
}


void PolicyActions()
{
    //initialize
    auto& dp = DynaPlexProvider::Get();
    std::string model_name = "dual_sourcing";
    std::string mdp_config_name = "mdp_configs/mdp_config_3.json";
    std::string file_path = dp.System().filepath("mdp_config_examples", model_name, mdp_config_name);
    auto mdp_vars_from_json = DynaPlex::VarGroup::LoadFromFile(file_path);
    auto mdp = dp.GetMDP(mdp_vars_from_json);

    //get trained policy
    std::string gen = "3";
    auto filename = dp.System().filepath(mdp->Identifier(), "dcl_policy_gen" + gen);
    auto path = dp.System().filepath(filename);
    auto policy = dp.LoadPolicy(mdp, path);

    //we load the benchmark here
    std::string policy_config_name = "policy_configs/policy_config_3.json";
    std::string policy_file_path = dp.System().filepath("mdp_config_examples", model_name, policy_config_name);
    VarGroup policy_vars_from_json = VarGroup::LoadFromFile(policy_file_path);
    //auto policy = mdp->GetPolicy(policy_vars_from_json);

    int64_t batchSizeCM;
    int64_t sMax;
    mdp_vars_from_json.Get("batchSizeCM", batchSizeCM);
    mdp_vars_from_json.Get("sMax", sMax);
    auto actionMatrix = actionspace(batchSizeCM, sMax);

    std::cout << "operatingItems;InventoryPosition;ActionCM;ActionAM" << std::endl;
    for (int i = 0; i < 8; i++)
    {
        for (int j = -9; j < 9; j++)
        {
            DynaPlex::VarGroup stateVars{
               {"cat",StateCategory::AwaitAction().ToVarGroup()},
               {"cmPart.operatingItems",i},
               {"cmPart.onHandStock",std::max(0,j)},
               {"cmPart.InventoryPosition",j},
               {"cmPart.orderQuantities",std::vector<int64_t>{0,0,0,0,0,0,0,0} },
               {"amPart.operatingItems",7-i},
               {"amPart.onHandStock",2},
               {"amPart.InventoryPosition",2},
               {"amPart.orderQuantities",std::vector<int64_t>{0,0,0,0,0,0,0,0}},
               {"lastdemand",0},
               {"inventoryposition",0},
            };
            auto state = mdp->GetState(stateVars);


            std::vector<Trajectory> trajVec{};
            //below two lines are not supported yet in the public version of DynaPlex, sorry, will add later
  /*          trajVec.push_back(std::move(Trajectory(mdp->NumEventRNGs(), 0)));
            trajVec[0].SeedRNGProvider(false, 12, 0);*/
            mdp->InitiateState({ &trajVec[0] ,1 }, state);
            policy->SetAction(trajVec);

            //mdp->IsAllowedAction(state, trajVec[0].NextAction);

            //9 = 1 AM 
            std::cout << i << ";"  << j << ";" << actionMatrix[trajVec[0].NextAction][0] << ";" << actionMatrix[trajVec[0].NextAction][1] << std::endl;
           
        }

    }
    
    return;
}

void EvaluateTrajectory()
{
    try {

        VarGroup kpis{};
        std::string policy_id = "IRA";
        auto& dp = DynaPlexProvider::Get();

        std::vector< int64_t > instance;
        std::vector< double > hCostsCM_;
        std::vector< double > hCostsAM_;
        std::vector< double > bCosts_;
        std::vector< double > mCostsCM_;
        std::vector< double > mCostsAM_;
        std::vector< double > pCostsCM_;
        std::vector< double > pCostsAM_;
        std::vector< double > onHandStockAM_;
        std::vector< double > onHandStockCM_;
        std::vector< double > operatingItemsCM_;
        std::vector< double > operatingItemsAM_;
        std::vector< double > avgAMratio;
        std::vector< double > gammaAMfraction;

        std::vector<int64_t> mdp_id_ = { 15,15,15,15,15,15,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18,18 };
        std::vector<int64_t> mdp_sub_id_ = { 1,2,4,5,16,22,0,1,2,5,8,9,11,14,15,16,17,18,23,24,25,26,27,28,29,32,35,36,38,41,42,43,44,45,50,51,52,53,54,55,56,62,63,65,68,69,70,71,72,74,77,78,79,80,2,5,8,11,14,17,20,23,29,32,35,38,41,44,47,50,56,59,62,65,68,71,59,62};

        for (int64_t i = 0; i < 1; i++)
        {

            std::string model_name = "dual_sourcing";

            int64_t mdp_id = mdp_id_[i];
            int64_t mdp_sub_id = mdp_id_[i];

            std::string mdp_config_name = "mdp_configs/mdp_config_"+std::to_string(mdp_id) + ".json";
            std::string file_path = dp.System().filepath("mdp_config_examples", model_name, mdp_config_name);
            auto mdp_vars_from_json = DynaPlex::VarGroup::LoadFromFile(file_path);
            mdp_vars_from_json.Add("inst_no", mdp_sub_id);
            auto mdp = dp.GetMDP(mdp_vars_from_json);

            using underlying_mdp = DynaPlex::Models::dual_sourcing::MDP;

            std::int64_t max_period_count = 250;
            std::int64_t replications = 1;

            //get trained policy
            // auto policy = dp.LoadPolicy(mdp, path);
            auto p = mdp->GetPolicy("random");
            auto dcl = dp.GetDCL(mdp, p);
            //auto policies = dcl.GetPolicies();

            //auto policy = policies[4];




            //we load the benchmark here
            std::string policy_config_name = "policy_configs/policy_config_" + std::to_string(mdp_id) + ".json";
            std::string policy_file_path = dp.System().filepath("mdp_config_examples", model_name, policy_config_name);
            VarGroup policy_vars_from_json = VarGroup::LoadFromFile(policy_file_path);
            auto policy = mdp->GetPolicy(policy_vars_from_json);

            //bsp
            //VarGroup policy_config{};
            //policy_config.Add("id", "base_stock");
            //policy_config.Add("orderupto_cm", 7);
            //// auto policy = mdp->GetPolicy(policy_config);
            //policy_config.Get("id", policy_id);

            //random
            //auto policy = mdp->GetPolicy("random");
            //policy_id = "random";

            //I evaluated PPO in a seperate script
            std::vector < int64_t> sMax;
            std::vector<double> piecePriceCM;
            std::vector< double> piecePriceAM;
            std::vector<double> orderCostsCM;
            std::vector<double> holdingCosts;
            std::vector< double> backorderCosts;
            std::vector<double >meanFailureAM;
            std::vector<double >meanFailureCM;
            std::vector < int64_t> InstalledBase;
            std::vector<double> maintenanceCosts;
            mdp_vars_from_json.Get("sMax", sMax);
            mdp_vars_from_json.Get("piecePriceCM", piecePriceCM);
            mdp_vars_from_json.Get("piecePriceAM", piecePriceAM);
            mdp_vars_from_json.Get("orderCostsCM", orderCostsCM);
            mdp_vars_from_json.Get("holdingCosts", holdingCosts);
            mdp_vars_from_json.Get("backorderCosts", backorderCosts);
            mdp_vars_from_json.Get("meanFailureAM", meanFailureAM);
            mdp_vars_from_json.Get("meanFailureCM", meanFailureCM);
            mdp_vars_from_json.Get("InstalledBase", InstalledBase);
            mdp_vars_from_json.Get("maintenanceCosts", maintenanceCosts);
            auto actionMatrix = actionspace(1, sMax[i]);


            std::vector<double> pCostsCM{};
            std::vector<double> pCostsAM{};
            std::vector<double> hCostsCM{};
            std::vector<double> hCostsAM{};
            std::vector<double> bCosts{};
            std::vector<double> mCostsCM{};
            std::vector<double> mCostsAM{};
            std::vector<int64_t> onHandStockCM{};
            std::vector<int64_t> onHandStockAM{};
            std::vector<int64_t> operatingItemsCM{};
            std::vector<int64_t> operatingItemsAM{};


            for (int64_t i = 0; i < replications; i++)
            {
                pCostsCM.push_back(0.0);
                pCostsAM.push_back(0.0);
                hCostsCM.push_back(0.0);
                hCostsAM.push_back(0.0);
                bCosts.push_back(0.0);
                mCostsCM.push_back(0.0);
                mCostsAM.push_back(0.0);
                onHandStockCM.push_back(0);
                onHandStockAM.push_back(0);
                operatingItemsCM.push_back(0);
                operatingItemsAM.push_back(0);

                std::int64_t rng_seed = 11112014 + i;

                //below is not supported yet in public DynaPlex version
                /*Trajectory trajectory{ mdp->NumEventRNGs() };
                trajectory.SeedRNGProvider(true, rng_seed);*/
                //mdp->InitiateState({ &trajectory,1 });
                //while (trajectory.PeriodCount < max_period_count)
                //{

                //    auto& cat = trajectory.Category;
                //    auto& state = DynaPlex::RetrieveState<underlying_mdp::State>(trajectory.GetState());

                //    if (cat.IsAwaitEvent()) {
                //        mdp->IncorporateEvent({ &trajectory,1 });
                //        //track KPIs here

                //        //track costs
                //        hCostsCM.back() += holdingCosts[i] * state.cmPart.onHandStock;
                //        hCostsAM.back() += holdingCosts[i] * state.amPart.onHandStock;

                //        int64_t Backorders = InstalledBase[i] - state.cmPart.operatingItems - state.amPart.operatingItems;
                //        bCosts.back() += backorderCosts[i] * std::max((meanFailureCM[i] * (double)state.cmPart.operatingItems + meanFailureAM[i] * (double)state.amPart.operatingItems) + (double)Backorders - (double)state.cmPart.onHandStock - (double)state.amPart.onHandStock, (double)0.0);

                //        mCostsCM.back() += maintenanceCosts[i] * meanFailureCM[i] * state.cmPart.operatingItems;
                //        mCostsAM.back() += maintenanceCosts[i] * meanFailureAM[i] * state.amPart.operatingItems;

                //        //track other KPIs
                //        onHandStockCM.back() += state.cmPart.onHandStock;
                //        onHandStockAM.back() += state.amPart.onHandStock;
                //        operatingItemsCM.back() += state.cmPart.operatingItems;
                //        operatingItemsAM.back() += state.amPart.operatingItems;

                //    }
                //    else if (cat.IsAwaitAction()) {
                //        policy->SetAction({ &trajectory,1 });
                //        //track KPIs here
                //        mdp->IncorporateAction({ &trajectory,1 });

                //        auto orderCM = actionMatrix[trajectory.NextAction][0];
                //        auto orderAM = actionMatrix[trajectory.NextAction][1];

                //        if (orderCM > 0)
                //        {//fixed costs for ordering CM
                //            pCostsCM.back() += orderCostsCM[i] + piecePriceCM[i] * orderCM;
                //        }
                //        pCostsAM.back() += orderCostsCM[i] + orderAM * piecePriceAM[i];


                //    }
                //}
            }


            //just an example of storing the variable after evaluation
            instance.push_back(i);
            hCostsCM_.push_back(std::accumulate(hCostsCM.begin(), hCostsCM.end(), 0.0) / hCostsCM.size() / max_period_count);
            hCostsAM_.push_back(std::accumulate(hCostsAM.begin(), hCostsAM.end(), 0.0) / hCostsAM.size() / max_period_count);
            bCosts_.push_back(std::accumulate(bCosts.begin(), bCosts.end(), 0.0) / bCosts.size() / max_period_count);
            mCostsCM_.push_back(std::accumulate(mCostsCM.begin(), mCostsCM.end(), 0.0) / mCostsCM.size() / max_period_count);
            mCostsAM_.push_back(std::accumulate(mCostsAM.begin(), mCostsAM.end(), 0.0) / mCostsAM.size() / max_period_count);
            pCostsCM_.push_back(std::accumulate(pCostsCM.begin(), pCostsCM.end(), 0.0) / pCostsCM.size() / max_period_count);
            pCostsAM_.push_back(std::accumulate(pCostsAM.begin(), pCostsAM.end(), 0.0) / pCostsAM.size() / max_period_count);
            onHandStockCM_.push_back(std::accumulate(onHandStockCM.begin(), onHandStockCM.end(), 0.0) / onHandStockCM.size() / max_period_count);
            onHandStockAM_.push_back(std::accumulate(onHandStockAM.begin(), onHandStockAM.end(), 0.0) / onHandStockAM.size() / max_period_count);
            operatingItemsCM_.push_back(std::accumulate(operatingItemsCM.begin(), operatingItemsCM.end(), 0.0) / operatingItemsCM.size() / max_period_count);
            operatingItemsAM_.push_back(std::accumulate(operatingItemsAM.begin(), operatingItemsAM.end(), 0.0) / operatingItemsAM.size() / max_period_count);
            double avgAMratio_ = (std::accumulate(operatingItemsAM.begin(), operatingItemsAM.end(), 0.0) / operatingItemsAM.size() / max_period_count) / ((std::accumulate(operatingItemsAM.begin(), operatingItemsAM.end(), 0.0) / operatingItemsAM.size() / max_period_count) + (std::accumulate(operatingItemsCM.begin(), operatingItemsCM.end(), 0.0) / operatingItemsCM.size() / max_period_count));
            avgAMratio.push_back(avgAMratio_);
            gammaAMfraction.push_back(((avgAMratio_ * meanFailureCM[i]) / (((1.0 - avgAMratio_) * meanFailureAM[i]) + (avgAMratio_ * meanFailureCM[i]))));

        }

        kpis.Add("Instance", instance);
        kpis.Add("hCostsCM", hCostsCM_);
        kpis.Add("hCostsAM", hCostsAM_);
        kpis.Add("bCosts", bCosts_);
        kpis.Add("mCostsCM", mCostsCM_);
        kpis.Add("mCostsAM", mCostsAM_);
        kpis.Add("pCostsCM", pCostsCM_);
        kpis.Add("pCostsAM", pCostsAM_);
        kpis.Add("onHandStockCM", onHandStockCM_);
        kpis.Add("onHandStockAM", onHandStockAM_);
        kpis.Add("operatingItemsCM", operatingItemsCM_);
        kpis.Add("operatingItemsAM", operatingItemsAM_);
        kpis.Add("avgAMratio", avgAMratio);
        kpis.Add("gammaAMfraction", gammaAMfraction);
        auto file = dp.System().filepath("dual_sourcing", "Evaluation", "kpi_" + policy_id + ".json");
       /* std::string gen = "5";
        auto file = dp.System().filepath("dual_sourcing", "Evaluation", "kpi_" + policy_id + gen + "periods" + ".json");*/
        kpis.SaveToFile(file);
            
    }
    catch (const DynaPlex::Error& e)
    {
        std::cout << e.what() << std::endl;
    }


}

void EvaluateBaseStockPolicy()
{
    //initialize
    auto& dp = DynaPlexProvider::Get();
    std::string model_name = "dual_sourcing";
    for (int64_t i = 3; i < 4; i++)
    {

        std::string mdp_config_name = "mdp_configs/mdp_config_" + std::to_string(i) + ".json";
        std::string file_path = dp.System().filepath("mdp_config_examples", model_name, mdp_config_name);
        auto mdp_vars_from_json = DynaPlex::VarGroup::LoadFromFile(file_path);
        auto mdp = dp.GetMDP(mdp_vars_from_json);

        std::vector<DynaPlex::Policy> policies;
        for (int64_t i = 0; i < 7 + 1; i++)
        {
            VarGroup policy_config{};
            policy_config.Add("id", "base_stock");
            policy_config.Add("orderupto_cm", i);
            policies.push_back(mdp->GetPolicy(policy_config));
        }

        std::string policy_config_name = "policy_configs/policy_config_" + std::to_string(i) + ".json";
        std::string policy_file_path = dp.System().filepath("mdp_config_examples", model_name, policy_config_name);
        VarGroup policy_vars_from_json = VarGroup::LoadFromFile(policy_file_path);
        auto policy = mdp->GetPolicy(policy_vars_from_json);
        std::string policy_id;
        policy_vars_from_json.Get("id", policy_id);
        policies.push_back(mdp->GetPolicy(policy_vars_from_json));

        policies.push_back(mdp->GetPolicy("random"));

        //Optional, you can also rely on defaults:
        auto dp_config = VarGroup{
            {"number_of_trajectories",200},
            {"periods_per_trajectory",10000},
        };


        auto comparer = dp.GetPolicyComparer(mdp, dp_config);
        auto results = comparer.Compare(policies);

        for (auto res : results)
        {
            std::cout << res.Dump() << std::endl;
        }
    }
}


int main() {

    //trains the DCL algorithm
    //runDCL();

    //several KPIs to track
    //EvaluateTrajectory();

    //evaluates the action taken by some policy, given some state
    //PolicyActions();

    //Compare against a sigle source base stock policy
    //EvaluateBaseStockPolicy();

}