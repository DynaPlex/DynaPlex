import sys
import os
import glob


print(sys.version)
#get absolute path to current file.
current_directory = os.path.dirname(os.path.abspath(__file__))
#add parent directory to path to enable importing from the /libs subdirectory. 
sys.path.append(current_directory)

import torch

try:
    from libs import DP_Bindings as DynaPlex
except ImportError as e:
    # Check for the existence of a pybind module in the directory
    so_files = glob.glob(os.path.join(current_directory, 'libs', 'DP_Bindings*.so'))    
    pyd_files = glob.glob(os.path.join(current_directory, 'libs', 'DP_Bindings*.pyd'))
    module_files = so_files + pyd_files
    if module_files:
        # If any are found, provide a custom error message.
        print("ERROR: Could not locate or failed to load the DynaPlex bindings.")
        print("Found the following Pybind11 modules that could provide DP_Bindings:")
        for file in module_files:
            print("-", os.path.basename(file))
        print("When running scripts, please ensure you're using the same Python interpreter/version that was used when compiling the bindings.")
        print("Inner message")
        print(e.msg)
        sys.exit()
    else:
        # If no modules are found, just raise the original error.
        raise e
    


if __name__ == "__main__":
    print("Torch version (python):  " +torch.__version__)
    DynaPlex.testPyTorch()
