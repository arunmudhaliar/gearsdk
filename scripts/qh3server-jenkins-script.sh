git branch
rm -rf $WORKSPACE/out
rm -rf $WORKSPACE/qh3server/build

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

mkdir $WORKSPACE/qh3server/build
mv $WORKSPACE/qh3server/qh3server-app $WORKSPACE/qh3server/build/qh3server-app
#mv $WORKSPACE/qh3server/qh3client-app $WORKSPACE/qh3server/build/qh3client-app
mv $WORKSPACE/qh3server/certs/cert.crt $WORKSPACE/qh3server/build/cert.crt
mv $WORKSPACE/qh3server/certs/cert.key $WORKSPACE/qh3server/build/cert.key

mkdir $WORKSPACE/out
zip -r $WORKSPACE/out/build_$BUILD_ID.zip $WORKSPACE/qh3server/build/

echo
echo
echo

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
   cp $WORKSPACE/qh3server/build/cert.crt $WORKSPACE/qh3server/certs/cert.crt
   cp $WORKSPACE/qh3server/build/cert.key $WORKSPACE/qh3server/certs/cert.key
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
         docker run --publish 4004:4004/udp --name qh3server-container -d qh3server-exp
      fi
   fi
fi