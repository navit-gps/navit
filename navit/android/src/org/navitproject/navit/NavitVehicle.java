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
