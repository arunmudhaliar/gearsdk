git branch
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
   
   cd $WORKSPACE/qstats-crawler/docker
   sh ./prepare-docker-qstats-crawler-folder.sh
   # remove the build folder from docker folder. Not requird.
   rm -rf $WORKSPACE/docker-qstats-crawler/qstats-crawler/build   
   
   if [ $BUILD_DOCKER = true ] ; then
      cd $WORKSPACE/docker-qstats-crawler
      set +x
      echo "--- BUILD DOCKER ---"
      echo "--- BUILD DOCKER ---"
      echo "--- BUILD DOCKER ---"
      set -x
      docker build -t qstats-crawler-exp .
      
      if [ $PUBLISH_DOCKER = true ] ; then
         set +x
         echo "--- PUBLISH DOCKER ---"
         echo "--- PUBLISH DOCKER ---"
         echo "--- PUBLISH DOCKER ---"
         set -x
         docker stop qstats-crawler-container
         docker rm --force qstats-crawler-container
         docker run --name qstats-crawler-container -d qstats-crawler-exp
      fi
   fi
fi