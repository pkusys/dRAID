
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import rc
import os
import sys
import re

rc('font',**{'family':'sans-serif','sans-serif':['Helvetica']})
rc('text', usetex=True)

linux_bw = []
spdk_bw = []
draid_bw = []
linux_lat = []
spdk_lat = []
draid_lat = []

chunk_size = [32,64,128,256,512,1024]
filenames = ['32K.log','64K.log','128K.log','256K.log','512K.log','1024K.log']

def parse_log(filename):
    result = dict()
    if os.path.isfile(filename):
        f = open(filename, "r")
        data = f.read()
        f.close()
        read_bw_gb = re.search(r'READ: bw=(\d*\.?\d+)GiB/s', data)
        if read_bw_gb:
            result['read_bw'] = float(read_bw_gb.group(1)) * 1024
        read_bw_mb = re.search(r'READ: bw=(\d*\.?\d+)MiB/s', data)
        if read_bw_mb:
            result['read_bw'] = float(read_bw_mb.group(1))
        write_bw_gb = re.search(r'WRITE: bw=(\d*\.?\d+)GiB/s', data)
        if write_bw_gb:
            result['write_bw'] = float(write_bw_gb.group(1)) * 1024
        write_bw_mb = re.search(r'WRITE: bw=(\d*\.?\d+)MiB/s', data)
        if write_bw_mb:
            result['write_bw'] = float(write_bw_mb.group(1))
        read_lat_ms = re.search(r'read: IOPS.*\n.*\n *clat \(msec\): min=(?:\d*\.?\d+), max=(?:\d*\.?\d+k?), avg=(\d*\.?\d+),', data, re.M)
        if read_lat_ms:
            result['read_lat'] = float(read_lat_ms.group(1)) * 1000
        read_lat_us = re.search(r'read: IOPS.*\n.*\n *clat \(usec\): min=(?:\d*\.?\d+), max=(?:\d*\.?\d+k?), avg=(\d*\.?\d+),', data, re.M)
        if read_lat_us:
            result['read_lat'] = float(read_lat_us.group(1))
        read_lat_ns = re.search(r'read: IOPS.*\n.*\n *clat \(nsec\): min=(?:\d*\.?\d+), max=(?:\d*\.?\d+k?), avg=(\d*\.?\d+),', data, re.M)
        if read_lat_ns:
            result['read_lat'] = float(read_lat_ns.group(1)) / 1000
        write_lat_ms = re.search(r'write: IOPS.*\n.*\n *clat \(msec\): min=(?:\d*\.?\d+), max=(?:\d*\.?\d+k?), avg=(\d*\.?\d+),', data, re.M)
        if write_lat_ms:
            result['write_lat'] = float(write_lat_ms.group(1)) * 1000
        write_lat_us = re.search(r'write: IOPS.*\n.*\n *clat \(usec\): min=(?:\d*\.?\d+), max=(?:\d*\.?\d+k?), avg=(\d*\.?\d+),', data, re.M)
        if write_lat_us:
            result['write_lat'] = float(write_lat_us.group(1))
        write_lat_ns = re.search(r'write: IOPS.*\n.*\n *clat \(nsec\): min=(?:\d*\.?\d+), max=(?:\d*\.?\d+k?), avg=(\d*\.?\d+),', data, re.M)
        if write_lat_ns:
            result['write_lat'] = float(write_lat_ns.group(1)) / 1000
    return result

def collect_data(draid_path, spdk_path, linux_path):
    global linux_bw, spdk_bw, draid_bw, linux_lat, spdk_lat, draid_lat
    for i in filenames:
        extracted_data = parse_log(os.path.join(draid_path, i))
        if 'write_bw' in extracted_data:
            draid_bw.append(extracted_data['write_bw'])
        else:
            draid_bw.append(0)
        if 'write_lat' in extracted_data:
            draid_lat.append(extracted_data['write_lat'])
        else:
            draid_lat.append(0)
        extracted_data = parse_log(os.path.join(spdk_path, i))
        if 'write_bw' in extracted_data:
            spdk_bw.append(extracted_data['write_bw'])
        else:
            spdk_bw.append(0)
        if 'write_lat' in extracted_data:
            spdk_lat.append(extracted_data['write_lat'])
        else:
            spdk_lat.append(0)
        extracted_data = parse_log(os.path.join(linux_path, i))
        if 'write_bw' in extracted_data:
            linux_bw.append(extracted_data['write_bw'])
        else:
            linux_bw.append(0)
        if 'write_lat' in extracted_data:
            linux_lat.append(extracted_data['write_lat'])
        else:
            linux_lat.append(0)
    draid_bw = np.array(draid_bw)
    draid_lat = np.array(draid_lat)
    spdk_bw = np.array(spdk_bw)
    spdk_lat = np.array(spdk_lat)
    linux_bw = np.array(linux_bw)
    linux_lat = np.array(linux_lat)

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

def _raid5_write_bw():
    plt.clf()
    fig, ax = plt.subplots(figsize=(8, 5))

    confs = {
        "linewidth": 3,
        "markersize": 6
    }
    markers = ['o', 'v', 's', '>']
    lw = 3
    ms = 10
    x = np.array(list(range(6)))
    width = 0.28

    plt.bar(x, linux_bw, width=width, linewidth=lw, color='tab:blue', linestyle='solid', label='Linux', edgecolor='black')
    plt.bar(x+width, spdk_bw, width=width, linewidth=lw, color='tab:orange', linestyle='solid', label='SPDK', hatch='..', edgecolor='black')
    plt.bar(x+width*2, draid_bw, width=width, linewidth=lw, color='tab:green', linestyle='solid', label='dRAID', hatch='//', edgecolor='black')

    plt.xticks(x+width,chunk_size)

    plt.yticks(fontsize=25)
    plt.xticks(fontsize=25)
    plt.legend(fontsize=25, loc='upper right')

    bwith = 1
    ax.spines['top'].set_linewidth(bwith)
    ax.spines['right'].set_linewidth(bwith)
    ax.spines['bottom'].set_linewidth(bwith)
    ax.spines['left'].set_linewidth(bwith)

    adapt_y_labels(ax, '{: >3}')
    ax.set_ylim([0, 8000])
    ax.set_ylabel('Bandwidth (MB/s)', fontsize=25)
    ax.set_xlabel('Chunk Size (KB)', fontsize=25)

    plt.savefig('plots/fig11a.pdf', bbox_inches='tight', pad_inches=0.2)

def _raid5_write_lat():
    plt.clf()

    fig, ax = plt.subplots(figsize=(6, 5.4))
    confs = {
        "linewidth": 3,
        "markersize": 6
    }
    markers = ['o', 'v', 's', '>']
    lw = 2
    ms = 10
    x = np.array(list(range(6)))
    width = 0.3
    x_ticks = chunk_size
    plt.xticks(ticks=[32,256,512,1024], labels=[32,256,512,1024])

    ax.plot(chunk_size, linux_lat, linewidth=1, color='tab:blue', linestyle='solid', marker = "o",markersize=ms, label='Linux', alpha=0.8)

    ax.plot(chunk_size, spdk_lat, linewidth=1, color='tab:orange', linestyle='solid', marker = "^",markersize=ms, label='SPDK', alpha=0.8)

    ax.plot(chunk_size, draid_lat, linewidth=4, color='tab:green', linestyle='solid', marker = "s",markersize=ms, label='dRAID', alpha=0.8)

    bwith = 1
    ax.spines['top'].set_linewidth(bwith)
    ax.spines['right'].set_linewidth(bwith)
    ax.spines['bottom'].set_linewidth(bwith)
    ax.spines['left'].set_linewidth(bwith)

    ax.yaxis.label.set_fontsize(55)

    ax.xaxis.label.set_fontsize(45)
    ax.tick_params(labelsize=29, grid_linestyle='-')

    add_value_labels(ax)
    ax.set_ylim([0, 1900])
    ax.set_xlim([0, 1060])

    adapt_y_labels(ax, '{: >3}')

    ax.set_ylabel('Avg Latency (us)', fontsize=29)
    ax.set_xlabel('Chunk Size (KB)', fontsize=29,labelpad=20)

    ax.margins(y=0.1)

    ax.yaxis.grid(color='gray', linestyle='--', linewidth=2, alpha=0.5)

    from matplotlib.pyplot import MultipleLocator
    ax.xaxis.set_major_locator(MultipleLocator(32))

    plt.subplots_adjust(hspace=0, wspace=0.3)
    plt.legend(fontsize=29, frameon=False)
    plt.savefig('plots/fig11b.pdf', bbox_inches='tight', pad_inches=0.2)

collect_data(sys.argv[1], sys.argv[2], sys.argv[3])
if not os.path.exists('plots'):
    os.makedirs('plots')
_raid5_write_bw()
_raid5_write_lat()
