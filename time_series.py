from mpl_toolkits.axes_grid1 import host_subplot
from mpl_toolkits import axisartist
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import argparse

# Grab user arguments
parser = argparse.ArgumentParser(description="Visualize time-series data for cooperator ant success")

parser.add_argument("FILE", type=str, help="relative path to .csv file")
parser.add_argument("-s", type=str, help="output path of plot (default: saved working directory as plot.png)")

# Parse arguments
args = parser.parse_args()
if not args.s: args.s = "plot.png"

# Define fixed figure parameters
plt.rcParams["figure.figsize"] = (12,10)
plt.rcParams["axes.labelsize"] = 16
plt.rcParams['ytick.labelsize'] = 12
plt.rcParams['legend.fontsize'] = 12

# Parse CSV file
data = pd.read_csv(args.FILE,header=None).T
data = np.array(data)

# Create plots
host = host_subplot(111, axes_class=axisartist.Axes)
plt.subplots_adjust(right=0.75)
par1 = host.twinx()
par1.axis["right"].toggle(all=True)

# Plot data in semi-log scale
l1,l2 = host.semilogy(data[0:2].T) #this is the left axis data
l3,l4 = par1.semilogy(100*data[2:4].T) #this is the right axis data, multiply by 100 to make it percentage.

# Modify the line styles
plt.setp(l2,linestyle='--', linewidth=2) 
plt.setp(l3, linestyle=':', linewidth=2)
plt.setp(l4, linestyle='-.', linewidth=2)  

# Set y-axes limits
host.set_ylim(1e-3, 100)
par1.set_ylim(1e-3, 100) #set y range for fraction to be between 0 - 100

# Create axes labels
plt.xlabel('Iterations') #x axis
plt.ylabel('Food bits per ants', fontsize=20) #left axis
par1.set_ylabel("Percentage of cooperators", fontsize=20) #right axis

# Create legends
plt.legend([l1, l2, l3, l4],
    ['Food bits per ant collected from source',
    'Food bits per ant delivered food to nest',
    'Successful collectors',
    'Successful deliverers'])  #change legends here for right data

# Save figure
plt.savefig(args.s, dpi = 400, facecolor=host.get_facecolor())