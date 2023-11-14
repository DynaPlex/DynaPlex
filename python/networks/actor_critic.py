from typing import Dict, Any, Optional, Tuple, Union

import torch
from torch.nn import Linear, ReLU, Sequential, LayerNorm


class ActorCriticMLP(torch.nn.Module):
    def __init__(self, input_dim, output_dim, hidden_dim, mode, min_val=torch.finfo(torch.float).min, activation=ReLU):
        super(ActorCriticMLP, self).__init__()
        self.mode = mode
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
        self.critic = Sequential(
            Linear(input_dim, hidden_dim),
            LayerNorm(hidden_dim),
            activation(),
            Linear(hidden_dim, hidden_dim),
            activation(),
            Linear(hidden_dim, hidden_dim),
            activation(),
            Linear(hidden_dim, 1)
        )
        self.softmax = torch.nn.Softmax(dim=1)

    def forward(self, observations: Dict[str, torch.Tensor], state=None, info={}):

        #a bit convoluted, there is probably a better way
        action_masks = observations['mask']
        batch_data = observations['obs']

        batch_size = batch_data.shape[0]

        x = torch.tensor(batch_data, dtype=torch.float)
        if self.mode == 'actor':
            x = self.actor(x)
            x[~action_masks] = self.min_val
            x = self.softmax(x)
            return x, state
        elif self.mode == 'critic':
            x = self.critic(x)
            return x
        else:
            raise ValueError(f'Unknown mode: {self.mode}')


class ActorMLP(torch.nn.Module):
    def __init__(self, input_dim, output_dim, hidden_dim, min_val=torch.finfo(torch.float).min, activation=ReLU):
        super(ActorMLP, self).__init__()
        self.min_val = min_val
        self.output_dim = output_dim
        self.dynaplex_eval = False
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
        self.softmax = torch.nn.Softmax(dim=1)

    # def forward(self, observations: Dict[str, torch.Tensor], state: Optional[torch.Tensor] = None, info: Optional[Dict[str, int]] = None) \
    def forward(self, observations: torch.Tensor, state: Optional[torch.Tensor] = None, info: Optional[Dict[str, int]] = None) \
            -> Union[torch.Tensor, Tuple[torch.Tensor, Optional[torch.Tensor]]]:

        #a bit convoluted, there is probably a better way
        # action_masks = observations['mask']
        # batch_data = observations['obs']

        if not self.dynaplex_eval:
            action_masks = observations[:, :self.output_dim].to(torch.bool)
            batch_data = observations[:, self.output_dim:]

            batch_size = batch_data.shape[0]

            x = torch.tensor(batch_data, dtype=torch.float)
            x = self.actor(x)
            x[~action_masks] = self.min_val
            x = self.softmax(x)

            return x, state
        else:
            x = observations
            x = self.actor(x)
            x = self.softmax(x)
            return x


class CriticMLP(torch.nn.Module):
    def __init__(self, input_dim, output_dim, hidden_dim, min_val=torch.finfo(torch.float).min, activation=ReLU):
        super(CriticMLP, self).__init__()
        self.min_val = min_val
        self.output_dim = output_dim
        self.critic = Sequential(
            Linear(input_dim, hidden_dim),
            LayerNorm(hidden_dim),
            activation(),
            Linear(hidden_dim, hidden_dim),
            activation(),
            Linear(hidden_dim, hidden_dim),
            activation(),
            Linear(hidden_dim, 1)
        )

    def forward(self, observations, state=None, info={}):

        #a bit convoluted, there is probably a better way
        # action_masks = observations['mask']
        # batch_data = observations['obs']
        action_masks = observations[:, :self.output_dim].to(torch.bool)
        batch_data = observations[:, self.output_dim:]

        batch_size = batch_data.shape[0]

        x = torch.tensor(batch_data, dtype=torch.float)
        x = self.critic(x)
        return x
