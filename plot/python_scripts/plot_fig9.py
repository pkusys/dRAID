
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import rc
import os
import sys
import re

rc('font',**{'family':'sans-serif','sans-serif':['Helvetica']})
rc('text', usetex=True)

linux_bw = np.array([2136,
                     3933,
                     7118,
                     11.1 * 1024,
                     11.4 * 1024,
                     11.4 * 1024])
spdk_bw = np.array([2665,
                    5843,
                    9820,
                    11.3 * 1024,
                    11.4 * 1024,
                    11.4 * 1024])
draid_bw = np.array([8688,
                     11.1 * 1024,
                     11.3 * 1024,
                     11.3 * 1024,
                     11.4 * 1024,
                     11.4 * 1024])
linux_lat = np.array([110.39,
                      120.16,
                      133.47,
                      168.67,
                      335.51,
                      341.94])
spdk_lat = np.array([106.91,
                     125.23,
                     135.41,
                     172.63,
                     257.36,
                     342.42])
draid_lat = np.array([100.09,
                      119.82,
                      129.36,
                      171.56,
                      256.89,
                      342.21])

io_size = [4,8,16,32,64,128]

def parse_log(filename):
    f = open(filename, "r")
    data = f.read()
    f.close()
    result = dict()
    read_bw_gb = re.match(r'READ: bw=[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?GiB/s', data)
    if read_bw_gb is not None:
        print('read_bw_gb: ' + read_bw_gb.group())
    read_bw_mb = re.search(r'READ: bw=(\d*\.*\d+)MiB/s', data)
    if read_bw_mb:
        print('read_bw_mb: ', read_bw_mb.group(1))
    write_bw_gb = re.match(r'WRITE: bw=[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?GiB/s', data)
    if write_bw_gb is not None:
        print('write_bw_gb: ' + write_bw_gb.group())
    write_bw_mb = re.match(r'WRITE: bw=[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?MiB/s', data)
    if write_bw_mb is not None:
        print('write_bw_mb: ' + write_bw_mb.group())
    read_lat_ms = re.match(r'read: IOPS.*clat (msec): min=.*avg=[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?,', data)
    if read_lat_ms is not None:
        print('read_lat_ms: ' + read_lat_ms.group())
    read_lat_us = re.search(r'read: IOPS.*\n.*\n *clat \(usec\): min=(?:\d*\.*\d+), max=(?:\d*\.*\d+), avg=(\d*\.*\d+),', data, re.M)
    if read_lat_us:
        print('read_lat_us: ' + read_lat_us.group(1))
    read_lat_ns = re.match(r'read: IOPS.*clat (nsec): min=.*avg=[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?,', data)
    if read_lat_ns is not None:
        print('read_lat_ns: ' + read_lat_ns.group())


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

def _raid5_read_bw():
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
    plt.bar(x+width*2, draid_bw, width=width, linewidth=lw, color='tab:green', linestyle='solid', label='dRAID', hatch='//',edgecolor='black')

    plt.xticks(x+width,io_size)

    plt.yticks(fontsize=27)
    plt.xticks(fontsize=27)
    plt.legend(fontsize=27, loc='lower right')

    bwith = 1
    ax.spines['top'].set_linewidth(bwith)
    ax.spines['right'].set_linewidth(bwith)
    ax.spines['bottom'].set_linewidth(bwith)
    ax.spines['left'].set_linewidth(bwith)

    adapt_y_labels(ax, '{: >3}')
    ax.set_ylim([0, 12000])
    ax.set_ylabel('Bandwidth (MB/s)', fontsize=27)
    ax.set_xlabel('I/O Size (KB)', fontsize=27)

    plt.savefig('plots/fig9a.pdf', bbox_inches='tight', pad_inches=0.2)

def _raid5_read_lat():
    plt.clf()

    fig, ax = plt.subplots(figsize=(6, 5.1))
    confs = {
        "linewidth": 3,
        "markersize": 6
    }
    markers = ['o', 'v', 's', '>']
    lw = 2
    ms = 10
    x = np.array(list(range(6)))
    width = 0.3
    x_ticks = io_size
    plt.xticks(ticks=x_ticks, labels=io_size)

    ax.plot(io_size, linux_lat, linewidth=1, color='tab:blue', linestyle='solid', marker="o", markersize=ms, label='Linux', alpha=0.8)

    ax.plot(io_size, spdk_lat, linewidth=1, color='tab:orange', linestyle='solid', marker="^", markersize=ms, label='SPDK', alpha=0.8)

    ax.plot(io_size, draid_lat, linewidth=4, color='tab:green', linestyle='solid', marker="s", markersize=ms, label='dRAID', alpha=0.8)

    bwith = 1
    ax.spines['top'].set_linewidth(bwith)
    ax.spines['right'].set_linewidth(bwith)
    ax.spines['bottom'].set_linewidth(bwith)
    ax.spines['left'].set_linewidth(bwith)

    #ax.yaxis.label.set_family('Helvetica')
    ax.yaxis.label.set_fontsize(55)

    #ax.xaxis.label.set_family('Helvetica')
    ax.xaxis.label.set_fontsize(45)
    ax.tick_params(labelsize=27, grid_linestyle='-')

    # ax.set_xticklabels(x, batch_size)

    add_value_labels(ax)

    ax.set_ylim([0, 360])
    ax.set_xlim([0, 135])
    adapt_y_labels(ax, '{: >3}')

    ax.set_ylabel('Avg Latency (us)', fontsize=27)
    ax.set_xlabel('I/O Size (KB)', fontsize=27)

    ax.margins(y=0.1)

    ax.xaxis.grid(color='gray', linestyle='--', linewidth=2, alpha=0.5)
    ax.yaxis.grid(color='gray', linestyle='--', linewidth=2, alpha=0.5)

    from matplotlib.pyplot import MultipleLocator
    ax.xaxis.set_major_locator(MultipleLocator(16))

    plt.subplots_adjust(hspace=0, wspace=0.3)
    plt.legend(fontsize=27, frameon=False)
    plt.savefig('plots/fig9b.pdf', bbox_inches='tight', pad_inches=0.2)

#_raid5_read_bw()
#_raid5_read_lat()
parse_log(sys.argv[1] + '/4K.log')
