#!/bin/sh
pwd
cd ./qlogs-crawler
python3 ./qlogs-crawler.py "$1" "$2" "$3" --batch_size $4 --interval $5