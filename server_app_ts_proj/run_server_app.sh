#!/bin/sh
pwd

#https://unix.stackexchange.com/questions/8518/how-to-get-my-own-ip-address-and-save-it-to-a-variable-in-a-shell-script
# ip=$(ip route get 8.8.8.8 | sed -n '/src/{s/.*src *\([^ ]*\).*/\1/p;q}')

#!/bin/bash

uname_out="$(uname -s)"
case "${uname_out}" in
    Linux*)     machine=Linux;;
    Darwin*)    machine=Mac;;
    CYGWIN*)    machine=Cygwin;;
    MINGW*)     machine=MinGw;;
    MSYS_NT*)   machine=Git;;
    *)          machine="UNKNOWN:${uname_out}";;
esac
echo "Detected OS: ${machine}"

# Determine IP Address without newline
if [ "$machine" = 'Linux' ]; then
    ip=$(ip route get 8.8.8.8 | awk '/src/{print $7}' | tr -d '\n')
elif [ "$machine" = 'Mac' ]; then
    ip=$(ifconfig en0 | grep "inet " | awk '{print $2}' | tr -d '\n')
else
    echo "Unsupported OS. Exiting."
    exit 1
fi
echo "Detected IP: $ip"

# Replace IP in config files without newlines
if [ "$machine" = 'Mac' ]; then
    sed -i "" "s/127\.0\.0\.1/${ip}/g" ./serverconfig.dev.inf
    sed -i "" "s/127\.0\.0\.1/${ip}/g" ./serverconfig.rel.inf
else
    sed -i "s/127\.0\.0\.1/${ip}/g" ./serverconfig.dev.inf
    sed -i "s/127\.0\.0\.1/${ip}/g" ./serverconfig.rel.inf
fi

# Start the server
npm run start
