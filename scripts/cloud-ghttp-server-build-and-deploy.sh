#!/bin/bash
echo branch $BRANCH
# pwd
# ls
branch_name="${BRANCH#origin/}"
echo branch name $branch_name
root_dir="/home/ubuntu"
deploy_dir="$root_dir/deploy-build"
echo "Deploy directory "$deploy_dir
# rm -rf "$deploy_dir/gsdk-source"
# mkdir "$deploy_dir/gsdk-source"
git clone git@github.com:arunmudhaliar/gearsdk.git "$deploy_dir/gsdk-source"
cd "$deploy_dir/gsdk-source"
pwd

git fetch origin
git branch -r
git checkout $BRANCH
git reset --hard
git pull origin $branch_name

CACHE_WORKSPACE=$WORKSPACE
WORKSPACE=$deploy_dir/gsdk-source
echo "WORKSPACE directory "$WORKSPACE

if [ $PREP_DOCKER = true ] ; then
   set +x
   echo "--- PREPARE DOCKER FOLDER ---"
   echo "--- PREPARE DOCKER FOLDER ---"
   echo "--- PREPARE DOCKER FOLDER ---"
   set -x
   cd $WORKSPACE/qh3server
   # make clean
   
   cd $WORKSPACE/qh3server/docker
   # copy the certs from build. certs required for docker image
   # cp $WORKSPACE/qh3server/build/cert.crt $WORKSPACE/qh3server/certs/cert.crt
   # cp $WORKSPACE/qh3server/build/cert.key $WORKSPACE/qh3server/certs/cert.key
   sh ./prepare-docker-qh3server-folder.sh
   # remove the build folder from docker folder. Not requird.
   # rm -rf $WORKSPACE/docker-qh3server/qh3server/build   
   # zip -r $WORKSPACE/out/docker_qh3server_$BUILD_ID.zip $WORKSPACE/docker-qh3server

   sed -i "s/192\.168\.0\.230/$SERVICE_SERVER_IP/" $WORKSPACE/docker-qh3server/run_qh3server.sh

   if [ $BUILD_DOCKER = true ] ; then
      cd $WORKSPACE/docker-qh3server
      set +x
      echo "--- BUILD DOCKER ---"
      echo "--- BUILD DOCKER ---"
      echo "--- BUILD DOCKER ---"
      set -x
      sudo docker build -t qh3server-exp .
      
      if [ $PUBLISH_DOCKER = true ] ; then
         set +x
         echo "--- PUBLISH DOCKER ---"
         echo "--- PUBLISH DOCKER ---"
         echo "--- PUBLISH DOCKER ---"
         set -x
         sudo docker stop qh3server-container
         sudo docker rm --force qh3server-container
#         docker run --publish 4004:4004/udp --name qh3server-container -d qh3server-exp
		 sudo docker run --publish 4004:4004/udp --publish 4010:4010/udp --publish 4011:4011/udp --publish 5100:5100/udp --publish 5101:5101/udp --publish 5102:5102/udp --publish 5103:5103/udp --publish 5104:5104/udp --name qh3server-container --restart unless-stopped -d qh3server-exp
      fi
   fi
fi

WORKSPACE=$CACHE_WORKSPACE