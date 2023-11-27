cd ../../
rm -rf ./docker-qstats-crawler

mkdir ./docker-qstats-crawler
cp -r ./common ./docker-qstats-crawler/common
cp -r ./networkcommon/ ./docker-qstats-crawler/networkcommon
cp -r ./qstats-crawler/ ./docker-qstats-crawler/qstats-crawler
cp -r ./qhiredis/ ./docker-qstats-crawler/qhiredis
cp -r ./servercommon/ ./docker-qstats-crawler/servercommon
cp ./qstats-crawler/docker/Dockerfile ./docker-qstats-crawler/Dockerfile
cp ./qstats-crawler/docker/make_qstats-crawler.sh ./docker-qstats-crawler/make_qstats-crawler.sh
cp ./qstats-crawler/docker/run_qstats-crawler.sh ./docker-qstats-crawler/run_qstats-crawler.sh

# docker-qstats-crawler
rm -rf -- ./docker-qstats-crawler/**/*.o
rm -rf -- ./docker-qstats-crawler/**/**/*.o
rm -rf -- ./docker-qstats-crawler/**/**/**/*.o
rm -rf -- ./docker-qstats-crawler/**/**/**/**/*.o

rm -rf -- ./docker-qstats-crawler/**/.DS_Store
rm -rf -- ./docker-qstats-crawler/**/**/.DS_Store
rm -rf -- ./docker-qstats-crawler/**/**/**/.DS_Store
rm -rf -- ./docker-qstats-crawler/**/**/**/**/.DS_Store

rm -rf -- ./docker-qstats-crawler/**/DerivedData
rm -rf -- ./docker-qstats-crawler/**/**/DerivedData
rm -rf -- ./docker-qstats-crawler/**/**/**/DerivedData
rm -rf -- ./docker-qstats-crawler/**/**/**/**/DerivedData

rm -rf -- ./docker-qstats-crawler/**/*.xcodeproj
rm -rf -- ./docker-qstats-crawler/**/**/*.xcodeproj
rm -rf -- ./docker-qstats-crawler/**/**/**/*.xcodeproj
rm -rf -- ./docker-qstats-crawler/**/**/**/**/*.xcodeproj

rm -rf ./docker-qstats-crawler/networkcommon/libs/macos
rm -rf ./docker-qstats-crawler/common/libs/macos
rm -rf ./docker-qstats-crawler/qhiredis/libs/macos
rm -rf ./docker-qstats-crawler/servercommon/libs/macos
rm -rf ./docker-qstats-crawler/qstats-crawler/libs/macos
rm -rf ./docker-qstats-crawler/qstats-crawler/docker
rm -rf ./docker-qstats-crawler/qstats-crawler/*.dSYM
rm -rf ./docker-qstats-crawler/qstats-crawler/qstats-crawler-app

# build
echo "use 'sudo docker build -t qstats-crawler-exp .' to build the docker"
# sudo docker build -t qstats-crawler.

# run
echo "use 'sudo docker run --name qstats-crawler-container -d qstats-crawler-exp' to run the docker"
# sudo docker run --name qstats-crawler-container -d qstats-crawler-exp

# stop
echo "use 'sudo docker stop qstats-crawler-container' to stop the docker"
# sudo docker stop qstats-crawler-container

# restart
echo "use 'sudo docker restart qstats-crawler-container' to re-start the docker"
# sudo docker restart qstats-crawler-container

# REMOVE
echo "use 'sudo docker rm --force qstats-crawler-container' to REMOVE the docker"
# sudo docker rm --force qstats-crawler-container