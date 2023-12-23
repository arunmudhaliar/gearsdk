#!/bin/sh
pwd
cd ./qstats-crawler
./qstats-crawler-app --d "$1" --f "$2" --host "$3" --port "$4"