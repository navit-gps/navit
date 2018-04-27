#!/bin/bash
set -e

apt-get update && apt-get install -y software-properties-common
add-apt-repository -y ppa:openjdk-r/ppa
apt-get update && apt-get install -y openjdk-8-jdk wget expect curl libsaxonb-java ant ca-certificates python-pip
apt-get remove -y openjdk-7-jre-headless

export ANDROID_SDK_HOME=/opt/android-sdk-linux
export ANDROID_HOME=/opt/android-sdk-linux

cd /opt && wget -q https://dl.google.com/android/android-sdk_r24.4.1-linux.tgz -O android-sdk.tgz
cd /opt && tar -xvzf android-sdk.tgz --no-same-owner
cd /opt && rm -f android-sdk.tgz

export PATH=${PATH}:${ANDROID_SDK_HOME}/tools:${ANDROID_SDK_HOME}/platform-tools:/opt/tools

echo y | android update sdk --no-ui --all --filter platform-tools | grep 'package installed'

# This is only an workaround to make sure the platform tools are installed
if [ ! -d ${ANDROID_SDK_HOME}/platform-tools ] && [ -f ${ANDROID_SDK_HOME}/temp/platform-tools_r26.0.2-linux.zip ]; then
	if [ "$(md5sum ${ANDROID_SDK_HOME}/temp/platform-tools_r26.0.2-linux.zip | cut -d" " -f1)" == "ef952bb31497f7535e061ad0e712bed8" ]; then
		cd ${ANDROID_SDK_HOME} && unzip ${ANDROID_SDK_HOME}/temp/platform-tools_r26.0.2-linux.zip
	fi
fi

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

