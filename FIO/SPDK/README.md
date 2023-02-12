# SPDK FIO Experiments

If not specified, all the following operations are from node0, which servers as the host.

1. Start all remote targets by running:
```Bash
cd ~/dRAID/FIO/SPDK
./run_nvmf_remote.sh ens1f0 # enter yes when it prompts
```

2. Setup SPDK on node0:
```Bash
cd ~/draid-spdk/scripts
sudo ./setup.sh reset
sudo HUGEMEM=90000 PCI_BLOCKED="0000:c5:00.0 0000:c6:00.0" ./setup.sh
```

3. Follow the instruction of each experiment to reproduce the results.
   - We have not thoroughly tested `run_all.sh` yet, and they may contain bugs.
   - If `run_all.sh` does not work, please manually generate each data point.
   - Figure 17 requires a different testbed setup and a bit code hacking, which we do not include in this artifact.
   - Ignore the warning "RPC client command timeout". It does not affect the experiments.
   - Some experiments may hang there for a while (often observed on large stripe width and very high load), you should kill and rerun them for a more accurate result.