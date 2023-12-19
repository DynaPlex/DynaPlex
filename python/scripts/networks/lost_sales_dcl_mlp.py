from typing import Dict

import torch
from torch.nn import Linear, ReLU, Sequential, LayerNorm


class ActorMLP(torch.nn.Module):
    def __init__(self, input_dim, output_dim, hidden_dim, min_val=torch.finfo(torch.float).min, activation=ReLU):
        super(ActorMLP, self).__init__()
        self.min_val = min_val
        self.output_dim = output_dim

        self.actor = Sequential(
            Linear(input_dim, hidden_dim),
            LayerNorm(hidden_dim),
            activation(),
            Linear(hidden_dim, hidden_dim),
            activation(),
            Linear(hidden_dim, hidden_dim),
            activation(),
            Linear(hidden_dim, output_dim)
        )

    def forward(self, observations: Dict[str, torch.Tensor]) -> torch.Tensor:

        x = observations['obs']
        x = self.actor(x)

        # If we are in inference mode, mask is optional:
        if observations.get('mask') is not None:
            action_masks = observations['mask']
            x[~action_masks] = self.min_val

        # Since mask is not needed in inference mode, the `input_type` of this network should be `dict', i.e.
        # this key and value must be included in the json when saving.
        # If mask were mandatory, e.g. if it is needed to make inference faster by avoiding the forwarding of certain nodes,
        # Then `input_type' should be `dict_with_mask'.
        return x
