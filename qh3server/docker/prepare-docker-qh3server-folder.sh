cd ../../
rm -rf ./docker-qh3server

mkdir ./docker-qh3server
cp -r ./common ./docker-qh3server/common
cp -r ./networkcommon/ ./docker-qh3server/networkcommon
cp -r ./qh3server/ ./docker-qh3server/qh3server
cp -r ./qhiredis/ ./docker-qh3server/qhiredis
cp -r ./servercommon/ ./docker-qh3server/servercommon
cp ./qh3server/docker/Dockerfile ./docker-qh3server/Dockerfile
cp ./qh3server/docker/make_qh3server.sh ./docker-qh3server/make_qh3server.sh
cp ./qh3server/docker/run_qh3server.sh ./docker-qh3server/run_qh3server.sh

# docker-qh3server
rm -rf -- ./docker-qh3server/**/*.o
rm -rf -- ./docker-qh3server/**/**/*.o
rm -rf -- ./docker-qh3server/**/**/**/*.o
rm -rf -- ./docker-qh3server/**/**/**/**/*.o

rm -rf -- ./docker-qh3server/**/.DS_Store
rm -rf -- ./docker-qh3server/**/**/.DS_Store
rm -rf -- ./docker-qh3server/**/**/**/.DS_Store
rm -rf -- ./docker-qh3server/**/**/**/**/.DS_Store

rm -rf -- ./docker-qh3server/**/DerivedData
rm -rf -- ./docker-qh3server/**/**/DerivedData
rm -rf -- ./docker-qh3server/**/**/**/DerivedData
rm -rf -- ./docker-qh3server/**/**/**/**/DerivedData

rm -rf -- ./docker-qh3server/**/*.xcodeproj
rm -rf -- ./docker-qh3server/**/**/*.xcodeproj
rm -rf -- ./docker-qh3server/**/**/**/*.xcodeproj
rm -rf -- ./docker-qh3server/**/**/**/**/*.xcodeproj

rm -rf ./docker-qh3server/networkcommon/libs/macos
rm -rf ./docker-qh3server/common/libs/macos
rm -rf ./docker-qh3server/qhiredis/libs/macos
rm -rf ./docker-qh3server/servercommon/libs/macos
rm -rf ./docker-qh3server/qh3server/docker
rm -rf ./docker-qh3server/qh3server/*.dSYM
rm -rf ./docker-qh3server/qh3server/qh3server-app

# build
echo "use 'sudo docker build -t qh3server-exp .' to build the docker"
# sudo docker build -t qh3server.

# run
echo "use 'sudo docker run --publish 4004:4004/udp --name qh3server-container -d qh3server-exp' to run the docker"
# sudo docker run --publish 4004:4004/udp --name qh3server-container -d qh3server-exp

# stop
echo "use 'sudo docker stop qh3server-container' to stop the docker"
# sudo docker stop qh3server-container

# restart
echo "use 'sudo docker restart qh3server-container' to re-start the docker"
# sudo docker restart qh3server-container

# REMOVE
echo "use 'sudo docker rm --force qh3server-container' to REMOVE the docker"
# sudo docker rm --force qh3server-container