import os
import sys

parent_directory = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
#load DynaPlex from ../dp/loader.py
sys.path.append(parent_directory)
from dp.loader import DynaPlex as dp

settings = { "ape": 1, "donkey": 0.5, "longsetting": -120000231,
            "test" : {"test": [1.0,2.0,3.0]} ,
           "names":({"name": "bill"}, {"name2": "bill"})
            }


dp.process(settings)
dp.process(test=['asdf','asdf','asf'],ace='asdf')
x = dp.get()
print(x)
