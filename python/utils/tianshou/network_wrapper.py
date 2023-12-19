from typing import Union, Dict

import torch
import torch.nn as nn


class TianshouModuleWrapper(nn.Module):
    def __init__(self, wrapped_module):
        super().__init__()
        self.wrapped_module = wrapped_module

    def forward(self, observations, state, info):
        out = self.wrapped_module(observations)

        return out, state

    def __getattr__(self, name):
        try:
            return super().__getattr__(name)
        except AttributeError:
            if name == "wrapped_module":
                raise AttributeError()
            return getattr(self.wrapped_module, name)
