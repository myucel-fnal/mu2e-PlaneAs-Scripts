import sys
import json
import numpy as np
from array import array
import os
import re
import argparse, textwrap
import datetime
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec

#usage : python plot.py 1
# number tells from which file program will start plotting.

#mypath = "delivery_Feb3/X16-4DF8/GCDC" #usb-2
mypath = "delivery_Feb3/X16-A08A/GCDC" #usb-1
files=os.listdir(mypath)
files.sort()

mystartfile = int(sys.argv[1])

accdata=[]
accx=[]
accy=[]
accz=[]
time=[]
first=1

print(files)
for ifile in files:
    filenumber = int(re.search(r'\d+', ifile).group(0))
    if filenumber < mystartfile:
        continue
    f = open(mypath+"/"+ifile,"r")
    data=f.readlines()
    print (data[0])

    for iline in data:
        if iline.startswith(";"):
            iline=iline.lstrip(';')
            if iline.startswith("Start_time"):
                filestime = iline.rstrip().split(",")
                filestarttime =  datetime.datetime.strptime(filestime[1]+filestime[2], ' %Y-%m-%d %H:%M:%S.%f')
                print (ifile,filestarttime)
                if first==1:
                    starttime = filestarttime
                    first=0
        else:
            d = iline.rstrip().split(",")
            time.append( (float(d[0])+(filestarttime-starttime).total_seconds())/3600. )
            accx.append(float(d[1]))
            accy.append(float(d[2]))
            accz.append(float(d[3]))





fig = plt.figure()
fig, (ax1, ax2,ax3) = plt.subplots(nrows=3, sharex=True)

axs = [ax1,ax2,ax3]

fig.suptitle(mypath + ' Accelerometer data')
axs[0].plot(time, accx)
axs[1].plot(time,accy)
axs[2].plot(time,accz)

for ax in axs:
    ax.label_outer()

plt.subplots_adjust(hspace=.0)
plt.xlabel('time(hours)')
plt.show()


        
