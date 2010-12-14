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

import android.app.Activity;
import android.content.Context;
import android.graphics.*;
import android.os.Bundle;
import android.os.Debug;
import android.view.*;
import android.util.Log;
import android.location.*;

public class NavitVehicle {
	private LocationManager locationManager;
	private int vehicle_callbackid;
	private String preciseProvider;
	private String fastProvider;
	public native void VehicleCallback(int id, Location location);

	NavitVehicle (Context context, int callbackid) {
		locationManager = (LocationManager)context.getSystemService(Context.LOCATION_SERVICE);

		/* Use 2 LocationProviders, one precise (usually GPS), and one
		   not so precise, but possible faster. The fast provider is 
		   disabled when the precise provider gets its first fix. */
		
		// Selection criterias for the precise provider
		Criteria highCriteria = new Criteria();
		highCriteria.setAccuracy(Criteria.ACCURACY_FINE);
		highCriteria.setAltitudeRequired(true);
		highCriteria.setBearingRequired(true);
		highCriteria.setCostAllowed(true);
		highCriteria.setPowerRequirement(Criteria.POWER_HIGH);

		// Selection criterias for the fast provider
		Criteria lowCriteria = new Criteria();
		lowCriteria.setAccuracy(Criteria.ACCURACY_COARSE);
		lowCriteria.setAltitudeRequired(false);
		lowCriteria.setBearingRequired(false);
		lowCriteria.setCostAllowed(true);
		lowCriteria.setPowerRequirement(Criteria.POWER_HIGH);
		
		Log.e("NavitVehicle", "Providers "+locationManager.getAllProviders());
		
		preciseProvider = locationManager.getBestProvider(highCriteria, false);
		Log.e("NavitVehicle", "Precise Provider " + preciseProvider);
		fastProvider = locationManager.getBestProvider(lowCriteria, false);
		Log.e("NavitVehicle", "Fast Provider " + fastProvider);
		vehicle_callbackid=callbackid;
		
		locationManager.requestLocationUpdates(preciseProvider, 0, 0, preciseLocationListener);
		
		// If the 2 providers is the same, only activate one listener
		if (fastProvider == null || preciseProvider.compareTo(fastProvider) == 0) {
			fastProvider = null;
		} else {
			locationManager.requestLocationUpdates(fastProvider, 0, 0, fastLocationListener);
		}
	}
	private final LocationListener preciseLocationListener = new LocationListener() {
		public void onLocationChanged(Location location) {
			
			// Disable the fast provider if still active
			if (fastProvider != null) {
				locationManager.removeUpdates(fastLocationListener);
				fastProvider = null;
			}
			
			// Log.e("NavitVehicle", "LocationChanged Accuracy " + location.getAccuracy() + " Altitude " + location.getAltitude() + " Bearing " + location.getBearing() + " Speed " + location.getSpeed() + " Latitude " + location.getLatitude() + " Longitude " + location.getLongitude());
			VehicleCallback(vehicle_callbackid, location);
		}
	  	public void onProviderDisabled(String provider){}
	  	public void onProviderEnabled(String provider) {}
	  	public void onStatusChanged(String provider, int status, Bundle extras) {}
	};

	private final LocationListener fastLocationListener = new LocationListener() {
		public void onLocationChanged(Location location) {
			
			// Log.e("NavitVehicle", "LocationChanged Accuracy " + location.getAccuracy() + " Altitude " + location.getAltitude() + " Bearing " + location.getBearing() + " Speed " + location.getSpeed() + " Latitude " + location.getLatitude() + " Longitude " + location.getLongitude());
			VehicleCallback(vehicle_callbackid, location);
		}
	  	public void onProviderDisabled(String provider){}
	  	public void onProviderEnabled(String provider) {}
	  	public void onStatusChanged(String provider, int status, Bundle extras) {}
	};

}
