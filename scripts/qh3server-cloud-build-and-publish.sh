export WORKSPACE=$1
export BUILD_EXECUTABLE=false
export BUILD_TYPE="release"
export BUILD_ID=0
export PREP_DOCKER=true
export BUILD_DOCKER=true
export PUBLISH_DOCKER=true

sh ./qh3server-jenkins-script.sh