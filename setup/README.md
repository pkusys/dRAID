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
3. (Optional) You may change "Number of hosts" to a smaller number (> 6 nodes). If you only want to evaluate the functionality, 10 nodes should be sufficient.
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
./parse_upload_manifest.sh manifest.xml <your_CloudLab_username>
```

## CentOS Setup

The CentOS nodes are all the nodes except the last one. Check the "List View" tab of your experiment page for more details.

SSH into each of the nodes, at your home directory run:
```Bash
# clone the artifacts
git clone https://github.com/pkusys/dRAID.git
# run the setup script
cd dRAID/setup
./configure_cloudlab_centos.sh
```
It will take a few minutes to run, you can setup multiple nodes in parallel.

## Ubuntu Setup

The Ubuntu node is the last one. Check the "List View" tab of your experiment page for more details.

SSH into the node, at your home directory run:
```Bash
# clone the artifacts
git clone https://github.com/pkusys/dRAID.git
# run the setup script
cd dRAID/setup
./configure_cloudlab_ubuntu.sh
```