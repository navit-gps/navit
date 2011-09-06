/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

package org.navitproject.navit;

import android.content.Context;
import android.location.Criteria;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;
import android.util.Log;

public class NavitVehicle {
	private static LocationManager sLocationManager = null;
	private int vehicle_callbackid;
	private String preciseProvider;
	private String fastProvider;

	private static NavitLocationListener preciseLocationListener = null;
	private static NavitLocationListener fastLocationListener = null;

	public native void VehicleCallback(int id, Location location);

	private class NavitLocationListener implements LocationListener {
		public boolean precise = false;
		public void onLocationChanged(Location location) {
			
			// Disable the fast provider if still active
			if (precise && fastProvider != null) {
				sLocationManager.removeUpdates(fastLocationListener);
				fastProvider = null;
			}
			
			VehicleCallback(vehicle_callbackid, location);
		}
		public void onProviderDisabled(String provider){}
		public void onProviderEnabled(String provider) {}
		public void onStatusChanged(String provider, int status, Bundle extras) {}
	}

	NavitVehicle (Context context, int callbackid) {
		sLocationManager = (LocationManager)context.getSystemService(Context.LOCATION_SERVICE);
		preciseLocationListener = new NavitLocationListener();
		preciseLocationListener.precise = true;
		fastLocationListener = new NavitLocationListener();

		/* Use 2 LocationProviders, one precise (usually GPS), and one
		   not so precise, but possible faster. The fast provider is
		   disabled when the precise provider gets its first fix. */

		// Selection criteria for the precise provider
		Criteria highCriteria = new Criteria();
		highCriteria.setAccuracy(Criteria.ACCURACY_FINE);
		highCriteria.setAltitudeRequired(true);
		highCriteria.setBearingRequired(true);
		highCriteria.setCostAllowed(true);
		highCriteria.setPowerRequirement(Criteria.POWER_HIGH);

		// Selection criteria for the fast provider
		Criteria lowCriteria = new Criteria();
		lowCriteria.setAccuracy(Criteria.ACCURACY_COARSE);
		lowCriteria.setAltitudeRequired(false);
		lowCriteria.setBearingRequired(false);
		lowCriteria.setCostAllowed(true);
		lowCriteria.setPowerRequirement(Criteria.POWER_HIGH);

		Log.e("NavitVehicle", "Providers " + sLocationManager.getAllProviders());

		preciseProvider = sLocationManager.getBestProvider(highCriteria, false);
		Log.e("NavitVehicle", "Precise Provider " + preciseProvider);
		fastProvider = sLocationManager.getBestProvider(lowCriteria, false);
		Log.e("NavitVehicle", "Fast Provider " + fastProvider);
		vehicle_callbackid=callbackid;

		sLocationManager.requestLocationUpdates(preciseProvider, 0, 0, preciseLocationListener);

		// If the 2 providers are the same, only activate one listener
		if (fastProvider == null || preciseProvider.compareTo(fastProvider) == 0) {
			fastProvider = null;
		} else {
			sLocationManager.requestLocationUpdates(fastProvider, 0, 0, fastLocationListener);
		}
	}

	public static void removeListener() {
		if (sLocationManager != null) {
			if (preciseLocationListener != null) sLocationManager.removeUpdates(preciseLocationListener);
			if (fastLocationListener != null) sLocationManager.removeUpdates(fastLocationListener);
		}

	}
}
