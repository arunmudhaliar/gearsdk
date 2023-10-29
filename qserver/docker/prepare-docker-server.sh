cd ../../
rm -rf ./docker-server

mkdir ./docker-server
cp -r ./common ./docker-server/common
cp -r ./networkcommon/ ./docker-server/networkcommon
cp -r ./qserver/ ./docker-server/qserver
cp ./qserver/docker/Dockerfile ./docker-server/Dockerfile
cp ./qserver/docker/make_qserver.sh ./docker-server/make_qserver.sh

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
rm -rf ./docker-server/qserver/docker
rm -rf ./docker-server/qserver/*.dSYM
rm -rf ./docker-server/qserver/qserver-app

echo "use 'docker build -t qserver-exp .' to build the docker"
# docker build -t qserver-exp .
echo "use 'docker run -d qserver-exp' to run the docker"
# docker run -d qserver-exp