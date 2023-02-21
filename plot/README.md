# Generate Figures

Once you have the results ready, you can use the scripts we created to generate the figures.

Make sure you have `Python3` along with `matplotlib` and `numpy`. Then simply run the following commands on your own computer:
```Bash
cd <your local path to this repository>/plot
./pull_results.sh <your_CloudLab_username> # we reuse the manifest file under setup directory
./generate_all.sh
```
It will copy results from the remote nodes to local directories, then run the Python programs to generate all the figures. The generated plots can be found under `plots` directory.
***
If you want to generate a specific figure when your data are not stored at the default directories, run:
```Bash
cd <your local path to this repository>/plot
python3 python_scripts/plot_fig<#> <path to a directory that contains dRAID results> <path to a directory that contains SPDK results> <path to a directory that contains Linux results (only for FIO experiments)>
```
***
**Note**
- We do not generate the interpolating B-spline for some figures as in the paper.
- Missing data points are substituted with zeroes.