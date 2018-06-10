#!/usr/bin/env bash

if [ -z $CI ];then
    echo "This Script needs to be run by CI"
fi
if [ -z $CIRCLECI ];then
    echo "This Script needs to be run on CircleCI"
fi
if [[ "${CIRCLE_PROJECT_USERNAME}" != "navit-gps" || "${CIRCLE_BRANCH}" != "trunk" ]]; then
    echo "Only trunk on navit-gps may upload to the Play Store"
    exit 0
fi

set -e

ARCH="arm"
BINPATH="android-arm/navit/android/bin"

export PATH="/opt/android-sdk-linux/build-tools/25.0.1/":$PATH

cd ~/
git clone git@github.com:navit-gps/infrastructure-blackbox.git
cd infrastructure-blackbox/keyrings/
openssl aes-256-cbc -d -in keystore.gpg -k $KEY > ~/.keystore
openssl aes-256-cbc -d -in client_secrets.gpg -k $KEY > /root/project/scripts/client_secrets.json
openssl aes-256-cbc -d -in androidpublisher.gpg -k $KEY > /root/project/androidpublisher.dat
cd ~/project

pip install google-api-python-client

jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -storepass $SP $BINPATH/navit-$CIRCLE_SHA1-${ARCH}-release-unsigned.apk $key_name

zipalign 4 $BINPATH/navit-$CIRCLE_SHA1-${ARCH}-release-unsigned.apk $BINPATH/navit-$CIRCLE_SHA1-${ARCH}-release-signed.apk
apksigner verify -v $BINPATH/navit-$CIRCLE_SHA1-${ARCH}-release-signed.apk
python ~/project/scripts/basic_upload_apks.py org.navitproject.navit $BINPATH/navit-$CIRCLE_SHA1-${ARCH}-release-signed.apk --noauth_local_webserver
