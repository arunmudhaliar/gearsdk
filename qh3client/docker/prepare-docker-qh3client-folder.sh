cd ../../
docker_dir="docker-qh3client"
rm -rf ./$docker_dir

mkdir ./$docker_dir

# qh3server files
cp -r ./common ./$docker_dir/common
cp -r ./networkcommon/ ./$docker_dir/networkcommon
# cp -r ./qh3server/ ./$docker_dir/qh3server
cp -r ./qh3client/ ./$docker_dir/qh3client
# cp -r ./qhiredis/ ./$docker_dir/qhiredis
# cp -r ./qstats-crawler/ ./$docker_dir/qstats-crawler
# cp -r ./servercommon/ ./$docker_dir/servercommon
# cp -r ./qzookeeper/ ./$docker_dir/qzookeeper
cp ./qh3client/docker/Dockerfile ./$docker_dir/Dockerfile
cp ./qh3client/docker/make_qh3client.sh ./$docker_dir/make_qh3client.sh
cp ./qh3client/docker/run_qh3client.sh ./$docker_dir/run_qh3client.sh

# supervisor
# cp ./qh3server/docker/supervisord.conf ./$docker_dir/supervisord.conf

# qstats-crawler files
# cp ./qstats-crawler/docker/make_qstats-crawler.sh ./$docker_dir/make_qstats-crawler.sh
# cp ./qstats-crawler/docker/run_qstats-crawler.sh ./$docker_dir/run_qstats-crawler.sh
# cp ./qstats-crawler/docker/make_postgresql.sh ./$docker_dir/make_postgresql.sh

# $docker_dir
rm -rf -- ./$docker_dir/**/*.o
rm -rf -- ./$docker_dir/**/**/*.o
rm -rf -- ./$docker_dir/**/**/**/*.o
rm -rf -- ./$docker_dir/**/**/**/**/*.o

rm -rf -- ./$docker_dir/**/.DS_Store
rm -rf -- ./$docker_dir/**/**/.DS_Store
rm -rf -- ./$docker_dir/**/**/**/.DS_Store
rm -rf -- ./$docker_dir/**/**/**/**/.DS_Store

rm -rf -- ./$docker_dir/**/DerivedData
rm -rf -- ./$docker_dir/**/**/DerivedData
rm -rf -- ./$docker_dir/**/**/**/DerivedData
rm -rf -- ./$docker_dir/**/**/**/**/DerivedData

rm -rf -- ./$docker_dir/**/*.xcodeproj
rm -rf -- ./$docker_dir/**/**/*.xcodeproj
rm -rf -- ./$docker_dir/**/**/**/*.xcodeproj
rm -rf -- ./$docker_dir/**/**/**/**/*.xcodeproj

rm -rf ./$docker_dir/networkcommon/libs/macos
rm -rf ./$docker_dir/common/libs/macos
# rm -rf ./$docker_dir/qhiredis/libs/macos
# rm -rf ./$docker_dir/qstats-crawler/libs/macos
# rm -rf ./$docker_dir/servercommon/libs/macos
# rm -rf ./$docker_dir/qzookeeper/libs/macos
rm -rf ./$docker_dir/qh3client/docker
rm -rf ./$docker_dir/qh3client/*.dSYM
rm -rf ./$docker_dir/qh3client/qh3client-app

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