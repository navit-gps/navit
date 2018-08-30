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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.location.Criteria;
import android.location.GpsSatellite;
import android.location.GpsStatus;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;
import android.support.v4.content.ContextCompat;
import android.util.Log;

import java.util.List;



public class NavitVehicle {

    private static final String GPS_FIX_CHANGE = "android.location.GPS_FIX_CHANGE";

    public static Location lastLocation = null;

    private static LocationManager sLocationManager = null;
    private static Context context = null;
    private int vehicle_pcbid;
    private int vehicle_scbid;
    private int vehicle_fcbid;
    private String preciseProvider;
    private String fastProvider;

    private static NavitLocationListener preciseLocationListener = null;
    private static NavitLocationListener fastLocationListener = null;

    public native void VehicleCallback(int id, Location location);
    public native void VehicleCallback(int id, int satsInView, int satsUsed);
    public native void VehicleCallback(int id, int enabled);

    private class NavitLocationListener extends BroadcastReceiver implements GpsStatus.Listener, LocationListener {
        public boolean precise = false;
        public void onLocationChanged(Location location) {
            lastLocation = location;
            // Disable the fast provider if still active
            if (precise && fastProvider != null) {
                sLocationManager.removeUpdates(fastLocationListener);
                fastProvider = null;
            }

            VehicleCallback(vehicle_pcbid, location);
            VehicleCallback(vehicle_fcbid, 1);
        }
        public void onProviderDisabled(String provider) {}
        public void onProviderEnabled(String provider) {}
        public void onStatusChanged(String provider, int status, Bundle extras) {}

        /**
         * Called when the status of the GPS changes.
         */
        public void onGpsStatusChanged (int event) {
            if (ContextCompat.checkSelfPermission(context, android.Manifest.permission.ACCESS_FINE_LOCATION)
                    != PackageManager.PERMISSION_GRANTED) {
                // Permission is not granted
                return;
            }
            GpsStatus status = sLocationManager.getGpsStatus(null);
            int satsInView = 0;
            int satsUsed = 0;
            Iterable<GpsSatellite> sats = status.getSatellites();
            for (GpsSatellite sat : sats) {
                satsInView++;
                if (sat.usedInFix()) {
                    satsUsed++;
                }
            }
            VehicleCallback(vehicle_scbid, satsInView, satsUsed);
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(GPS_FIX_CHANGE)) {
                if (intent.getBooleanExtra("enabled", false))
                    VehicleCallback(vehicle_fcbid, 1);
                else if (!intent.getBooleanExtra("enabled", true))
                    VehicleCallback(vehicle_fcbid, 0);
            }
        }
    }

    /**
     * @brief Creates a new {@code NavitVehicle}
     *
     * @param context
     * @param pcbid The address of the position callback function which will be called when a location update is received
     * @param scbid The address of the status callback function which will be called when a status update is received
     * @param fcbid The address of the fix callback function which will be called when a
     * {@code android.location.GPS_FIX_CHANGE} is received, indicating a change in GPS fix status
     */
    NavitVehicle (Context context, int pcbid, int scbid, int fcbid) {
        if (ContextCompat.checkSelfPermission(context, android.Manifest.permission.ACCESS_FINE_LOCATION)
                != PackageManager.PERMISSION_GRANTED) {
            // Permission is not granted
            return;
        }
        this.context = context;
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
        vehicle_pcbid = pcbid;
        vehicle_scbid = scbid;
        vehicle_fcbid = fcbid;

        context.registerReceiver(preciseLocationListener, new IntentFilter(GPS_FIX_CHANGE));
        sLocationManager.requestLocationUpdates(preciseProvider, 0, 0, preciseLocationListener);
        sLocationManager.addGpsStatusListener(preciseLocationListener);

        /*
         * Since Android criteria have no way to specify "fast fix", lowCriteria may return the same
         * provider as highCriteria, even if others are available. In this case, do not register two
         * listeners for the same provider but pick the fast provider manually. (Usually there will
         * only be two providers in total, which makes the choice easy.)
         */
        if (fastProvider == null || preciseProvider.compareTo(fastProvider) == 0) {
            List<String> fastProviderList = sLocationManager.getProviders(lowCriteria, false);
            fastProvider = null;
            for (String fastCandidate: fastProviderList) {
                if (preciseProvider.compareTo(fastCandidate) != 0) {
                    fastProvider = fastCandidate;
                    break;
                }
            }
        }
        if (fastProvider != null) {
            sLocationManager.requestLocationUpdates(fastProvider, 0, 0, fastLocationListener);
        }
    }

    public static void removeListener() {
        if (sLocationManager != null) {
            if (preciseLocationListener != null) {
                sLocationManager.removeUpdates(preciseLocationListener);
                sLocationManager.removeGpsStatusListener(preciseLocationListener);
                context.unregisterReceiver(preciseLocationListener);
            }
            if (fastLocationListener != null) sLocationManager.removeUpdates(fastLocationListener);
        }

    }
}
