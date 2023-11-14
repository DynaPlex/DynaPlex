import torch
import torch.nn as nn
import torch.nn.functional as F
from torch.nn import Linear, BatchNorm1d, ReLU


class MLPNew(nn.Module):

    def __init__(self, input_dim, hidden_dim, gridsize):
        super().__init__()

        self.gridsize = gridsize
        self.num_features = input_dim

        self.lin1 = Linear(self.num_features, hidden_dim * 2)
        self.lin2 = Linear(hidden_dim * 2, hidden_dim * 2)
        self.batch_norm = BatchNorm1d(hidden_dim * 2)
        self.relu = ReLU()
        self.lin3 = Linear(hidden_dim * 2, 1)

    def forward(self, observations: torch.Tensor) -> torch.Tensor:

        num_nodes = self.gridsize * self.gridsize

        x = observations[:, num_nodes:].reshape(-1, num_nodes, self.num_features)

        x = x.reshape(-1, x.size(2))

        x = self.lin1(x)
        x = self.batch_norm(x)
        x = self.relu(x)
        x = F.dropout(x, p=0.2, training=self.training)
        x = self.lin3(x)

        x = x.reshape(-1, num_nodes)

        return x

