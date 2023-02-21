
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import rc
import os
import sys
import re
from matplotlib.patches import Patch

rc('font',**{'family':'sans-serif','sans-serif':['Helvetica']})
rc('text', usetex=True)

linux_bw_write = []
linux_bw_read = []
spdk_bw_write = []
spdk_bw_read = []
draid_bw_write = []
draid_bw_read = []
linux_lat_write = []
linux_lat_read = []
spdk_lat_write = []
spdk_lat_read = []
draid_lat_write = []
draid_lat_read = []

linux_lat = []
spdk_lat = []
draid_lat = []

rw_ratio = [0, 25, 50, 75, 100]
filenames = ['0.log','25.log','50.log','75.log','100.log']

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
    global linux_bw_write, linux_bw_read, spdk_bw_write, spdk_bw_read, draid_bw_write, draid_bw_read, linux_lat_write, \
        linux_lat_read, spdk_lat_write, spdk_lat_read, draid_lat_write, draid_lat_read, linux_lat, spdk_lat, draid_lat
    for i in filenames:
        extracted_data = parse_log(os.path.join(draid_path, i))
        if 'read_bw' in extracted_data:
            draid_bw_read.append(extracted_data['read_bw'])
        else:
            draid_bw_read.append(0)
        if 'read_lat' in extracted_data:
            draid_lat_read.append(extracted_data['read_lat'])
        else:
            draid_lat_read.append(0)
        if 'write_bw' in extracted_data:
            draid_bw_write.append(extracted_data['write_bw'])
        else:
            draid_bw_write.append(0)
        if 'write_lat' in extracted_data:
            draid_lat_write.append(extracted_data['write_lat'])
        else:
            draid_lat_write.append(0)
        extracted_data = parse_log(os.path.join(spdk_path, i))
        if 'read_bw' in extracted_data:
            spdk_bw_read.append(extracted_data['read_bw'])
        else:
            spdk_bw_read.append(0)
        if 'read_lat' in extracted_data:
            spdk_lat_read.append(extracted_data['read_lat'])
        else:
            spdk_lat_read.append(0)
        if 'write_bw' in extracted_data:
            spdk_bw_write.append(extracted_data['write_bw'])
        else:
            spdk_bw_write.append(0)
        if 'write_lat' in extracted_data:
            spdk_lat_write.append(extracted_data['write_lat'])
        else:
            spdk_lat_write.append(0)
        extracted_data = parse_log(os.path.join(linux_path, i))
        if 'read_bw' in extracted_data:
            linux_bw_read.append(extracted_data['read_bw'])
        else:
            linux_bw_read.append(0)
        if 'read_lat' in extracted_data:
            linux_lat_read.append(extracted_data['read_lat'])
        else:
            linux_lat_read.append(0)
        if 'write_bw' in extracted_data:
            linux_bw_write.append(extracted_data['write_bw'])
        else:
            linux_bw_write.append(0)
        if 'write_lat' in extracted_data:
            linux_lat_write.append(extracted_data['write_lat'])
        else:
            linux_lat_write.append(0)
    linux_bw_write = np.array(linux_bw_write)
    linux_bw_read = np.array(linux_bw_read)
    spdk_bw_write = np.array(spdk_bw_write)
    spdk_bw_read = np.array(spdk_bw_read)
    draid_bw_write = np.array(draid_bw_write)
    draid_bw_read = np.array(draid_bw_read)
    linux_lat_write = np.array(linux_lat_write)
    linux_lat_read = np.array(linux_lat_read)
    spdk_lat_write = np.array(spdk_lat_write)
    spdk_lat_read = np.array(spdk_lat_read)
    draid_lat_write = np.array(draid_lat_write)
    draid_lat_read = np.array(draid_lat_read)
    linux_lat = (np.array(rw_ratio) * linux_lat_read + np.array(rw_ratio)[::-1] * linux_lat_write) / 100
    spdk_lat = (np.array(rw_ratio) * spdk_lat_read + np.array(rw_ratio)[::-1] * spdk_lat_write) / 100
    draid_lat = (np.array(rw_ratio) * draid_lat_read + np.array(rw_ratio)[::-1] * draid_lat_write) / 100

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
    x = np.array(list(range(5)))
    width = 0.28

    plt.bar(x, linux_bw_read, width=width, linewidth=lw, color='tab:blue', linestyle='solid', label='', edgecolor='black')
    plt.bar(x+width, spdk_bw_read, width=width, linewidth=lw, color='tab:orange', linestyle='solid', label='', edgecolor='black', hatch='..')
    plt.bar(x+width*2, draid_bw_read, width=width, linewidth=lw, color='tab:green', linestyle='solid', label='', edgecolor='black', hatch='//')
    plt.bar(x, linux_bw_write, width=width, linewidth=lw, color='tab:blue', linestyle='solid', label='', edgecolor='black', bottom=linux_bw_read)
    plt.bar(x+width, spdk_bw_write, width=width, linewidth=lw, color='tab:orange', linestyle='solid', label='', edgecolor='black', bottom=spdk_bw_read, hatch='..')
    plt.bar(x+width*2, draid_bw_write, width=width, linewidth=lw, color='tab:green', linestyle='solid', label='', edgecolor='black', bottom=draid_bw_read, hatch='//')

    plt.yticks(fontsize=26)
    plt.xticks(fontsize=26)

    legend_elements = [Patch(facecolor='tab:blue', edgecolor='black',
                             label='Linux'),
                       Patch(facecolor='tab:orange', edgecolor='black',
                             hatch='..', label='SPDK'),
                       Patch(facecolor='tab:green', edgecolor='black',
                             hatch='//', label='dRAID')
                       ]
    plt.legend(handles=legend_elements, fontsize=26, loc='best')

    plt.xticks(x+width)
    ax.set_xticklabels(('0\%', '25\%', '50\%', '75\%', '100\%'))

    bwith = 1
    ax.spines['top'].set_linewidth(bwith)
    ax.spines['right'].set_linewidth(bwith)
    ax.spines['bottom'].set_linewidth(bwith)
    ax.spines['left'].set_linewidth(bwith)

    adapt_y_labels(ax, '{: >3}')
    ax.set_ylim([0, 14000])
    ax.set_ylabel('Bandwidth (MB/s)', fontsize=26)
    ax.set_xlabel('Read Ratio', fontsize=26)
    plt.savefig('plots/fig13a.pdf', bbox_inches='tight', pad_inches=0.2)

def _raid5_write_lat():
    plt.clf()

    fig, ax = plt.subplots(figsize=(6, 5.5))
    confs = {
        "linewidth": 3,
        "markersize": 6
    }
    markers = ['o', 'v', 's', '>']
    lw = 2
    ms = 10
    x = np.array(list(range(6)))
    width = 0.3

    ax.plot(rw_ratio, linux_lat, linewidth=1, color='tab:blue', linestyle='solid', marker = "o",markersize=ms, label='Linux', alpha=0.8)

    ax.plot(rw_ratio, spdk_lat, linewidth=1, color='tab:orange', linestyle='solid', marker = "^",markersize=ms, label='SPDK', alpha=0.8)

    ax.plot(rw_ratio, draid_lat, linewidth=4, color='tab:green', linestyle='solid', marker = "s",markersize=ms, label='dRAID', alpha=0.8)

    bwith = 1
    ax.spines['top'].set_linewidth(bwith)
    ax.spines['right'].set_linewidth(bwith)
    ax.spines['bottom'].set_linewidth(bwith)
    ax.spines['left'].set_linewidth(bwith)

    ax.yaxis.label.set_fontsize(55)

    ax.xaxis.label.set_fontsize(45)
    ax.tick_params(labelsize=31, grid_linestyle='-')
    ax.tick_params(axis='x',which='major',pad=10)

    add_value_labels(ax)
    ax.set_ylim([0, 1800])
    ax.set_xlim([0, 100])

    adapt_y_labels(ax, '{: >3}')
    ax.set_ylabel('Avg Latency (us)', fontsize=31)
    ax.set_xlabel('Read Ratio', fontsize=31,labelpad=10)

    ax.margins(y=0.1)

    ax.xaxis.grid(color='gray', linestyle='--', linewidth=2, alpha=0.5)
    ax.yaxis.grid(color='gray', linestyle='--', linewidth=2, alpha=0.5)

    from matplotlib.pyplot import MultipleLocator
    ax.xaxis.set_major_locator(MultipleLocator(16))

    plt.subplots_adjust(hspace=0, wspace=0.3)
    plt.legend(fontsize=31, frameon=False)
    plt.xticks(ticks=rw_ratio, labels=['0\%','25\%','50\%','75\%','100\%'])
    plt.savefig('plots/fig13b.pdf', bbox_inches='tight', pad_inches=0.2)

collect_data(sys.argv[1], sys.argv[2], sys.argv[3])
if not os.path.exists('plots'):
    os.makedirs('plots')
_raid5_write_bw()
_raid5_write_lat()
