
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import rc
import os
import sys
import re

rc('font',**{'family':'sans-serif','sans-serif':['Helvetica']})
rc('text', usetex=True)

spdk_iops_d = []
draid_iops_d = []
spdk_lat_d = []
draid_lat_d = []

workload = ['YCSB-A', 'YCSB-B', 'YCSB-C', 'YCSB-D', 'YCSB-F']
filenames = ['A.log','B.log','C.log','D.log','F.log']

def parse_log(filename):
    result = dict()
    if os.path.isfile(filename):
        f = open(filename, "r")
        data = f.read()
        f.close()
        iops = re.search(r'total tput: (\d*\.?\d+) K', data)
        if iops:
            result['iops'] = float(iops.group(1))
        lat = re.search(r', work latency: (\d*\.?\d+) us', data)
        if lat:
            result['lat'] = float(lat.group(1))
    return result

def collect_data(draid_path, spdk_path, linux_path):
    global spdk_iops_d, draid_iops_d, spdk_lat_d, draid_lat_d
    for i in filenames:
        extracted_data = parse_log(os.path.join(draid_path, i))
        if 'iops' in extracted_data:
            draid_iops_d.append(extracted_data['iops'])
        else:
            draid_iops_d.append(0)
        if 'lat' in extracted_data:
            draid_lat_d.append(extracted_data['lat'])
        else:
            draid_lat_d.append(0)
        extracted_data = parse_log(os.path.join(spdk_path, i))
        if 'iops' in extracted_data:
            spdk_iops_d.append(extracted_data['iops'])
        else:
            spdk_iops_d.append(0)
        if 'lat' in extracted_data:
            spdk_lat_d.append(extracted_data['lat'])
        else:
            spdk_lat_d.append(0)
    draid_iops_d = np.array(draid_iops_d)
    spdk_iops_d = np.array(spdk_iops_d)
    draid_lat_d = np.array(draid_lat_d)
    spdk_lat_d = np.array(spdk_lat_d)

def add_value_labels(ax, spacing=5, formatstr="{:.1f}"):
    """Add labels to the end of each bar in a bar chart.

    Arguments:
        ax (matplotlib.axes.Axes): The matplotlib object containing the axes
            of the plot to annotate.
        spacing (int): The distance between the labels and the bars.
    """

    # For each bar: Place a label
    for rect in ax.patches:
        # Get X and Y placement of label from rect.
        y_value = rect.get_height()
        x_value = rect.get_x() + rect.get_width() / 2

        # Number of points between bar and label. Change to your liking.
        space = spacing
        # Vertical alignment for positive values
        va = 'bottom'

        # If value of bar is negative: Place label below bar
        if y_value < 0:
            # Invert space to place label below
            space *= -1
            # Vertically align label at top
            va = 'top'

        # Use Y value as label and format number with one decimal place
        label = formatstr.format(y_value)

        # Create annotation
        ax.annotate(
            label,                      # Use `label` as label
            (x_value, y_value),         # Place label at end of the bar
            xytext=(0, space),          # Vertically shift label by `space`
            fontsize=25,
            textcoords="offset points", # Interpret `xytext` as offset in points
            ha='center',                # Horizontally center label
            va=va)                      # Vertically align label differently for
        # positive and negative values.

def adapt_y_labels(ax, space):

    y_value_label = ax.get_yticks()

    new_ylabel = []
    for y_value in y_value_label:

        y_value = round(y_value, 2)
        if int(y_value) == float(y_value):
            new_ylabel.append(space.format(int(y_value)))
        else:
            new_ylabel.append(space.format(y_value))

    ax.set_yticks(y_value_label, new_ylabel)

def _raid5_app(spdk, draid, path):
    plt.clf()
    fig, ax = plt.subplots(figsize=(8, 6))

    confs = {
        "linewidth": 3,
        "markersize": 6
    }
    markers = ['o', 'v', 's', '>']
    lw = 3
    ms = 10
    x = np.array(list(range(5)))
    width = 0.4

    plt.bar(x, spdk, width=width, linewidth=lw, color='tab:orange', linestyle='solid', label='SPDK', hatch='..', edgecolor='black')
    plt.bar(x+width, draid, width=width, linewidth=lw, color='tab:green', linestyle='solid', label='dRAID', hatch='//', edgecolor='black')

    plt.xticks(x+width/2,workload)

    plt.yticks(fontsize=31)
    plt.xticks(fontsize=31, rotation = 30)
    plt.legend(fontsize=31)

    bwith = 1
    ax.spines['top'].set_linewidth(bwith)
    ax.spines['right'].set_linewidth(bwith)
    ax.spines['bottom'].set_linewidth(bwith)
    ax.spines['left'].set_linewidth(bwith)

    ax.set_ylim([0, np.amax(np.maximum(spdk, draid))*1.06])
    adapt_y_labels(ax, '{: >3}')
    if '-bw' in path:
        ax.set_ylabel('Throughput (KIOPS)', fontsize=31)
    else:
        ax.set_ylabel('Avg Latency (us)', fontsize=31)

    plt.savefig(path, bbox_inches='tight', pad_inches=0.2)

collect_data(sys.argv[1], sys.argv[2])
if not os.path.exists('plots'):
    os.makedirs('plots')
_raid5_app(spdk_iops_d,draid_iops_d,'plots/fig21a.pdf')
_raid5_app(spdk_lat_d,draid_lat_d,'plots/fig21b.pdf')
