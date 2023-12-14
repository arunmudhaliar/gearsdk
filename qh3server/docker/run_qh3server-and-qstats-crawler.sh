#!/bin/bash
pwd
echo "This script only works in linux. 'ip' cmd not available in other OS."
#https://unix.stackexchange.com/questions/8518/how-to-get-my-own-ip-address-and-save-it-to-a-variable-in-a-shell-script
cd ./qh3server
ip=$(ip route get 8.8.8.8 | sed -n '/src/{s/.*src *\([^ ]*\).*/\1/p;q}')
port=4004
db_uri=mongodb://192.168.0.230:6006
redis_ip=192.168.0.230
redis_port=6379
pwd
# ./qh3server-app --h $ip --p $port --db $db_uri --rh $redis_ip --rp $redis_port
# ../qstats-crawler/qstats-crawler-app --f "./stats/qh3_statfile"
# ./qh3server-app --h $ip --p $port --db $db_uri --rh $redis_ip --rp $redis_port &
cd ../qstats-crawler
./qstats-crawler-app --f "./stats/qh3_statfile"

# # # Wait for any process to exit
# wait -n

# # # Exit with status of process that exited first
# exit $?