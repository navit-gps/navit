#!/bin/bash

set -e

release_keystore="${KEYSTORE:-}"

if [[ -n "${GOOGLE_KEY:-}" ]]; then
  echo "$GOOGLE_KEY" | base64 -d > key.json
fi
if [[ -n "${KEY:-}" ]]; then
  wget "https://github.com/navit-gps/infrastructure-blackbox/raw/master/keyrings/keystore.gpg"
  openssl aes-256-cbc -d -in keystore.gpg -md md5 -k "$KEY" > keystore
  keytool -importkeystore -srcstorepass "$STORE_PASS" -deststorepass "$STORE_PASS" -srckeystore keystore -destkeystore ~/.keystore -deststoretype pkcs12
  rm keystore.gpg keystore
  release_keystore="$HOME/.keystore"
  if [[ -n "${GITHUB_ENV:-}" ]]; then
    echo "KEYSTORE=$release_keystore" >> "$GITHUB_ENV"
  fi
fi
if [[ -n "${GITHUB_ENV:-}" && ! -f "${release_keystore:-/nonexistent}" ]]; then
  release_keystore="${RUNNER_TEMP:-/tmp}/navit-ci-keystore.jks"
  rm -f "$release_keystore"
  keytool -genkeypair -v \
    -keystore "$release_keystore" \
    -alias navit-ci \
    -keyalg RSA \
    -keysize 2048 \
    -validity 3650 \
    -storepass navit-ci \
    -keypass navit-ci \
    -dname "CN=Navit CI, OU=CI, O=Navit, L=Unknown, ST=Unknown, C=DE"
  {
    echo "KEYSTORE=$release_keystore"
    echo "KEY_ALIAS=navit-ci"
    echo "STORE_PASS=navit-ci"
  } >> "$GITHUB_ENV"
fi
