<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
      package="org.navitproject.navit"
      android:sharedUserId="org.navitproject.navit"
      android:versionCode="@ANDROID_VERSION_INT@" android:versionName="@ANDROID_VERSION_NAME@">
    <application android:label="@string/app_name"
                 android:icon="@drawable/icon">
        <activity android:name="Navit"
                  android:label="@string/app_name"
                  android:theme="@android:style/Theme.NoTitleBar"
		  android:configChanges="locale|touchscreen|keyboard|keyboardHidden|navigation|orientation|fontScale">
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
    </application>
    <uses-sdk android:minSdkVersion="@ANDROID_API_VERSION@" android:targetSdkVersion="7"/>
    <supports-screens android:smallScreens="true" android:normalScreens="true" android:largeScreens="true" android:resizeable="true" android:anyDensity="true"/>
    @ANDROID_PERMISSIONS@
    <uses-permission android:name="android.permission.ACCESS_COARSE_LOCATION" />
    <uses-permission android:name="android.permission.ACCESS_FINE_LOCATION" />
    <uses-permission android:name="android.permission.WAKE_LOCK" />
    <uses-permission android:name="android.permission.INTERNET" />
    <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE"/>
</manifest> 
