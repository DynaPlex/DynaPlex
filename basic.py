
from pickle import TRUE
import sys
import numpy as np

# Prints current Python version
print("Current version of Python is ", sys.version)
#print(sys.executable)
from out.WinRelease.lib.DynaPlexLib import DynaPlex as DP

kong = [1,2,3]
settings = { "ape": 1, "donkey": 0.5, "longsetting": -1231000000231}


DP.process(settings)


#input("press enter to finish")


