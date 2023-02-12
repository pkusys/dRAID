# CloudLab Testbed Setup

To the best of our knowledge, the only publicly available testbed is CloudLab. 
- To setup a testbed, you will need to acquire a CloudLab account at https://www.cloudlab.us/ first.
- We strongly suggest to reserve the nodes (https://www.cloudlab.us/resgroup.php) in advance, because availability of them are very limited.
  - You will need 20 c6525-100g instances from CloudLab Utah cluster to run the full test suite.
- Don't forget to upload your SSH keys to CloudLab (https://www.cloudlab.us/ssh-keys.php). Otherwise, you will not be able to log into the nodes.

## Clone the repository
You will need to clone this repository to your desktop. Run:
```Bash
git clone https://github.com/pkusys/dRAID.git
```

## Start Experiment

1. Instantiate a new experiment (https://www.cloudlab.us/instantiate.php).
2. Choose the pre-created profile "dRAID-AE".
3. (Optional) You may change "Number of hosts" to a smaller number (>= 6 nodes). If you only want to evaluate the functionality, 10 nodes should be sufficient.
   - You won’t be able to reproduce some of the results that require more nodes, including Figure 12, 14, 16, 25, 27, 29.
   - Some scripts are hardcoded for 20 nodes. You might need to make minor changes to get it work if you want to reproduce other figures with only ten nodes.
4. Use the default setting for the rest of the steps.
5. After a few minutes, the experiment page should say "Your experiment is ready".
6. Copy the content under the "Manifest" tab on the experiment page to a local file `manifest.xml`
```Bash
cd <your local path to this repository>/setup
vim manifest.xml
# paste the content to it, save, and exit.
```
7. Generate configuration files and upload them to all the nodes by running:
```Bash
./setup_all_nodes.sh manifest.xml <your_CloudLab_username>
```