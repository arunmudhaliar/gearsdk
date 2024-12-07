#!/bin/bash

# Define variables for directories
SERVER="qsampleserver"
BASE_DIR="../../"
DOCKER_DIR="./docker-$SERVER"
COMMON_DIR="./common"
NETWORK_COMMON_DIR="./networkcommon"
SERVER_COMMON_DIR="./servercommon"
QHIREDIS_DIR="./qhiredis"
QUTILS_DIR="./qutils"
QSERVER_DIR="./qserver"
QSAMPLESERVER_DOCKER_DIR="$SERVER/docker"

# Move to the base directory
cd $BASE_DIR

# Remove the old directory if it exists
rm -rf $DOCKER_DIR

# Create a new directory
mkdir $DOCKER_DIR

# Copy necessary files and directories
cp -r $COMMON_DIR $DOCKER_DIR/common
cp -r ./configs $DOCKER_DIR/configs
cp -r $NETWORK_COMMON_DIR $DOCKER_DIR/networkcommon
cp -r $SERVER_COMMON_DIR $DOCKER_DIR/servercommon
cp -r ./qzookeeper/ $DOCKER_DIR/qzookeeper
cp -r $QHIREDIS_DIR $DOCKER_DIR/qhiredis
cp -r $QUTILS_DIR $DOCKER_DIR/qutils
cp -r $QSERVER_DIR $DOCKER_DIR/qserver
cp -r ./$SERVER $DOCKER_DIR/$SERVER
cp $QSAMPLESERVER_DOCKER_DIR/Dockerfile $DOCKER_DIR/Dockerfile
cp $QSAMPLESERVER_DOCKER_DIR/make_$SERVER.sh $DOCKER_DIR/make_$SERVER.sh
cp $QSAMPLESERVER_DOCKER_DIR/run_$SERVER.sh $DOCKER_DIR/run_$SERVER.sh

# Clean up object files and macOS specific files
find $DOCKER_DIR -name '*.o' -exec rm -rf {} +
find $DOCKER_DIR -name '.DS_Store' -exec rm -rf {} +
find $DOCKER_DIR -name 'DerivedData' -exec rm -rf {} +
find $DOCKER_DIR -name '*.xcodeproj' -exec rm -rf {} +

# Remove macOS specific directories
rm -rf $DOCKER_DIR/networkcommon/libs/macos
rm -rf $DOCKER_DIR/qhiredis/libs/macos
rm -rf $DOCKER_DIR/qutils/libs/macos
rm -rf $DOCKER_DIR/qzookeeper/libs/macos
rm -rf $DOCKER_DIR/$SERVER/docker
rm -rf $DOCKER_DIR/$SERVER/*.dSYM
rm -rf $DOCKER_DIR/$SERVER/$SERVER-app

# Instructions for building and running the Docker container
echo "use 'sudo docker build -t $SERVER-exp .' to build the docker"
echo "use 'sudo docker run --name $SERVER-container -d $SERVER-exp' to run the docker"
echo "use 'sudo docker stop $SERVER-container' to stop the docker"
echo "use 'sudo docker restart $SERVER-container' to restart the docker"
echo "use 'sudo docker rm --force $SERVER-container' to remove the docker"
