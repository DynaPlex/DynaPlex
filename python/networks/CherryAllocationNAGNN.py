import torch
import torch.nn.functional as F
from torch.nn import Linear, ReLU, Sequential, BatchNorm1d, LayerNorm
from torch_geometric.nn import GINConv


class CherryAllocationNAGNN(torch.nn.Module):
    def __init__(self, input_dim: int, hidden_dim: int, n_layers: int,
                 gridsize: int, dist_matrix: torch.Tensor, min_val: float):

        super().__init__()

        self.gridsize = gridsize
        self.num_features = input_dim

        self.dist_matrix = dist_matrix
        self.edge_index_adj = torch.tensor([])

        self.n_layers = n_layers

        self.forward_time = 0.0
        self.reshape_time = 0.0
        self.reshape_mask_time = 0.0
        self.reshape_idx_time = 0.0
        self.reshape_edge_feats_time = 0.0
        self.reshape_node_feats_time = 0.0

        self.num_edges = 0

        self.min_val = min_val

        self.convs = torch.nn.ModuleList()
        self.econvs = torch.nn.ModuleList()

        for _ in range(n_layers):
            mlp = Sequential(
                Linear(input_dim, hidden_dim),
                LayerNorm(hidden_dim),
                ReLU(),
                # Linear(hidden_channels, hidden_channels),
            )
            # fully connected graph contains self-loops:
            # eps=-1 avoids considering nodes' features twice as they are already in the neighborhood
            # self.convs.append(GINConv(mlp, eps=-1).jittable())
            self.convs.append(GINConv(mlp, eps=-1, node_dim=1).jittable())
            input_dim = hidden_dim

        self.mid_dim = self.num_features + n_layers * hidden_dim
        self.lin1 = Linear(self.mid_dim, 2 * hidden_dim)
        self.batch_norm = BatchNorm1d(2 * hidden_dim)
        self.relu = ReLU()
        self.lin2 = Linear(2 * hidden_dim, 1)

    def forward(self, observations: torch.Tensor) -> torch.Tensor:

        num_nodes = self.gridsize * self.gridsize
        batch_size = observations.shape[0]

        x = observations[:, num_nodes:].reshape(-1, num_nodes, self.num_features)

        action_masks = observations[:, :num_nodes].to(torch.bool)

        dist_matrix = self.dist_matrix
        dist_matrix = dist_matrix[:-1, :-1]

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

            out = out.reshape(-1, num_nodes)

            return out

        else:
            x = x.reshape(-1, num_nodes)

            return x
