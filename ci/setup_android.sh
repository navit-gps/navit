apt-get update && apt-get install -y openjdk-7-jdk wget expect git curl

export ANDROID_SDK_HOME=/opt/android-sdk-linux
export ANDROID_HOME=/opt/android-sdk-linux

cd /opt && wget -q https://dl.google.com/android/android-sdk_r24.4.1-linux.tgz -O android-sdk.tgz
cd /opt && tar -xvzf android-sdk.tgz
cd /opt && rm -f android-sdk.tgz

export PATH=${PATH}:${ANDROID_SDK_HOME}/tools:${ANDROID_SDK_HOME}/platform-tools:/opt/tools

echo y | android update sdk --no-ui --all --filter platform-tools | grep 'package installed'
#RUN echo y | android update sdk --no-ui --all --filter extra-android-support | grep 'package installed'

echo y | android update sdk --no-ui --all --filter android-25 | grep 'package installed'
echo y | android update sdk --no-ui --all --filter android-24 | grep 'package installed'
echo y | android update sdk --no-ui --all --filter android-23 | grep 'package installed'
echo y | android update sdk --no-ui --all --filter android-18 | grep 'package installed'
echo y | android update sdk --no-ui --all --filter android-16 | grep 'package installed'

echo y | android update sdk --no-ui --all --filter build-tools-25.0.3 | grep 'package installed'
echo y | android update sdk --no-ui --all --filter build-tools-25.0.2 | grep 'package installed'
echo y | android update sdk --no-ui --all --filter build-tools-25.0.1 | grep 'package installed'
echo y | android update sdk --no-ui --all --filter build-tools-25.0.0 | grep 'package installed'
echo y | android update sdk --no-ui --all --filter build-tools-24.0.3 | grep 'package installed'
echo y | android update sdk --no-ui --all --filter build-tools-24.0.2 | grep 'package installed'
echo y | android update sdk --no-ui --all --filter build-tools-24.0.1 | grep 'package installed'
echo y | android update sdk --no-ui --all --filter build-tools-23.0.3 | grep 'package installed'
echo y | android update sdk --no-ui --all --filter build-tools-23.0.2 | grep 'package installed'
echo y | android update sdk --no-ui --all --filter build-tools-23.0.1 | grep 'package installed'

