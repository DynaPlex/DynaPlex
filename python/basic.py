import sys
import os


# Prints current Python version
print("Current version of Python is ", sys.version)
#print(sys.executable)
#from out.WinRelease.lib.DynaPlexLib import DynaPlexLib

os.chdir("..")
os.getcwd()


path=os.path.join(os.getcwd(),"out" ,"WinRelease")

sys.path.append(path)

print(path)

from lib import DynaPlexLib as dp

kong = [1,2,3]
settings = { "ape": 1, "donkey": 0.5, "longsetting": -1231000000231}

#process(settings)
dp.process(settings)
#input("press enter to finish")


