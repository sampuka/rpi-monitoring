# rpi-monitoring
Passive monitoring for RPi

## Startup:
### On sniffer0 (192.168.10.2):
```
./init.sh
./start_master.sh
./start_slave.sh 0
```

### On sniffer1 (192.168.10.3):
```
./init.sh
./start_slave.sh 1
```

### On sniffer2 (192.168.10.4):
```
./init.sh
./start_slave.sh 2
```
