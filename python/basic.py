import sys

# Prints current Python version
print("Current version of Python is ", sys.version)
#Feature: Version warning system before import!

from lib import DynaPlex as dp

kong = [1,2,3]
settings = { "ape": 1, "donkey": 0.5, "longsetting": -120000231,
            "test" : {"test": [1.0,2.0,3.0]} ,
           "names":({"name": "bill"}, {"name2": "bill"})
            }


dp.process(envi='lostsales',hiddenlayers=[100,200,200])

dp.process({'kort':[1,2,3],'lang':(1,2,3),
            'moeilijk':[{'a':[1,2,1,3,1,2,1,1,3,1,12,13,1,13,13,1,3,1]},{'b':2}],
            'last':1000
            })

input("press enter to finish")


