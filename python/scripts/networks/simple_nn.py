import torch
import torch.nn as nn
import torch.nn.functional as F
from torch.nn import Linear, ReLU


class SimpleNN(nn.Module):

    def __init__(self, input_dim, hidden_dim, output_dim):
        super().__init__()

        self.num_features = input_dim

        self.lin1 = Linear(self.num_features, hidden_dim)
        self.lin2 = Linear(hidden_dim, hidden_dim)
        self.relu = ReLU()
        self.lin3 = Linear(hidden_dim, output_dim)

    def forward(self, observations: torch.Tensor) -> torch.Tensor:

        x = self.lin1(observations)
        x = self.relu(x)
      #  x = F.dropout(x, p=0.2, training=self.training)
        x = self.lin3(x)

        return x

