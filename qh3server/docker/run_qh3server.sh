#!/bin/sh
pwd
cd ./qh3server

#https://unix.stackexchange.com/questions/8518/how-to-get-my-own-ip-address-and-save-it-to-a-variable-in-a-shell-script
# ip=$(ip route get 8.8.8.8 | sed -n '/src/{s/.*src *\([^ ]*\).*/\1/p;q}')

uname_out="$(uname -s)"
case "${uname_out}" in
    Linux*)     machine=Linux;;
    Darwin*)    machine=Mac;;
    CYGWIN*)    machine=Cygwin;;
    MINGW*)     machine=MinGw;;
    MSYS_NT*)   machine=Git;;
    *)          machine="UNKNOWN:${uname_out}"
esac
echo ${machine}

if [ $machine = 'Linux' ]; then
	ip=$(ip route get 8.8.8.8 | sed -n '/src/{s/.*src *\([^ ]*\).*/\1/p;q}')
elif [ $machine = 'Mac' ]; then
	ip=$(ipconfig getifaddr en0)
fi
port=4004
db_uri=mongodb://192.168.0.230:6006
redis_ip=192.168.0.230
redis_port=6379
./qh3server-app --h $ip --p $port --db $db_uri --rh $redis_ip --rp $redis_port