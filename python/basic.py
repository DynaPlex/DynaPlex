import sys

# Prints current Python version
print("Current version of Python is ", sys.version)


from lib import DynaPlex as dp

kong = [1,2,3]
settings = { "ape": 1, "donkey": 0.5, "longsetting": -1231000000231}

#process(settings)
dp.process(settings)
#input("press enter to finish")


