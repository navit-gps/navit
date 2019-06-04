<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
      package="org.navitproject.navit"
      android:sharedUserId="org.navitproject.navit"
      android:versionCode="@ANDROID_VERSION_INT@"
      android:versionName="@ANDROID_VERSION_NAME@-@ANDROID_VERSION_INT@"
      android:installLocation="auto">
    <uses-sdk android:minSdkVersion="7" android:targetSdkVersion="@ANDROID_API_VERSION@"/>
    <uses-feature android:name="android.hardware.location.network" android:required="false"/>
    <uses-feature android:name="android.hardware.touchscreen" android:required="false"/>
    <supports-screens android:smallScreens="true" android:normalScreens="true" android:largeScreens="true" android:resizeable="true" android:anyDensity="true"/>
    @ANDROID_PERMISSIONS@
    <uses-permission android:name="android.permission.ACCESS_COARSE_LOCATION" />
    <uses-permission android:name="android.permission.ACCESS_FINE_LOCATION" />
    <uses-permission android:name="android.permission.WAKE_LOCK" />
    <uses-permission android:name="android.permission.INTERNET" />
    <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE"/>
    <application android:label="@string/app_name"
                 android:icon="@drawable/icon"
                 android:name=".NavitAppConfig"
                 android:theme="@style/NavitBaseTheme">
        <activity android:name="Navit"
                  android:label="@string/app_name"
                  android:configChanges="locale|touchscreen|keyboard|keyboardHidden|navigation|orientation|fontScale|screenSize"
                  android:theme="@style/NavitTheme">
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
            <intent-filter>
                <action android:name="android.intent.action.VIEW" />
                <category android:name="android.intent.category.DEFAULT" />
                <data android:scheme="google.navigation" />
            </intent-filter>
        </activity>
        <activity android:name=".NavitAddressSearchActivity"></activity>
        <activity android:name=".NavitDownloadSelectMapActivity"></activity>
        <activity android:name=".NavitAddressResultListActivity"></activity>
        <activity android:name=".FileBrowserActivity"></activity>
    </application>
</manifest>
