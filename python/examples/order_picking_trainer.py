import json
import sys
import torch
from torch.utils.data import DataLoader, TensorDataset
import numpy as np
import torch.nn as nn
import os

parent_directory = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(parent_directory)
# noinspection PyUnresolvedReferences
from dp.loader import DynaPlex as dp
from networks.MLPNew import MLPNew
from networks.CherryAllocationNAGNN import CherryAllocationNAGNN
from utils.early_stopping import EarlyStopping
sys.path.remove(parent_directory)

# when compiling and running the library - it is important that the python version matches
# the version that the dynaplex library was compiled against.
# print("Current version of Python is ", sys.version)

MAX_EPOCH = 100
num_feature_per_node = 8
num_gens = 2

train_GNN = True

if train_GNN:
    arch = "GINPyGFull"
else:
    arch = "MLP"

# setting device on GPU if available, else CPU
# device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
device = torch.device('cpu')

gridsize = 6
n_pickers = 2
mdp = dp.get_mdp(id="order_picking",
                 grid_size=gridsize,
                 n_pickers=n_pickers,
                 max_orders_per_event=1)

num_valid_actions = mdp.num_valid_actions()

base_policy = mdp.get_policy("random")
sample_generator = dp.get_sample_generator(mdp, N=4000, M=100)
save_filename = 'dcl_python'

def policy_path(gen):
    return dp.filepath(mdp.identifier(), f'{save_filename}_{gen}')


def sample_path(gen):
    return dp.filepath(mdp.identifier(), f'samples_{gen}.json')


for gen in range(0, num_gens):

    if gen > 0:
        policy = dp.load_policy(mdp, policy_path(gen))
    else:
        policy = base_policy

    sample_generator.generate_samples(policy, sample_path(gen))
    save_model_path = policy_path(gen + 1)

    with open(sample_path(gen), 'r') as json_file:
        sample_data = json.load(json_file)['samples']

        # tensor_y = torch.FloatTensor([sample['action_label'] for sample in sample_data])
        tensor_y = torch.LongTensor([sample['action_label'] for sample in sample_data])
        tensor_mask = torch.BoolTensor([sample['allowed_actions'] for sample in sample_data])
        tensor_x = torch.FloatTensor([sample['features'] for sample in sample_data])

        min_val = torch.finfo(tensor_x.dtype).min

        dist_matrix = mdp.get_static_info()['distance_matrix']
        dist_mat_tensor = torch.FloatTensor([dist_vec[f'row_{idx}'] for idx, dist_vec in enumerate(dist_matrix)])

        # define model
        if arch == "GINPyGFull":
            model = CherryAllocationNAGNN(input_dim=num_feature_per_node, hidden_dim=64, n_layers=3,
                                          gridsize=gridsize, dist_matrix=dist_mat_tensor, min_val=min_val)
        else:
            model = MLPNew(input_dim=num_feature_per_node, hidden_dim=64, gridsize=gridsize)

        if device != torch.device('cpu'):
            tensor_mask = tensor_mask.to(device)
            tensor_x = tensor_x.to(device)
            tensor_y = tensor_y.to(device)
            dist_matrix = dist_matrix.to(device)
            model.to(device)  # in place

        # define datasets
        dataset = TensorDataset(tensor_x, tensor_y, tensor_mask)

        # split train and validation
        train_dim = int(0.95 * len(dataset))
        valid_dim = len(dataset) - train_dim

        # Created using indices from train_size to train_size + test_size.
        valid_dataset = torch.utils.data.Subset(dataset, range(train_dim, len(dataset)))

        # Created using indices from 0 to train_size.
        dataset = torch.utils.data.Subset(dataset, range(train_dim))

        # Instantiate two dataloaders
        dataloader = DataLoader(dataset, batch_size=32, shuffle=False,
                                num_workers=0)  # is shuffling useful? probably no
        valid_dataloader = DataLoader(valid_dataset, num_workers=0)

        # define optimizer
        optimizer = torch.optim.Adam(model.parameters(), lr=0.0002, betas=(0.5, 0.999), weight_decay=0.0)

        # define loss function
        loss_function = nn.NLLLoss()

        # Needed for the distribution implementation of DCL
        # loss_function = nn.CrossEntropyLoss()
        log_softmax = nn.LogSoftmax(dim=-1)

        # to track the average training loss per epoch as the model trains
        avg_train_losses = []
        # to track the average validation loss per epoch as the model trains
        avg_valid_losses = []

        # initialize the early_stopping object
        early_stopping = EarlyStopping(patience=15, verbose=True, delta=0.0005)

        reshape_times = []
        forward_times = []
        for ep in range(MAX_EPOCH):

            # prepare lists for train loss and validation loss
            train_losses = []
            valid_losses = []

            print(f'Starting epoch {ep + 1}')

            # Train model
            # Iterate over the DataLoader for training data
            for z, data in enumerate(dataloader, 0):
                # Get inputs
                inputs, targets, data_mask = data

                # Zero the gradients
                optimizer.zero_grad()

                # Perform forward pass
                outputs = model(inputs)

                # apply mask
                masked_outputs = torch.masked_fill(outputs, ~data_mask, min_val)
                log_outputs = log_softmax(masked_outputs)

                # Compute loss
                loss = loss_function(log_outputs, targets)
                # loss = loss_function(masked_outputs, targets)
                # loss = loss_function(outputs, targets)

                # Perform backward pass
                loss.backward()

                # Perform optimization
                optimizer.step()

                # Print statistics
                train_losses.append(loss.item())

            # Validate model

            # Iterate over the DataLoader for training data
            for z, data in enumerate(valid_dataloader, 0):
                # Get inputs
                inputs, targets, data_mask = data

                # Perform forward pass
                outputs = model(inputs)
                # apply mask
                masked_outputs = torch.masked_fill(outputs, ~data_mask, min_val)
                log_outputs = log_softmax(masked_outputs)

                # Compute loss
                loss = loss_function(log_outputs, targets)
                # loss = loss_function(masked_outputs, targets)
                # loss = loss_function(outputs, targets)

                # Print statistics
                valid_losses.append(loss.item())

            # print training/validation statistics
            # calculate average loss over an epoch
            train_loss = np.average(train_losses)
            valid_loss = np.average(valid_losses)
            avg_train_losses.append(train_loss)
            avg_valid_losses.append(valid_loss)

            print_msg = (f'epoch --> {ep + 1} ' +
                         f'train_loss: {train_loss:.5f} ' +
                         f'valid_loss: {valid_loss:.5f}')
            print(print_msg)

            # early_stopping needs the validation loss to check if it has decreased,
            # and if it has, it will make a checkpoint of the current model
            save_model, early_stop = early_stopping(valid_loss, model)

            if save_model:
                json_info = {'gen': gen + 1}
                dp.save_policy(model, json_info, save_model_path, device)
                print(f"Saved model with name {save_model_path} \n")

            if early_stop:
                print("Early stopping")
                break

policies = [base_policy]

for i in range(1, num_gens+1):
    load_path = policy_path(i)
    policy = dp.load_policy(mdp, load_path)
    policies.append(policy)
comparer = dp.get_comparer(mdp, number_of_trajectories=100, periods_per_trajectory=100)
comparison = comparer.compare(policies)
result = [(item['mean']) for item in comparison]

print(result)
