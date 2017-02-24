ARCH="arm"

cd ~/
git clone git@github.com:navit-gps/infrastructure-blackbox.git
cd infrastructure-blackbox/keyrings/
openssl aes-256-cbc -d -in keystore.gpg -k $KEY > ~/.keystore
openssl aes-256-cbc -d -in client_secrets.gpg -k $KEY > ~/navit/ci/client_secrets.json
openssl aes-256-cbc -d -in androidpublisher.gpg -k $KEY > androidpublisher.dat

pip install google-api-python-client

jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -storepass $SP $CIRCLE_ARTIFACTS/navit-$CIRCLE_SHA1-${ARCH}-release-unsigned.apk $key_name

/usr/local/android-sdk-linux/build-tools/25.0.1/zipalign 4 $CIRCLE_ARTIFACTS/navit-$CIRCLE_SHA1-${ARCH}-release-unsigned.apk $CIRCLE_ARTIFACTS/navit-$CIRCLE_SHA1-${ARCH}-release-signed.apk
# /usr/local/android-sdk-linux/build-tools/25.0.1/apksigner sign -v --ks-pass pass:$SP --key $key_name $CIRCLE_ARTIFACTS/navit-$CIRCLE_SHA1-${ARCH}-release-signed.apk 
/usr/local/android-sdk-linux/build-tools/25.0.1/apksigner verify -v $CIRCLE_ARTIFACTS/navit-$CIRCLE_SHA1-${ARCH}-release-signed.apk
python ~/navit/ci/basic_upload_apks.py org.navitproject.navit $CIRCLE_ARTIFACTS/navit-$CIRCLE_SHA1-${ARCH}-release-signed.apk
