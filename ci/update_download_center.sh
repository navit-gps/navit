#!/bin/bash


#############################################
#############     Functions     #############
#############################################

cleanup(){
    if [ -d $TMP_DIR ]; then
        rm -rf $TMP_DIR
    fi
}

#############################################
#############   Functions END   #############
#############################################

echo "Check the requirements"
if [ -z $CI ];then
    echo "This Script needs to be run by CI"
fi
if [ -z $CIRCLECI ];then
    echo "This Script needs to be run on CircleCI"
fi

#############################################

echo "Setting up all Variables"

UUID=$(uuidgen)
TMP_DIR=$(mktemp -d)
CIRCLECI_API_BASE="https://circleci.com/api/v1.1/"
NAVIT_DOWNLOAD_CENTER_REPO="https://github.com/jkoan/data-test"

# To keep it generic
CVS_TYPE="github"
REPONAME=$CIRCLE_PROJECT_REPONAME
USERNAME=$CIRCLE_PROJECT_USERNAME
BUILD_NUM=$CIRCLE_BUILD_NUM

# Build all api urls
URL_BUILD_ARTIFACTS="${CIRCLECI_API_BASE}${CVS_TYPE}/${USERNAME}/${REPONAME}/${BUILD_NUM}/artifacts"
echo $URL_BUILD_ARTIFACTS
exit

#############################################

echo "Setup trap for cleanup"
trap cleanup EXIT

#############################################

echo "Init Git Repo"
git checkout $NAVIT_DOWNLOAD_CENTER_REPO $UUID
cd $UUID

