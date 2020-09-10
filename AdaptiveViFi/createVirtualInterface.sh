#hardcoded for now

#creates interface
sudo iw dev wlp2s0 interface add monitor type monitor

sudo ifconfig monitor up

#sets channel 
sudo iwconfig monitor channel 6


