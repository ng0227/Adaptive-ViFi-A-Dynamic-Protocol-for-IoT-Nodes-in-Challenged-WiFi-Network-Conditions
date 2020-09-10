# Adaptive-ViFi-A-Dynamic-Protocol-for-IoT-Nodes-in-Challenged-WiFi-Network-Conditions

## Pre-Requisite
[libnl](https://www.infradead.org/~tgr/libnl/) is required to run the code. To install libnl, run the following command in your terminal:
```
sudo apt-get install libnl-genl-3-dev
```
## How to run
**Note**: The code only works on network interfaces whose drivers are compatible with Netlink. Test this by running `iw list` command in your terminal. If your network interface shows up in the output, then it is compatible with Netlink.

**Note**: Root privileges are required to run the compiled program.

Build with: `make`

Run with: `sudo ./probe_stuffing -i <Network Interface> -d <Data> or -f <File Name> -e <Delivery Effort>`

`<Network Interface>` The network interface to use for sending scan request. Data is stuffed inside the probe request packets sent during the scan request.

`<Data>` The data to stuff inside the IEs.

`<File Name>` Read data from the given file and stuff it inside the IEs.

`<Delivery Effort>` 1 for Best delivery effort and 2 for Guaranteed delivery effort. Default = 2.

Example (Best Delivery Effort): `sudo ./probe_stuffing -i wlan0 -d "LoremIpsumDolor" -e 1`

`sudo ./probe_stuffing -i wlan0 -f abc.txt -e 1`

Example (Guaranteed Delivery Effort): `sudo ./probe_stuffing -i wlan0 -d "LoremIpsumDolor" -e 2`

`sudo ./probe_stuffing -i wlan0 -f abc.txt -e 2`
