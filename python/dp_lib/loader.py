import sys
import os
import glob


#get absolute path to current file.
current_directory = os.path.dirname(os.path.abspath(__file__))
#add parent directory to path to enable importing from the /libs subdirectory. 
sys.path.append(current_directory)


import torch


try:
    from libs import DP_Bindings as DynaPlex
except ImportError as e:
    # Check for the existence of a pybind module in the directory
    module_files = glob.glob(os.path.join(current_directory, 'libs', 'DP_Bindings*.pyd'))

    if module_files:
        # If any are found, provide a custom error message.
        print("ERROR: Could not locate the DynaPlex bindings.")
        print("Found the following Pybind11 modules that could provide DP_Bindings:")
        for file in module_files:
            print("-", os.path.basename(file))
        print("However, there seems to be no module that is compatible with the current Python version:", sys.version)
        print("When running scripts, please ensure you're using the same Python interpreter/version that was used when compiling the bindings.")
        sys.exit()
    else:
        # If no modules are found, just raise the original error.
        raise e
    


if __name__ == "__main__":
    print(torch.version)
    DynaPlex.testPyTorch()
