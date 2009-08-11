/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
	public native void VehicleCallback(int id, Location location);

	NavitVehicle (Context context, int callbackid) {
		locationManager = (LocationManager)context.getSystemService(Context.LOCATION_SERVICE);

		Criteria criteria = new Criteria();
		criteria.setAccuracy(Criteria.ACCURACY_FINE);
		criteria.setAltitudeRequired(true);
		criteria.setBearingRequired(true);
		criteria.setCostAllowed(true);
		criteria.setPowerRequirement(Criteria.POWER_HIGH);

		
		Log.e("NavitVehicle", "Providers "+locationManager.getAllProviders());
		String provider = locationManager.getBestProvider(criteria, false);
		Log.e("NavitVehicle", "Provider "+provider);
		Location location = locationManager.getLastKnownLocation(provider);
		vehicle_callbackid=callbackid;
		locationManager.requestLocationUpdates(provider, 0, 0, locationListener);
	}
	private final LocationListener locationListener = new LocationListener() {
		public void onLocationChanged(Location location) {
			
			// Log.e("NavitVehicle", "LocationChanged Accuracy " + location.getAccuracy() + " Altitude " + location.getAltitude() + " Bearing " + location.getBearing() + " Speed " + location.getSpeed() + " Latitude " + location.getLatitude() + " Longitude " + location.getLongitude());
			VehicleCallback(vehicle_callbackid, location);	
		}
	  	public void onProviderDisabled(String provider){}
	  	public void onProviderEnabled(String provider) {}
	  	public void onStatusChanged(String provider, int status, Bundle extras) {}
	};
}
