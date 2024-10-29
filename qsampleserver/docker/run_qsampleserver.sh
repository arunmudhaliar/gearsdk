pwd
echo "This script only works in linux. 'ip' cmd not available in other OS."
#https://unix.stackexchange.com/questions/8518/how-to-get-my-own-ip-address-and-save-it-to-a-variable-in-a-shell-script
cd ./qsampleserver
ip=$(ip route get 8.8.8.8 | sed -n '/src/{s/.*src *\([^ ]*\).*/\1/p;q}')
port=4000
db_uri=mongodb://192.168.0.230:27017
redis_ip=192.168.0.230
redis_port=6379
zk_uri=192.168.0.230:2181
./qsampleserver-app --h $ip --p $port --db $db_uri --rh $redis_ip --rp $redis_port --zk $zk_uri