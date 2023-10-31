cd ../../
rm -rf ./docker-http-server

mkdir ./docker-http-server
cp -r ./common ./docker-http-server/common
cp -r ./networkcommon/ ./docker-http-server/networkcommon
cp -r ./qh3server/ ./docker-http-server/qh3server
cp ./qh3server/docker/Dockerfile ./docker-http-server/Dockerfile
cp ./qh3server/docker/make_qhttp_server.sh ./docker-http-server/make_qhttp_server.sh
cp ./qh3server/docker/run_qhttp_server.sh ./docker-http-server/run_qhttp_server.sh

# docker-http-server
rm -rf -- ./docker-http-server/**/*.o
rm -rf -- ./docker-http-server/**/**/*.o
rm -rf -- ./docker-http-server/**/**/**/*.o
rm -rf -- ./docker-http-server/**/**/**/**/*.o

rm -rf -- ./docker-http-server/**/.DS_Store
rm -rf -- ./docker-http-server/**/**/.DS_Store
rm -rf -- ./docker-http-server/**/**/**/.DS_Store
rm -rf -- ./docker-http-server/**/**/**/**/.DS_Store

rm -rf -- ./docker-http-server/**/DerivedData
rm -rf -- ./docker-http-server/**/**/DerivedData
rm -rf -- ./docker-http-server/**/**/**/DerivedData
rm -rf -- ./docker-http-server/**/**/**/**/DerivedData

rm -rf -- ./docker-http-server/**/*.xcodeproj
rm -rf -- ./docker-http-server/**/**/*.xcodeproj
rm -rf -- ./docker-http-server/**/**/**/*.xcodeproj
rm -rf -- ./docker-http-server/**/**/**/**/*.xcodeproj

rm -rf ./docker-http-server/networkcommon/libs/macos
rm -rf ./docker-http-server/qh3server/docker
rm -rf ./docker-http-server/qh3server/*.dSYM
rm -rf ./docker-http-server/qh3server/qh3server-app

# build
echo "use 'sudo docker build -t qhttp-server-exp .' to build the docker"
# sudo docker build -t qhttp-server-exp .

# run
echo "use 'sudo docker run --name qhttp-server-container -d qhttp-server-exp' to run the docker"
# sudo docker run --name qhttp-server-container -d qhttp-server-exp

# stop
echo "use 'sudo docker stop qhttp-server-container' to stop the docker"
# sudo docker stop qhttp-server-container

# restart
echo "use 'sudo docker restart qhttp-server-container' to re-start the docker"
# sudo docker restart qhttp-server-container

# REMOVE
echo "use 'sudo docker rm --force qhttp-server-container' to REMOVE the docker"
# sudo docker rm --force qhttp-server-container