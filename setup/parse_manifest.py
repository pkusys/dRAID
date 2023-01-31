import xml.etree.ElementTree as ET
import os
import json
import sys

if __name__ == '__main__':
    tree = ET.parse(sys.argv[1])
    hosts = open(sys.argv[2], "w")
    ip_addrs = open(sys.argv[3], "w")
    ip_addrs2 = open(sys.argv[4], "w")
    rspec = tree.getroot()
    for node in rspec:
        if node.tag == '{http://www.geni.net/resources/rspec/3}node':
            if node[0][0].attrib['name'] == "urn:publicid:IDN+emulab.net+image+emulab-ops:UBUNTU18-64-STD":
                ubuntu = node[7][0].attrib['hostname']
                continue
            ip_addrs.write(node[3][0].attrib['address'] + '\n')
            ip_addrs2.write(node[2][0].attrib['address'] + '\n')
            for entry in node[7]:
                try:
                    hosts.write(entry.attrib['hostname'] + '\n')
                    break
                except KeyError:
                    continue
    print(ubuntu)
    hosts.close()
    ip_addrs.close()
    ip_addrs2.close()
