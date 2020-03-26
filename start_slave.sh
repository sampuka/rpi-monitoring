#!/bin/sh

# Create FIFO pipes for wifi and bluetooth pcap data, if they don't exist
[ test wifi_pcap_pipe ] || mkfifo wifi_pcap_pipe
[ test bluetooth_pcap_pipe ] || mkfifo bluetooth_pcap_pipe

# Flush pipes
dd if=wifi_pcap_pipe iflag=nonblock of=/dev/null
dd if=bluetooth_pcap_pipe iflag=nonblock of=/dev/null

# Start capturing wifi and bluetooth
tcpdump -i mon0 -w wifi_pcap_pipe --immediate-mode &
#tcpdump -i bluetooth0 -w bluetooth_pcap_pipe -c 5 &

# Start slave program
./slave
