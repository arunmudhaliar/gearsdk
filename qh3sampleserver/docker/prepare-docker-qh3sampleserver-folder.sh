#!/bin/bash

# Define variables for the directories
SERVER="qh3sampleserver"
DOCKER_DIR="./docker-$SERVER"
QH3_SERVER_DIR="./qh3server"
SERVER_DIR="./$SERVER"

# Move to the appropriate directory
cd ../../

# Remove the old directory if it exists
rm -rf $DOCKER_DIR

# Create a new directory
mkdir $DOCKER_DIR

# Copy necessary files and directories
cp -r ./common $DOCKER_DIR/common
cp -r ./configs $DOCKER_DIR/configs
cp -r ./networkcommon/ $DOCKER_DIR/networkcommon
cp -r $QH3_SERVER_DIR/ $DOCKER_DIR/qh3server
cp -r $SERVER_DIR/ $DOCKER_DIR/$SERVER
cp -r ./qh3client/ $DOCKER_DIR/qh3client
cp -r ./qhiredis/ $DOCKER_DIR/qhiredis
cp -r ./qstats-crawler/ $DOCKER_DIR/qstats-crawler
cp -r ./servercommon/ $DOCKER_DIR/servercommon
cp -r ./qzookeeper/ $DOCKER_DIR/qzookeeper
cp -r ./qutils/ $DOCKER_DIR/qutils
cp $SERVER_DIR/docker/Dockerfile $DOCKER_DIR/Dockerfile
cp $SERVER_DIR/docker/make_$SERVER.sh $DOCKER_DIR/make_$SERVER.sh
cp $SERVER_DIR/docker/run_$SERVER.sh $DOCKER_DIR/run_$SERVER.sh

# Copy supervisor configuration
cp $SERVER_DIR/docker/supervisord.conf $DOCKER_DIR/supervisord.conf

# Copy qstats-crawler files
cp ./qstats-crawler/docker/make_qstats-crawler.sh $DOCKER_DIR/make_qstats-crawler.sh
cp ./qstats-crawler/docker/run_qstats-crawler.sh $DOCKER_DIR/run_qstats-crawler.sh
cp ./qstats-crawler/docker/make_postgresql.sh $DOCKER_DIR/make_postgresql.sh

# Clean up object files and macOS specific files
find $DOCKER_DIR -name '*.o' -exec rm -rf {} +
find $DOCKER_DIR -name '.DS_Store' -exec rm -rf {} +
find $DOCKER_DIR -name 'DerivedData' -exec rm -rf {} +
find $DOCKER_DIR -name '*.xcodeproj' -exec rm -rf {} +

# Remove macOS specific libraries
rm -rf $DOCKER_DIR/networkcommon/libs/macos
rm -rf $DOCKER_DIR/common/libs/macos
rm -rf $DOCKER_DIR/qhiredis/libs/macos
rm -rf $DOCKER_DIR/qstats-crawler/libs/macos
rm -rf $DOCKER_DIR/servercommon/libs/macos
rm -rf $DOCKER_DIR/qzookeeper/libs/macos
rm -rf $DOCKER_DIR/qutils/libs/macos
rm -rf $DOCKER_DIR/qh3server/docker
rm -rf $DOCKER_DIR/$SERVER/docker
rm -rf $DOCKER_DIR/$SERVER/*.dSYM
rm -rf $DOCKER_DIR/$SERVER/$SERVER-app

# Instructions for building and running the Docker container
echo "use 'sudo docker build -t $SERVER-exp .' to build the docker"
echo "use 'sudo docker run --publish 4004:4004/udp --name $SERVER-container -d $SERVER-exp' to run the docker"
echo "use 'sudo docker stop $SERVER-container' to stop the docker"
echo "use 'sudo docker restart $SERVER-container' to restart the docker"
echo "use 'sudo docker rm --force $SERVER-container' to remove the docker"
