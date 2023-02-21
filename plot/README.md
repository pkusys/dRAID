# Generate Figures

Once you have the results ready, you can use the scripts we created to generate the figures.

Make sure you have `Python3` along with `matplotlib` and `numpy`. Then simply run the following commands on your own computer:
```Bash
cd <your local path to this repository>/plot
./pull_results.sh
./generate_all.sh
```
It will copy results from the remote nodes to local directories, then run the Python programs to generate all the figures.
___
If you want to generate a specific figure when your data are not stored at the default directories, run:
```Bash
cd <your local path to this repository>/plot
python3 plot_fig<#> <path to a directory that contains dRAID results> <path to a directory that contains SPDK results> <path to a directory that contains Linux results (only for FIO experiments)>
```