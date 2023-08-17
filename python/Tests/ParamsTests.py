import os
import sys
#get absolute path to current file.
current_directory = os.path.dirname(os.path.abspath(__file__))
parent_directory = os.path.dirname(current_directory)
#add parent directory (which contains dp directory) to path to enable importing DynaPlex. 
sys.path.append(parent_directory)

from dp.loader import DynaPlex as dp

settings = { "ape": 1, "donkey": 0.5, "longsetting": -120000231,
            "test" : {"test": [1.0,2.0,3.0]} ,
           "names":({"name": "bill"}, {"name2": "bill"})
            }


dp.process(settings)
dp.process(test=['asdf','asdf','asef'],ace='asdf')
x = dp.get()
print(x)
