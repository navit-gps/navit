# How to create an iOS build

## Requirements

- You'll need a payable developer account at Apple to deploy the software to your device
- Install the macOS requirements for navit
- make sure to have `rsvg-convert` installed as `convert` has issues with some of the icons navit ships with
- Xcode 12.5 (may work with older versions as well but not tested)
- For older devices with arm7 (e.g. iPad2) download https://download.developer.apple.com/Developer_Tools/Xcode_8.3.3/Xcode_8.3.3.xip
- Unzip the file and provide the path to Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer in the cmake command line.
  See Workflow, 7.

## Workflow

1. Clone the navit repository

2. cd into the repository

3. Create a build folder

4. cd into build folder

5. For arm64 devices use:

   ```bash
   cmake -G Xcode ../ -DCMAKE_TOOLCHAIN_FILE=../Toolchain/xcode-iphone_new.cmake -DUSE_PLUGINS=0 -DBUILD_MAPTOOL=0 -DSAMPLE_MAP=0 -DXSLTS=iphone -DUSE_UIKIT=1 -T buildsystem=1 -DDEVELOPMENT_TEAM_ID="<enter your team id>" -DCODE_SIGN_IDENTITY="iPhone Developer"
   ```

   For iOS device with arm7 use an old SDK and this command:

   ```bash
   cmake -G Xcode ../ -DCMAKE_TOOLCHAIN_FILE=../Toolchain/xcode-iphone_new.cmake -DUSE_PLUGINS=0 -DBUILD_MAPTOOL=0 -DSAMPLE_MAP=0 -DXSLTS=iphone -DUSE_UIKIT=1 -T buildsystem=1 -DDEVELOPMENT_TEAM_ID="<enter your team id>" -DCODE_SIGN_IDENTITY="iPhone Developer" -DCMAKE_IOS_DEVELOPER_ROOT="/Volumes/Data/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer" -IOS_ARCH="armv7" -DIPHONEOS_DEPLOYMENT_TARGET="9.2"
   ```
