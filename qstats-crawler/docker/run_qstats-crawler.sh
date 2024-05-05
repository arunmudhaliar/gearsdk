#!/bin/sh
pwd
cd ./qstats-crawler
host=192.168.0.230
port=5432
./qstats-crawler-app --d "$1" --f "$2" --host "$host" --port "$port"