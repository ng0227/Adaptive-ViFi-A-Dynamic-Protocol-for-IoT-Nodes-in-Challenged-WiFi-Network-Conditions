# Adaptive-ViFi-A-Dynamic-Protocol-for-IoT-Nodes-in-Challenged-WiFi-Network-Conditions

## Description
A C implementation of association free protocols, ViFi [1] and Adaptive ViFi [2], that allows an IoT node to stuff data in a probe request for association free communication between IoT nodes and AP. The documentation includes a description of the files, the input parameters, the functions and an example application.


## File Organization
This repo contains the code for:
1. ViFi - Data is stuffed in a probe request, broadcasted on all channels, and at a constant transmission interval.

2. Adaptive ViFi - Data is stuffed in a probe request, broadcasted on a dynamically selected channel and data transmission interval is adaptive as per the operating channel conditions.

3. This repo also contains an easy to use dynamic library of ViFi code.
**Note**: You can create a library of Adaptive ViFi code by following the instruction given in /library/LibraryDocumentation.pdf

## Pre-Requisite
[libnl](https://www.infradead.org/~tgr/libnl/) is required to run the code. To install libnl, run the following command in your terminal:
```
sudo apt-get install libnl-genl-3-dev
```
## How to run ViFi and Adaptive ViFi
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


## How to use ViFi library in a program
1. To use the ViFi library, include library header in your program. Example: `#include<headers.h>` where headers.h is the name of the library header file.

2. Necessary function to call:
* `set_wifi_interface(<Name of the interface>);` 
* `start_probe_stuffing(<data_input_mode>, <data_source>);`

3. Optional functions to set different configurations: 
* `set_effort(<effort 1 or 2>);`
* `set_probe_tx_interval(<time in seconds>);`
* `set_data_to_stuff_size_limit(<size in bytes>);`

Example Program: `MyApplication.c`

```C
#include<headers.h> 
int main () {
  set_wifi_interface("wlp19s0"); 
  start_probe_stuffing (0, "Hello_World");
  return 0; 
}
```

To compile the program:
`gcc -o MyApplication MyApplication.c -I. -L. -lprobeStuffing -Wl, -rpath=.`

**Note**:
* `-I.` flag tells the compiler to find the library header file in the current directory for linking
* `-L.` flag tells the compiler to find the library in the current directory for linking
* `-l` flag specify the name of the probe stuffing library

To run the executable:
`sudo ./MyApplication`


## References
1. Dheryta Jaisinghani, Gursimran Singh, Harish Fulara, Mukulika Maity, and Vinayak Naik. 2018. Elixir: Efficient Data Transfer in WiFi-based IoT Nodes. In Proceedings of the 24th Annual International Conference on Mobile Computing and Networking (MobiCom 2018). DOI:https://doi.org/10.1145/3241539.3267717

2. Dheryta Jaisinghani, Naman Gupta, Mukulika Maity, Vinayak Naik. 2020. Adaptive ViFi: A Dynamic Protocol for IoT Nodes in Challenged WiFi Network Condition. In Proceedings of the 17th IEEE International Conference on Mobile Ad-Hoc and Smart Systems (MASS 2020). 
