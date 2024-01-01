git branch
rm -rf $WORKSPACE/out
rm -rf $WORKSPACE/qh3server/build
mkdir $WORKSPACE/qh3server/build
mkdir $WORKSPACE/out

if [ $BUILD_EXECUTABLE = true ] ; then
  set +x
  if [ $BUILD_TYPE = "release" ] ; then
     echo "++++++++++++ RELEASE build ++++++++++++"
     echo "++++++++++++ RELEASE build ++++++++++++"
     echo "++++++++++++ RELEASE build ++++++++++++"
  fi
  set -x
  
  cd $WORKSPACE/qh3server
  make clean
  make $BUILD_TYPE
  
  # cd $WORKSPACE/qh3client
  # make clean
  # make $BUILD_TYPE
 
  mv $WORKSPACE/qh3server/qh3server-app $WORKSPACE/qh3server/build/qh3server-app
  #mv $WORKSPACE/qh3server/qh3client-app $WORKSPACE/qh3server/build/qh3client-app
  cp $WORKSPACE/qh3server/certs/cert.crt $WORKSPACE/qh3server/build/cert.crt
  cp $WORKSPACE/qh3server/certs/cert.key $WORKSPACE/qh3server/build/cert.key
  
  zip -r $WORKSPACE/out/build_$BUILD_ID.zip $WORKSPACE/qh3server/build/
  echo
  echo
  echo
fi

if [ $PREP_DOCKER = true ] ; then
   set +x
   echo "--- PREPARE DOCKER FOLDER ---"
   echo "--- PREPARE DOCKER FOLDER ---"
   echo "--- PREPARE DOCKER FOLDER ---"
   set -x
   cd $WORKSPACE/qh3server
   make clean
   
   cd $WORKSPACE/qh3server/docker
   # copy the certs from build. certs required for docker image
   # cp $WORKSPACE/qh3server/build/cert.crt $WORKSPACE/qh3server/certs/cert.crt
   # cp $WORKSPACE/qh3server/build/cert.key $WORKSPACE/qh3server/certs/cert.key
   sh ./prepare-docker-qh3server-folder.sh
   # remove the build folder from docker folder. Not requird.
   rm -rf $WORKSPACE/docker-qh3server/qh3server/build   
   zip -r $WORKSPACE/out/docker_qh3server_$BUILD_ID.zip $WORKSPACE/docker-qh3server
   
   if [ $BUILD_DOCKER = true ] ; then
      cd $WORKSPACE/docker-qh3server
      set +x
      echo "--- BUILD DOCKER ---"
      echo "--- BUILD DOCKER ---"
      echo "--- BUILD DOCKER ---"
      set -x
      docker build -t qh3server-exp .
      
      if [ $PUBLISH_DOCKER = true ] ; then
         set +x
         echo "--- PUBLISH DOCKER ---"
         echo "--- PUBLISH DOCKER ---"
         echo "--- PUBLISH DOCKER ---"
         set -x
         docker stop qh3server-container
         docker rm --force qh3server-container
#         docker run --publish 4004:4004/udp --name qh3server-container -d qh3server-exp
		 docker run --publish 4004:4004/udp --publish 4010:4010/udp --publish 4011:4011/udp --publish 5100:5100/udp --publish 5101:5101/udp --publish 5102:5102/udp --publish 5103:5103/udp --publish 5104:5104/udp --name qh3server-container -d qh3server-exp
      fi
   fi
fi