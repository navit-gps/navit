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
if [[ "${CIRCLE_PROJECT_USERNAME}" == "navit-gps" && "${CIRCLE_BRANCH}" == "trunk" ]]; then
    echo "Only trunk on navit-gps may upload to the Download Center"
    exit 0
fi

#############################################

echo "Setting up all Variables"

UUID=${RANDOM}-${RANDOM}-${RANDOM}-${RANDOM}
TMP_DIR=$(mktemp -d)
CIRCLECI_API_BASE="https://circleci.com/api/v1.1/"
NAVIT_DOWNLOAD_CENTER_REPO="https://github.com/jkoan/data-test"

# To keep it generic
CVS_TYPE="github"
REPONAME=$CIRCLE_PROJECT_REPONAME
USERNAME=$CIRCLE_PROJECT_USERNAME
BUILD_NUM=$CIRCLE_BUILD_NUM

# Build all api urls
URL_BUILD_ARTIFACTS="${CIRCLECI_API_BASE}project/${CVS_TYPE}/${USERNAME}/${REPONAME}/${BUILD_NUM}/artifacts"
echo "Artifact URL: $URL_BUILD_ARTIFACTS"

#############################################

echo "Setup trap for cleanup"
trap cleanup EXIT

#############################################

echo "Init Git Repo"
cd $TMP_DIR
git clone $NAVIT_DOWNLOAD_CENTER_REPO $UUID
cd $UUID/_data
wget --no-check-certificate $URL_BUILD_ARTIFACTS -O ${BUILD_NUM}.json
RC=$?
if [ $RC -ne 0 ]; then
    echo "wget artifacts failed"
    exit 1
fi
git config --global push.default simple
git config user.name "Circle CI"
git config user.email "circleci@navit-project.org"
git add ${BUILD_NUM}.json
git commit -m "add:artifacts:Add artifacts for build #${BUILD_NUM} with SHA1:${CIRCLE_SHA1} [ci skip]" 
git push
