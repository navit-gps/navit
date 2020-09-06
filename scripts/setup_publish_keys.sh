#!/bin/bash

set -e

if [[ -n $GOOGLE_KEY ]]; then
  echo $GOOGLE_KEY | base64 -d > key.json
fi
if [[ -n $KEY ]]; then
  wget "https://github.com/navit-gps/infrastructure-blackbox/raw/master/keyrings/keystore.gpg"
  openssl aes-256-cbc -d -in keystore.gpg -md md5 -k $KEY > keystore
  keytool -importkeystore -srcstorepass "$STORE_PASS" -deststorepass "$STORE_PASS" -srckeystore keystore -destkeystore ~/.keystore -deststoretype pkcs12
  rm keystore.gpg keystore
fi
