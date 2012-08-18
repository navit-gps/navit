package org.navitproject.navit;

import java.util.ArrayList;
import java.util.List;

import org.navitproject.navit.NavitAddressSearchActivity.NavitAddress;

import android.app.Application;
import android.content.SharedPreferences;
import android.util.Log;

import org.acra.annotation.*;

@ReportsCrashes(formKey = "dGlrNVRIOVVKYjB0UGVoLUZPanlzWFE6MQ")
public class NavitAppConfig extends Application {

	private static final int         MAX_LAST_ADDRESSES = 10;
	private static final String      TAG                = "Navit";

	private List<NavitAddress> mLastAddresses     = null;
	private int                      mLastAddressField;
	private SharedPreferences        mSettings;

	@Override
	public void onCreate() {
		// call ACRA.init(this) as reflection, because old ant may forgot to include it
		try {
			Class<?> acraClass = Class.forName("org.acra.ACRA");
			Class<?> partypes[] = new Class[1];
			partypes[0] = Application.class;
			java.lang.reflect.Method initMethod = acraClass.getMethod("init", partypes);
			Object arglist[] = new Object[1];
			arglist[0] = this;
			initMethod.invoke(null, arglist);
		} catch (Exception e1) {
			Log.e(TAG, "Could not init ACRA crash reporter");
		}

		mSettings = getSharedPreferences(Navit.NAVIT_PREFS, MODE_PRIVATE);
		super.onCreate();
	}

	public List<NavitAddress> getLastAddresses() {
		if (mLastAddresses == null) {
			mLastAddresses = new ArrayList<NavitAddress>();
			int mLastAddressField = mSettings.getInt("LastAddress", -1);
			if (mLastAddressField >= 0) {
				int index = mLastAddressField;
				do {
					String addr_str = mSettings.getString("LastAddress_" + String.valueOf(index), "");

					if (addr_str.length() > 0) {
						mLastAddresses.add(new NavitAddress(
						        1,
						        mSettings.getFloat("LastAddress_Lat_" + String.valueOf(index), 0),
						        mSettings.getFloat("LastAddress_Lon_" + String.valueOf(index), 0),
						        addr_str));
					}

					if (--index < 0) index = MAX_LAST_ADDRESSES - 1;

				} while (index != mLastAddressField);
			}
		}
		return mLastAddresses;
	}

	public void addLastAddress(NavitAddress newAddress) {
		getLastAddresses();

		mLastAddresses.add(newAddress);
		if (mLastAddresses.size() > MAX_LAST_ADDRESSES) mLastAddresses.remove(0);

		mLastAddressField++;
		if (mLastAddressField >= MAX_LAST_ADDRESSES) mLastAddressField = 0;

		SharedPreferences.Editor editSettings = mSettings.edit();

		editSettings.putInt("LastAddress", mLastAddressField);
		editSettings.putString("LastAddress_" + String.valueOf(mLastAddressField), newAddress.addr);
		editSettings.putFloat("LastAddress_Lat_" + String.valueOf(mLastAddressField), newAddress.lat);
		editSettings.putFloat("LastAddress_Lon_" + String.valueOf(mLastAddressField), newAddress.lon);

		editSettings.commit();
	}
}
