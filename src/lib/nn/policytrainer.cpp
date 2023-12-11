#include "dynaplex/policytrainer.h"
#if DP_TORCH_AVAILABLE
#include <torch/torch.h>
#include "nn_policy.h"
#endif
#include "dynaplex/trainedpolicyprovider.h"
#include "neuralnetworkprovider.h"
#include <algorithm>

namespace DynaPlex::NN {



    std::string PolicyTrainer::PathToPolicy(DynaPlex::VarGroup nn_architecture, int64_t generation) {
        return system.filepath(mdp->Identifier(), "dcl_policy_gen" + std::to_string(generation));
    }

    PolicyTrainer::PolicyTrainer(const DynaPlex::System& system, DynaPlex::MDP mdp, const DynaPlex::VarGroup& training_config) :
        system{ system }, mdp{ mdp }
	{
        training_config.GetOrDefault("mini_batch_size", mini_batch_size, 64);
        training_config.GetOrDefault("early_stopping_patience", early_stopping_patience, 10);
        training_config.GetOrDefault("max_training_epochs", max_training_epochs, 1000);        
        training_config.GetOrDefault("train_based_on_probs", train_based_on_probs, false);
    }
#if DP_TORCH_AVAILABLE
    std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor> prepare_batch(const std::span<DynaPlex::NN::Sample> samples, const DynaPlex::MDP& mdp) {
        int batch_size = samples.size();
        int input_dim = mdp->NumFlatFeatures();
        int output_dim = mdp->NumValidActions();

        torch::Tensor batched_inputs = torch::empty({ batch_size, input_dim }, torch::kFloat32);
        torch::Tensor batched_targets = torch::empty({ batch_size }, torch::kInt64);
        torch::Tensor batched_probs = torch::full({ batch_size, output_dim }, .0f);
        torch::Tensor batched_relative_costs = torch::full({ batch_size, output_dim }, 32.0f);
        torch::Tensor mask = torch::full({ batch_size, output_dim }, 32.0f);

        float* input_data_ptr = batched_inputs.data_ptr<float>();
        int64_t* target_data_ptr = batched_targets.data_ptr<int64_t>();
        float* mask_ptr = mask.data_ptr<float>();
        float* cost_ptr = batched_relative_costs.data_ptr<float>();
        float* probs_ptr = batched_probs.data_ptr<float>();

        for (int idx = 0; idx < batch_size; idx++) {
            const auto& sample = samples[idx];

            std::span<float> span(input_data_ptr + idx * input_dim, input_dim);
            mdp->GetFlatFeatures(sample.state, span);
            target_data_ptr[idx] = sample.action_label;

            std::vector<int64_t> AllowedActions = mdp->AllowedActions(sample.state);
            for (int64_t action : AllowedActions) {
                mask_ptr[idx * output_dim + action] = 0.0f;
                auto it = std::lower_bound(AllowedActions.begin(), AllowedActions.end(), action);
                if (it != AllowedActions.end() && *it == action) {
                    size_t index = it - AllowedActions.begin();
                    cost_ptr[idx * output_dim + action] = sample.cost_improvement[index];
                    probs_ptr[idx * output_dim + action] = sample.probabilities[index];
                }
                else {
                    throw DynaPlex::Error("PolicyTrainer::prepare_batch - cannot find action index.");
                }
            }
        }

        return { batched_inputs, batched_targets, mask, batched_probs, batched_relative_costs };
    }
#endif
    DynaPlex::Policy PolicyTrainer::LoadPolicy(DynaPlex::VarGroup nn_architecture, int64_t generation) {
#if DP_TORCH_AVAILABLE
        return TrainedPolicyProvider::LoadPolicy(mdp, PathToPolicy(nn_architecture, generation));
#else
        throw DynaPlex::Error("PolicyTrainer::LoadPolicy - Torch not available, cannot load policy. To make torch available, set dynaplex_enable_pytorch to true and dynaplex_pytorch_path to an appropriate path, e.g. in CMakeUserPresets.txt ");
#endif
    }
    	
	void PolicyTrainer::TrainPolicy(DynaPlex::VarGroup nn_architecture, int64_t generation, std::string path_to_sample_data, bool silent) {
		NeuralNetworkProvider provider(mdp);
        SampleData data{ mdp };
        data.AddFromFile(mdp, path_to_sample_data);
        if (!silent)
            system << "loaded " << data.Samples.size() << " samples from " << path_to_sample_data << std::endl;

#if DP_TORCH_AVAILABLE
        auto any_module = provider.GetTrainableNN(nn_architecture);
        auto any_module_as_nn_module = any_module.ptr();
        if (!silent)
            system << nn_architecture.Dump() << std::endl;
         // Set up the optimizer (for example, Adam optimizer).
      
        torch::optim::Adam optimizer(any_module_as_nn_module->parameters(), torch::optim::AdamOptions(1e-3).betas({ 0.9,0.999 }).weight_decay(0.0));
            
        int64_t validation_size = std::max(static_cast<int64_t>(0.05 * data.Samples.size()), static_cast<int64_t>(1));
        int64_t training_size = static_cast<int64_t>(data.Samples.size()) - validation_size;
        
        // Ensure we have at least one mini-batch of training data and one sample of test data
        if (training_size < mini_batch_size || training_size < 0) {
            throw DynaPlex::Error("PolicyTrainer::TrainPolicy - Insufficient data samples for training and validation: " + std::to_string(data.Samples.size()));
        }
        // Round the training data down to a multiple of the mini_batch_size
        training_size = (training_size / mini_batch_size) * mini_batch_size;

        DynaPlex::RNG rng{ 26071983 };
        std::shuffle(data.Samples.begin(), data.Samples.end(), rng.gen());
        std::span<DynaPlex::NN::Sample> training_data(data.Samples.begin(), data.Samples.begin() + training_size);
        std::span<DynaPlex::NN::Sample> validation_data(data.Samples.begin() + training_size, data.Samples.end());
        auto [validation_samples, validation_targets, validation_mask, validation_probs, validation_relative_costs] = prepare_batch(validation_data, mdp);

        int64_t num_batches = training_size / mini_batch_size;
        float best_validation_loss = std::numeric_limits<float>::max();
        float best_training_loss = std::numeric_limits<float>::max();
        float best_cost_improvement = std::numeric_limits<float>::max();
        int64_t epoch = 0;
        int64_t epochs_without_improvement = 0;
        float training_loss{ 0.0f };
        float cost_improvement{ 0.0f };
     
        auto start_time = std::chrono::steady_clock::now();

        do {
            std::shuffle(training_data.begin(), training_data.end(), rng.gen());
            float total_training_loss = 0.0;
            for (int64_t batch = 0; batch < num_batches; batch++) {
                optimizer.zero_grad();       
                auto [batched_inputs, batched_targets, mask, batched_probs, _] = prepare_batch({ &training_data[batch * mini_batch_size], static_cast<size_t>(mini_batch_size) }, mdp);

                // Forward pass.
                torch::Tensor output = any_module.forward(batched_inputs) - mask;

                if (!train_based_on_probs)
                {
                    // Calculate the loss.
                    torch::Tensor loss = torch::nll_loss(torch::log_softmax(output, 1), batched_targets);
                    total_training_loss += loss.item<float>();
                    // Backward pass and optimize.
                    loss.backward();
                }
                else {
                    torch::Tensor probs = torch::softmax(output, /*dim=*/1);
                    // Compute cross-entropy loss manually for soft labels.
                    torch::Tensor loss = -batched_probs * torch::log(probs + 1e-8); // Adding epsilon to avoid log(0)
                    loss = loss.sum(1).mean(); // Sum over classes, then average over the batch.
                    total_training_loss += loss.item<float>();
                    // Backward pass.
                    loss.backward();
                }

                optimizer.step();
            }
            float average_training_loss = total_training_loss / num_batches;
            best_training_loss = std::min(average_training_loss, best_training_loss);

            // Disable gradient computation for validation
            torch::NoGradGuard no_grad;

            float current_validation_loss = 0.0;
            torch::Tensor validation_output = any_module.forward(validation_samples) - validation_mask;

            if (!train_based_on_probs){
                torch::Tensor validation_loss = torch::nll_loss(torch::log_softmax(validation_output, 1), validation_targets);
                current_validation_loss = validation_loss.item<float>();
            }
            else {
                torch::Tensor probs = torch::softmax(validation_output, /*dim=*/1);
                // Compute cross-entropy loss manually for soft labels.
                torch::Tensor validation_loss = -validation_probs * torch::log(probs + 1e-8); // Adding epsilon to avoid log(0)
                validation_loss = validation_loss.sum(1).mean(); // Sum over classes, then average over the batch.
                current_validation_loss = validation_loss.item<float>();
            }

            torch::Tensor costs = torch::sum(torch::softmax(validation_output / 0.001, 1) * validation_relative_costs);
            auto relative_cost_improvement= costs.item<float>() / validation_data.size();
            best_cost_improvement = std::min(best_cost_improvement, relative_cost_improvement);

            // Check for improvement in validation loss
            if (current_validation_loss < best_validation_loss) {
                best_validation_loss = current_validation_loss;
                training_loss = average_training_loss;
                cost_improvement = relative_cost_improvement;
                auto saved_model_path = system.filepath(mdp->Identifier(), "temp", "model_weights.pth");
                torch::save(any_module_as_nn_module, saved_model_path); // Save the model weights
                epochs_without_improvement = 0; // Reset counter
            }
            else {
                epochs_without_improvement++;
                if (epochs_without_improvement >= early_stopping_patience) {
                    if (!silent)
                        system << "Early stopping after " << early_stopping_patience << " epochs without improvement." << std::endl;
                    break; // Exit the training loop
                }
            }

            // Reporting losses every 5 epochs
            if (epoch % 5 == 0) {
                auto end_time = std::chrono::steady_clock::now();
                auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

                if (!silent)
                {
                    system << "EPOCHS: " << epoch
                        << " - Training Loss: " << average_training_loss
                        << " (" << (int)(100 * std::exp(-average_training_loss)) << "%)"
                        << " - Validation Loss: " << current_validation_loss
                        << " (" << (int)(100 * std::exp(-current_validation_loss)) << "%)"
                        << " - Cost Imp.: " << relative_cost_improvement
                        << " Time: " << system.Elapsed(elapsed_time)
                        << std::endl;
                }
            }
            epoch++;
        } while (epoch < max_training_epochs && epochs_without_improvement < early_stopping_patience);

        if (!silent) {
            system << "Actual Epochs/Max Epochs: " << epoch << "/" << max_training_epochs
                << " - Best Training Loss: " << best_training_loss
                << " (" << (int)(100 * std::exp(-best_training_loss)) << "%)"
                << " - Best Validation Loss: " << best_validation_loss
                << " (" << (int)(100 * std::exp(-best_validation_loss)) << "%)"
                << " - Best Cost Improvement : " << best_cost_improvement
                << std::endl;
            system << "Saved policy statistics: "
                << " - Training Loss: " << training_loss
                << " (" << (int)(100 * std::exp(-training_loss)) << "%)"
                << " - Validation Loss: " << best_validation_loss
                << " (" << (int)(100 * std::exp(-best_validation_loss)) << "%)"
                << " - Cost Improvement : " << cost_improvement
                << std::endl;
        }


        auto best_weights_path = system.filepath(mdp->Identifier(), "temp", "model_weights.pth");

        auto policy = std::make_shared<NN_Policy>(mdp);
        policy->neural_network = std::make_unique<torch::nn::AnyModule>(provider.GetTrainableNN(nn_architecture));
        auto as_nn_module = policy->neural_network->ptr();
        torch::load(as_nn_module, best_weights_path);
        system.remove_file(best_weights_path);
        
        policy->policy_config = VarGroup{
            {"id","NN_Policy"},
            {"gen",generation},
            {"nn_architecture", nn_architecture},
            {"num_inputs", mdp->NumFlatFeatures()},
            {"num_outputs", mdp->NumValidActions()}
        };

        TrainedPolicyProvider::SavePolicy(policy, PathToPolicy(nn_architecture, generation));

        if (!silent)
            system << "Training finished, total time elapsed: " << system.Elapsed() << std::endl;
#else
		throw DynaPlex::Error("PolicyTrainer::TrainPolicy - Torch not available, cannot train policy. To make torch available, set dynaplex_enable_pytorch to true and dynaplex_pytorch_path to an appropriate path, e.g. in CMakeUserPresets.txt ");
#endif
		
	}

}
