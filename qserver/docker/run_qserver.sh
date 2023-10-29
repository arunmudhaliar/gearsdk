pwd
echo "This script only works in linux. 'ip' cmd not available in other OS."
cd ./qserver
ip=$(ip route get 8.8.8.8 | sed -n '/src/{s/.*src *\([^ ]*\).*/\1/p;q}')
port=4000
./qserver-app $ip $port