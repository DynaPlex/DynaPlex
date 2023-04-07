import sys

# Prints current Python version
print("Current version of Python is ", sys.version)
#Feature: Version warning system before import!

from lib import DynaPlex as dp

kong = [1,2,3]
settings = { "ape": 1, "donkey": 0.5, "longsetting": -120231}

#process(settings)
#dp.process(settings)
#dp.process(ape=1)

dp.process(**settings)

#input("press enter to finish")


