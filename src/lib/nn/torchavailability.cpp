#include "dynaplex/torchavailability.h"
#include <iostream>
#if DP_TORCH_AVAILABLE
#include <torch/torch.h>
#endif

namespace DynaPlex ::TorchAvailability {

    bool TorchAvailable()
    {
#if DP_TORCH_AVAILABLE
        return true;
#else


        return false;
#endif	
    }

    std::string TorchVersion()
    {
        std::string result;

#if DP_TORCH_AVAILABLE
        result = "DynaPlex: torch available, Version ";
        result += std::to_string(TORCH_VERSION_MAJOR) + ".";
        result += std::to_string(TORCH_VERSION_MINOR) + ".";
        result += std::to_string(TORCH_VERSION_PATCH) + "\t";
        if (torch::cuda::is_available())
        {
       //     result += "cuda available. ";
        }
        else
        {
      //      result += "cuda not available. ";
        }       
#else
        result = "DynaPlex: torch not available. To make available, set dynaplex_enable_pytorch to true and dynaplex_pytorch_path to an appropriate path, e.g. in CMakeUserPresets.txt";
#endif	

        return result;
    }
}