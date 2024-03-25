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

set +x
if [ $BUILD_TYPE = "release" ] ; then
   echo "++++++++++++ RELEASE build ++++++++++++"
   echo "++++++++++++ RELEASE build ++++++++++++"
   echo "++++++++++++ RELEASE build ++++++++++++"
fi
set -x

if [ $PREP_DOCKER = true ] ; then
   set +x
   echo "--- PREPARE DOCKER FOLDER ---"
   echo "--- PREPARE DOCKER FOLDER ---"
   echo "--- PREPARE DOCKER FOLDER ---"
   set -x
   cd $WORKSPACE/qserver
   # make clean
   
   cd $WORKSPACE/qserver/docker
   # copy the certs from build. certs required for docker image
   # cp $WORKSPACE/qserver/build/cert.crt $WORKSPACE/qserver/certs/cert.crt
   # cp $WORKSPACE/qserver/build/cert.key $WORKSPACE/qserver/certs/cert.key
   sh ./prepare-docker-qserver-folder.sh
   # remove the build folder from docker folder. Not requird.
   # rm -rf $WORKSPACE/docker-server/qserver/build   
   # zip -r $WORKSPACE/out/docker_qserver_$BUILD_ID.zip $WORKSPACE/docker-server
   
   sed -i "s/192\.168\.0\.230/$SERVICE_SERVER_IP/" $WORKSPACE/docker-server/run_qserver.sh

   if [ $BUILD_DOCKER = true ] ; then
      cd $WORKSPACE/docker-server
      set +x
      echo "--- BUILD DOCKER ---"
      echo "--- BUILD DOCKER ---"
      echo "--- BUILD DOCKER ---"
      set -x
      sudo docker build -t qserver-exp .
      
      if [ $PUBLISH_DOCKER = true ] ; then
         set +x
         echo "--- PUBLISH DOCKER ---"
         echo "--- PUBLISH DOCKER ---"
         echo "--- PUBLISH DOCKER ---"
         set -x
         sudo docker stop qserver-container
         sudo docker rm --force qserver-container
         sudo docker run --publish 4000:4000/udp --name qserver-container --restart=unless-stopped -d qserver-exp
      fi
   fi
fi

WORKSPACE=$CACHE_WORKSPACE