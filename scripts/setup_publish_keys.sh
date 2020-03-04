#!/bin/bash

set -e

echo $GOOGLE_KEY | base64 -d > key.json

wget "https://github.com/navit-gps/infrastructure-blackbox/raw/master/keyrings/keystore.gpg"
openssl aes-256-cbc -d -in keystore.gpg -md md5 -k $KEY > ~/.keystore
keytool -importkeystore -srcstorepass "$STORE_PASS" -deststorepass "$STORE_PASS" -srckeystore /home/circleci/.keystore -destkeystore /home/circleci/.keystore -deststoretype pkcs12
keytool -keypasswd -alias $KEY_ALIAS -storepass $STORE_PASS -new $KEY_PASS -keystore ~/.keystore
rm keystore.gpg
