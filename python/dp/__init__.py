import json
import sys
import os
import glob

import torch

try:
    import importlib.util

    # Dynamically load the DP_Bindings .so file
    _dp_dir = os.path.dirname(__file__)
    _so_file = next(
        f for f in os.listdir(_dp_dir)
        if f.startswith("DP_Bindings") and f.endswith(".so")
    )
    _so_path = os.path.join(_dp_dir, _so_file)

    _spec = importlib.util.spec_from_file_location("DP_Bindings", _so_path)
    DP_Bindings = importlib.util.module_from_spec(_spec)
    _spec.loader.exec_module(DP_Bindings)

    dynaplex = DP_Bindings

except Exception as e:
    raise ImportError(
        f"Failed to import DP_Bindings from {_so_file if 'DP_Bindings' in locals() else 'unknown'}.\n"
        f"Make sure the shared object (.so) was correctly built and placed in the dp/ directory.\n"
        f"Original error: {e}"
    )

    print(sys.version)
    # get absolute path to current file.
    current_directory = os.path.dirname(os.path.abspath(__file__))
    # add parent directory to path to enable importing from the /libs subdirectory.
    sys.path.append(current_directory)

    # Check for the existence of a pybind module in the directory
    so_files = glob.glob(os.path.join(current_directory, 'DP_Bindings*.so'))
    pyd_files = glob.glob(os.path.join(current_directory, 'DP_Bindings*.pyd'))
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


def save_policy(model, json_info, path, device='cpu'):
    # Save the model and json at the path.
    # Torchscript model
    if device != torch.device('cpu'):
        model.to('cpu')

    model.dynaplex_eval = True
    traced_script_module = torch.jit.script(model)
    traced_script_module.save(f'{path}.pth')

    if device != torch.device('cpu'):
        model.to(device)
    model.dynaplex_eval = False

    # Json file
    # Serializing json
    json_info['id'] = 'torchscript'
    json_obj = json.dumps(json_info, indent=1)
    # Writing to sample.json
    with open(f"{path}.json", "w") as outfile:
        outfile.write(json_obj)


dynaplex.save_policy = save_policy
