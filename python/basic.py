import sys

#need check whether current python version is 
#version for which the extension was built.!

# Prints current Python version
print("Current version of Python is ", sys.version)
#Feature: Version warning system before import!

#seems better to first import torch, and then pytorch. 
import torch
from libs import DP_Bindings as dp



def test_torch():
    print(f"PyTorch version: {torch.__version__}")

    # Create a tensor
    x = torch.rand(5, 3)
    print(f"Random Tensor:\n{x}")

    # Test a simple operation
    y = torch.rand(5, 3)
    z = x + y
    print(f"Sum of two Tensors:\n{z}")

try:
    test_torch()
except ImportError:
    print("PyTorch is not installed.")
except Exception as e:
    print(f"An error occurred: {e}")



kong = [1,2,3]
settings = { "ape": 1, "donkey": 0.5, "longsetting": -120000231,
            "test" : {"test": [1.0,2.0,3.0]} ,
           "names":({"name": "bill"}, {"name2": "bill"})
            }
#dp.process(settings)


#dp.process(test=['asdf','asdf','asef'],ace='asdf')


dp.testPyTorch()
x = dp.get()
print(x)

#dp.process({"envi":'lostsales',"hiddenlayers":[100,200,200]})

#dp.process({'kort':[1,2,3],'lang':(1,2,3),
#            'moeilijk':[{'a':[1,2,1,3,1,2,1,1,3,1,12,13,1,13,13,1,3,1]},{'b':2}],
#           'last':1000
#           })


