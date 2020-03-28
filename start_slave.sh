#!/bin/sh

# Create FIFO pipes for wifi and bluetooth pcap data, if they don't exist
if ! test wifi_pcap_pipe ; then mkfifo wifi_pcap_pipe ; fi
#if ! test bluetooth_pcap_pipe ; then mkfifo bluetooth_pcap_pipe ; fi

# Flush pipes
dd if=wifi_pcap_pipe iflag=nonblock of=/dev/null
#dd if=bluetooth_pcap_pipe iflag=nonblock of=/dev/null

# Start capturing wifi and bluetooth
tcpdump -i mon0 -w wifi_pcap_pipe --immediate-mode &
WF_PID=$!
#tcpdump -i bluetooth0 -w bluetooth_pcap_pipe -c 5 &
#BT_PID=$!

trap 'kill $WF_PID ; exit' INT

# Start slave program
./slave "&1"

kill -9 $WF_PID
