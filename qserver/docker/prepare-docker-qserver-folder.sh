cd ../../
rm -rf ./docker-server

mkdir ./docker-server
cp -r ./common ./docker-server/common
cp -r ./networkcommon/ ./docker-server/networkcommon
cp -r ./qhiredis/ ./docker-server/qhiredis
cp -r ./qserver/ ./docker-server/qserver
cp ./qserver/docker/Dockerfile ./docker-server/Dockerfile
cp ./qserver/docker/make_qserver.sh ./docker-server/make_qserver.sh
cp ./qserver/docker/run_qserver.sh ./docker-server/run_qserver.sh

# docker-server
rm -rf -- ./docker-server/**/*.o
rm -rf -- ./docker-server/**/**/*.o
rm -rf -- ./docker-server/**/**/**/*.o
rm -rf -- ./docker-server/**/**/**/**/*.o

rm -rf -- ./docker-server/**/.DS_Store
rm -rf -- ./docker-server/**/**/.DS_Store
rm -rf -- ./docker-server/**/**/**/.DS_Store
rm -rf -- ./docker-server/**/**/**/**/.DS_Store

rm -rf -- ./docker-server/**/DerivedData
rm -rf -- ./docker-server/**/**/DerivedData
rm -rf -- ./docker-server/**/**/**/DerivedData
rm -rf -- ./docker-server/**/**/**/**/DerivedData

rm -rf -- ./docker-server/**/*.xcodeproj
rm -rf -- ./docker-server/**/**/*.xcodeproj
rm -rf -- ./docker-server/**/**/**/*.xcodeproj
rm -rf -- ./docker-server/**/**/**/**/*.xcodeproj

rm -rf ./docker-server/networkcommon/libs/macos
rm -rf ./docker-server/qhiredis/libs/macos
rm -rf ./docker-server/qserver/docker
rm -rf ./docker-server/qserver/*.dSYM
rm -rf ./docker-server/qserver/qserver-app

# build
echo "use 'sudo docker build -t qserver-exp .' to build the docker"
# sudo docker build -t qserver-exp .

# run
echo "use 'sudo docker run --name qserver-container -d qserver-exp' to run the docker"
# sudo docker run --name qserver-container -d qserver-exp

# stop
echo "use 'sudo docker stop qserver-container' to stop the docker"
# sudo docker stop qserver-container

# restart
echo "use 'sudo docker restart qserver-container' to re-start the docker"
# sudo docker restart qserver-container

# REMOVE
echo "use 'sudo docker rm --force qserver-container' to REMOVE the docker"
# sudo docker rm --force qserver-container