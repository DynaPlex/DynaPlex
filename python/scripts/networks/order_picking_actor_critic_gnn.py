from typing import Dict

import torch
import torch.nn.functional as F
from torch.nn import Linear, ReLU, Sequential, BatchNorm1d, LayerNorm
from torch_geometric.data import Data, Batch
from torch_geometric.nn.conv import GINConv
from torch_geometric.nn.pool import global_mean_pool


class NAGNNActor(torch.nn.Module):
    def __init__(self, input_dim: int, hidden_dim: int, n_layers: int,
                 gridsize: int, dist_matrix: torch.Tensor, min_val: float):

        super().__init__()

        self.gridsize = gridsize
        self.num_features = input_dim

        self.dist_matrix = dist_matrix
        self.edge_index_adj = torch.tensor([])

        self.n_layers = n_layers

        self.min_val = min_val

        self.convs = torch.nn.ModuleList()

        for _ in range(n_layers):
            mlp = Sequential(
                Linear(input_dim, hidden_dim),
                LayerNorm(hidden_dim),
                ReLU(),
            )
            # fully connected graph contains self-loops:
            # eps=-1 avoids considering nodes' features twice as they are already in the neighborhood
            self.convs.append(GINConv(mlp, eps=-1, node_dim=1).jittable())
            input_dim = hidden_dim

        self.mid_dim = self.num_features + n_layers * hidden_dim

        self.lin1 = Linear(self.mid_dim, 2 * hidden_dim)
        self.batch_norm = BatchNorm1d(2 * hidden_dim)
        self.relu = ReLU()
        self.lin2 = Linear(2 * hidden_dim, 1)
        self.softmax = torch.nn.Softmax(dim=1)

    def forward(self, observations: Dict[str, torch.Tensor]) -> torch.Tensor:

        num_nodes = self.gridsize * self.gridsize

        x = observations['obs'].reshape(-1, num_nodes, self.num_features)
        batch_size = x.shape[0]

        # The mask is always expected, even in inference mode to apply the decoder to fewer nodes
        action_masks = observations['mask'].to(torch.bool)

        dist_matrix = self.dist_matrix

        # Extracts the sparse edge_index required by torch_geometric from the provided distance matrix
        # Note that the distance matrix can be changed for inference:
        # in that case, self.edge_index is set to empty and re-extracted
        if self.edge_index_adj.numel() == 0:
            adj_matrix = (dist_matrix == 1)
            adj_matrix = adj_matrix.to(torch.int)

            index = adj_matrix.nonzero().transpose(0, 1)
            if len(index) == 3:
                batch = index[0] * adj_matrix.size(-1)
                index_tuple = (batch + index[1], batch + index[2])
            else:
                index_tuple = (index[0], index[1])

            self.edge_index_adj = torch.stack(index_tuple, dim=0)

        x1 = torch.flatten(x, end_dim=1)
        outs = [x1]

        # GIN layers
        for conv in self.convs:
            x = conv(x, self.edge_index_adj).relu()
            outs.append(torch.flatten(x, end_dim=1))

        x = torch.cat(outs, dim=1)

        x = x.reshape(-1, num_nodes, self.mid_dim)

        out = torch.full((batch_size * num_nodes, 1), self.min_val)

        if not self.training:
            x = x[action_masks]

        x = x.reshape(-1, self.mid_dim)

        x = self.lin1(x)
        x = self.batch_norm(x)
        x = self.relu(x)
        x = F.dropout(x, p=0.2, training=self.training)
        x = self.lin2(x)

        if not self.training:
            action_masks = action_masks.flatten()
            out[action_masks] = x
            x = out.reshape(-1, num_nodes)

        else:
            x = x.reshape(-1, num_nodes)
            x[~action_masks] = self.min_val

        x = self.softmax(x)

        return x


class NAGNNCritic(torch.nn.Module):
    def __init__(self, input_dim: int, hidden_dim: int, n_layers: int,
                 gridsize: int, dist_matrix: torch.Tensor, min_val: float):

        super().__init__()

        self.gridsize = gridsize
        self.num_features = input_dim

        self.dist_matrix = dist_matrix
        self.edge_index_adj = torch.tensor([])

        self.n_layers = n_layers

        self.min_val = min_val

        self.convs = torch.nn.ModuleList()

        for _ in range(n_layers):
            mlp = Sequential(
                Linear(input_dim, hidden_dim),
                LayerNorm(hidden_dim),
                ReLU()
            )
            # fully connected graph contains self-loops:
            # eps=-1 avoids considering nodes' features twice as they are already in the neighborhood
            self.convs.append(GINConv(mlp, eps=-1, node_dim=1).jittable())
            input_dim = hidden_dim

        self.mid_dim = self.num_features + n_layers * hidden_dim

        self.lin1 = Linear(self.mid_dim, 2 * hidden_dim)
        self.batch_norm = BatchNorm1d(2 * hidden_dim)
        self.relu = ReLU()
        self.lin2 = Linear(2 * hidden_dim, 1)

    def forward(self, observations: torch.Tensor) -> torch.Tensor:

        num_nodes = self.gridsize * self.gridsize

        x = observations['obs'].reshape(-1, num_nodes, self.num_features)
        batch_size = x.shape[0]

        dist_matrix = self.dist_matrix

        if self.edge_index_adj.numel() == 0:
            adj_matrix = (dist_matrix == 1)
            adj_matrix = adj_matrix.to(torch.int)

            index = adj_matrix.nonzero().transpose(0, 1)
            if len(index) == 3:
                batch = index[0] * adj_matrix.size(-1)
                index_tuple = (batch + index[1], batch + index[2])
            else:
                index_tuple = (index[0], index[1])

            self.edge_index_adj = torch.stack(index_tuple, dim=0)

        # Batch the data apply global_mean_pool later
        data_list = [Data(x=x[index],
                          edge_index=self.edge_index_adj)
                     for index in range(batch_size)]
        batch_data = Batch.from_data_list(data_list)
        batch = batch_data.batch

        x1 = torch.flatten(x, end_dim=1)
        outs = [x1]

        # GIN layers
        for conv in self.convs:
            x = conv(x, self.edge_index_adj).relu()
            outs.append(torch.flatten(x, end_dim=1))

        x = torch.cat(outs, dim=1)

        x = x.reshape(-1, num_nodes, self.mid_dim)

        x = x.reshape(-1, self.mid_dim)

        x = self.lin1(x)
        x = self.batch_norm(x)
        x = self.relu(x)
        x = F.dropout(x, p=0.2, training=self.training)
        x = self.lin2(x)

        x = global_mean_pool(x, batch)

        return x
