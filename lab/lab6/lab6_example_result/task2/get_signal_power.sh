#!/bin/bash
tcpdump -r $1 -nn -tt -e | grep Beacon | grep $2 | awk '{print $9}' | sort | uniq
