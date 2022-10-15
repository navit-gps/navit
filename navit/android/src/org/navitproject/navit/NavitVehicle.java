/*
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

import java.util.HashSet;
import java.util.List;



public class NavitVehicle {

    private static final String GPS_FIX_CHANGE = "android.location.GPS_FIX_CHANGE";
    static Location sLastLocation;
    private static LocationManager sLocationManager;
    private Context mContext;
    private long mVehiclePcbid;
    private long mVehicleScbid;
    private long mVehicleFcbid;
    private String mFastProvider;

    private static NavitLocationListener sPreciseLocationListener;
    private static NavitLocationListener sFastLocationListener;

    public native void vehicleCallback(long id, Location location);

    public native void vehicleCallback(long id, int satsInView, int satsUsed);

    public native void vehicleCallback(long id, int enabled);

    private class NavitLocationListener extends BroadcastReceiver implements GpsStatus.Listener, LocationListener {
        boolean mPrecise = false;

        public void onLocationChanged(Location location) {
            Log.e("NavitVehicle", String.format("Location update from %s, precise=%b", location.getProvider(), mPrecise));
            // Disable the fast provider if still active
            if (mPrecise && mFastProvider != null) {
                sLocationManager.removeUpdates(sFastLocationListener);
                mFastProvider = null;
            }
            sLastLocation = location;
            vehicleCallback(mVehiclePcbid, location);
            vehicleCallback(mVehicleFcbid, 1);
        }

        public void onProviderDisabled(String provider) {
            //unhandled
        }

        public void onProviderEnabled(String provider) {
            //unhandled
        }

        public void onStatusChanged(String provider, int status, Bundle extras) {
            //unhandled
        }

        /**
         * Called when the status of the GPS changes.
         */
        public void onGpsStatusChanged(int event) {
            if (ContextCompat.checkSelfPermission(mContext, android.Manifest.permission.ACCESS_FINE_LOCATION)
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
            vehicleCallback(mVehicleScbid, satsInView, satsUsed);
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(GPS_FIX_CHANGE)) {
                if (intent.getBooleanExtra("enabled", false)) {
                    vehicleCallback(mVehicleFcbid, 1);
                } else {
                    if (!intent.getBooleanExtra("enabled", true)) {
                        vehicleCallback(mVehicleFcbid, 0);
                    }
                }
            }
        }
    }

    /**
     * Creates a new {@code NavitVehicle}.
     *
     * @param context the context
     * @param pcbid The address of the position callback function called when a location update is received
     * @param scbid The address of the status callback function called when a status update is received
     * @param fcbid The address of the fix callback function called when a
     * {@code android.location.GPS_FIX_CHANGE} is received, indicating a change in GPS fix status
     */
    NavitVehicle(Context context, long pcbid, long scbid, long fcbid) {
        if (ContextCompat.checkSelfPermission(context, android.Manifest.permission.ACCESS_FINE_LOCATION)
                != PackageManager.PERMISSION_GRANTED) {
            // Permission is not granted
            return;
        }
        this.mContext = context;
        sLocationManager = (LocationManager)context.getSystemService(Context.LOCATION_SERVICE);
        sPreciseLocationListener = new NavitLocationListener();
        sPreciseLocationListener.mPrecise = true;
        sFastLocationListener = new NavitLocationListener();

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

        Log.d("NavitVehicle", "Providers " + sLocationManager.getAllProviders());

        HashSet<String> preciseProviders;
        try {
            preciseProviders = new HashSet<String>(sLocationManager.getProviders(highCriteria, false));
        } catch (NullPointerException e) {
            preciseProviders = new HashSet<String>();
        }
        HashSet<String> fastProviders;
        try {
            fastProviders = new HashSet<String>(sLocationManager.getProviders(lowCriteria, false));
        } catch (NullPointerException e) {
            fastProviders = new HashSet<String>();
        }
        /*
         * Never use the passive and fused providers for the precise providers.
         * These merge data from multiple sources, which can lead to your location skipping back and
         * forth between two places. While that may be acceptable for the fast provider (just to get a
         * first location), it may render navigation unusable if such a provider were to be used as the
         * precise provider.
         */
        preciseProviders.remove(LocationManager.PASSIVE_PROVIDER);
        preciseProviders.remove("fused");

        /*
         * Some magic to pick the precise provider:
         * If Android’s best chosen provider is on our list, use that.
         * Otherwise, if the list has only one candidate, use that.
         * Otherwise, prefer GPS for the precise provider, if available.
         * Otherwise, just pick a random provider from the list.
         */
        Log.d("NavitVehicle", "Precise Provider candidates " + preciseProviders);
        String mPreciseProvider = null;
        if (preciseProviders.contains(sLocationManager.getBestProvider(highCriteria, false)))
            mPreciseProvider = sLocationManager.getBestProvider(highCriteria, false);
        else if (preciseProviders.size() == 1)
            for (String provider : preciseProviders)
                mPreciseProvider = provider;
        else if (preciseProviders.contains(LocationManager.GPS_PROVIDER))
            mPreciseProvider = LocationManager.GPS_PROVIDER;
        else
            /*
             * Multiple providers, but no GPS, nor any fused/passive providers, and we can’t use the
             * best provider suggested by the OS.
             * If we ever get here, we’re on a very exotic device.
             * This may need some more tweaks – right now, we just pick one at random.
             */
            for (String provider : preciseProviders) {
                mPreciseProvider = provider;
                break;
            }
        Log.d("NavitVehicle", "Precise Provider " + mPreciseProvider);

        /*
         * Similar magic to pick the fast provider:
         * Eliminate the precise provider from our list so we get to use two providers.
         * If Android’s best chosen provider is on our list, use that.
         * Otherwise, if the list has only one candidate, use that.
         * Otherwise, just pick a random provider from the list.
         */
        fastProviders.remove(mPreciseProvider);
        Log.d("NavitVehicle", "Fast Provider candidates " + fastProviders);
        if (fastProviders.contains(sLocationManager.getBestProvider(lowCriteria, false)))
            mFastProvider = sLocationManager.getBestProvider(lowCriteria, false);
        else if (fastProviders.size() == 1)
            for (String provider: fastProviders)
                mFastProvider = provider;
        else
            for (String provider: fastProviders) {
                mFastProvider = provider;
                break;
            }
        Log.d("NavitVehicle", "Fast Provider " + mFastProvider);

        mVehiclePcbid = pcbid;
        mVehicleScbid = scbid;
        mVehicleFcbid = fcbid;

        context.registerReceiver(sPreciseLocationListener, new IntentFilter(GPS_FIX_CHANGE));
        sLocationManager.requestLocationUpdates(mPreciseProvider, 0, 0, sPreciseLocationListener);
        sLocationManager.addGpsStatusListener(sPreciseLocationListener);

        /*
         * Since Android criteria have no way to specify "fast fix", lowCriteria may return the same
         * provider as highCriteria, even if others are available. In this case, do not register two
         * listeners for the same provider but pick the fast provider manually. (Usually there will
         * only be two providers in total, which makes the choice easy.)
         */
        if (mFastProvider == null || mPreciseProvider.compareTo(mFastProvider) == 0) {
            List<String> fastProviderList = sLocationManager.getProviders(lowCriteria, false);
            mFastProvider = null;
            for (String fastCandidate: fastProviderList) {
                if (mPreciseProvider.compareTo(fastCandidate) != 0) {
                    mFastProvider = fastCandidate;
                    break;
                }
            }
        }
        if (mFastProvider != null) {
            sLocationManager.requestLocationUpdates(mFastProvider, 0, 0, sFastLocationListener);
        }
    }

    static void removeListeners(Navit navit) {
        if (sLocationManager != null) {
            if (sPreciseLocationListener != null) {
                sLocationManager.removeUpdates(sPreciseLocationListener);
                sLocationManager.removeGpsStatusListener(sPreciseLocationListener);
                navit.unregisterReceiver(sPreciseLocationListener);
            }
            if (sFastLocationListener != null) {
                sLocationManager.removeUpdates(sFastLocationListener);
            }
        }
    }
}
