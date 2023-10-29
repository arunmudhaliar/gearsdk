rm -rf $WORKSPACE/out
rm -rf $WORKSPACE/qserver/build

set +x
if [ $BUILD_TYPE = "release" ] ; then
   echo "++++++++++++ RELEASE build ++++++++++++"
   echo "++++++++++++ RELEASE build ++++++++++++"
   echo "++++++++++++ RELEASE build ++++++++++++"
fi
set -x

cd $WORKSPACE/qserver
make clean
make $BUILD_TYPE

cd $WORKSPACE/qclient
make clean
make $BUILD_TYPE

mkdir $WORKSPACE/qserver/build
mv $WORKSPACE/qserver/qserver-app $WORKSPACE/qserver/build/qserver-app
mv $WORKSPACE/qclient/qclient-app $WORKSPACE/qserver/build/qclient-app
mv $WORKSPACE/qserver/certs/cert.crt $WORKSPACE/qserver/build/cert.crt
mv $WORKSPACE/qserver/certs/cert.key $WORKSPACE/qserver/build/cert.key

mkdir $WORKSPACE/out
zip -r $WORKSPACE/out/build_$BUILD_ID.zip $WORKSPACE/qserver/build/

echo
echo
echo

if [ $PREP_DOCKER = true ] ; then
   set +x
   echo "--- PREPARE DOCKER FOLDER ---"
   echo "--- PREPARE DOCKER FOLDER ---"
   echo "--- PREPARE DOCKER FOLDER ---"
   set -x
   cd $WORKSPACE/qserver
   make clean
   
   cd $WORKSPACE/qserver/docker
   # copy the certs from build. certs required for docker image
   cp $WORKSPACE/qserver/build/cert.crt $WORKSPACE/qserver/certs/cert.crt
   cp $WORKSPACE/qserver/build/cert.key $WORKSPACE/qserver/certs/cert.key
   sh ./prepare-docker-qserver-folder.sh
   # remove the build folder from docker folder. Not requird.
   rm -rf $WORKSPACE/docker-server/qserver/build   
   zip -r $WORKSPACE/out/docker_qserver_$BUILD_ID.zip $WORKSPACE/docker-server
   
   if [ $BUILD_DOCKER = true ] ; then
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
         sudo docker run --name qserver-container -d qserver-exp
      fi
   fi
fi