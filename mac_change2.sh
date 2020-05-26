#!/bin/sh

ifconfig eth0 down && ifconfig eth0 hw ether 00:e0:4c:53:44:52 && ifconfig eth0 up
