/*
 * Navit, a modular navigation system. Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if
 * not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

package org.navitproject.navit;

import static org.navitproject.navit.NavitAppConfig.getTstring;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ExpandableListActivity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.location.Location;
import android.location.LocationManager;
import android.os.Bundle;
import android.os.Message;
import android.support.v4.app.ActivityCompat;
import android.util.Log;
import android.view.View;
import android.widget.ExpandableListView;
import android.widget.SimpleExpandableListAdapter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;


public class NavitDownloadSelectMapActivity extends ExpandableListActivity {

    private static final String MAP_BULLETPOINT = " * ";
    private static SimpleExpandableListAdapter sAdapter = null;
    private static ArrayList<HashMap<String, String>> sDownloadedMapsChilds = null;
    private static ArrayList<HashMap<String, String>> sMapsCurrentPositionChilds = null;
    private static boolean sCurrentLocationKnown = false;
    private static final String TAG = "DownloadSelectMapAct";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (sAdapter == null) {
            sAdapter = createAdapter();
        }
        updateDownloadedMaps();
        updateMapsForLocation();
        setListAdapter(sAdapter);
        try {
            setTitle(NavitUtils.getFreeSpace(Navit.sMapFilenamePath) / 1024 / 1024 + "MB available");
        } catch (Exception e) {
            Log.e(TAG, "Exception " + e.getClass().getName()
                    + " during getFreeSpace, reporting 'no sdcard present'");
            NavitDialogs.sendDialogMessage(NavitDialogs.MSG_TOAST_LONG, null,
                    String.format(
                        (getTstring(R.string.map_location_unavailable)),
                        Navit.sMapFilenamePath),
                    -1, 0, 0);
            finish();
        }
    }


    private void updateDownloadedMaps() {
        sDownloadedMapsChilds.clear();
        for (NavitMap map : NavitMapDownloader.getAvailableMaps()) {
            HashMap<String, String> child = new HashMap<>();
            child.put("map_name", map.mMapName + " " + (map.size() / 1024 / 1024) + "MB");
            child.put("map_location", map.getLocation());
            sDownloadedMapsChilds.add(child);
        }
    }

    private void updateMapsForLocation() {
        Location currentLocation = NavitVehicle.sLastLocation;
        if (sMapsCurrentPositionChilds.size() == 0 || (currentLocation != null
                    && !sCurrentLocationKnown)) {
            if (currentLocation == null) {
                LocationManager mapLocationManager = (LocationManager) getSystemService(
                        Context.LOCATION_SERVICE);
                List<String> providers = mapLocationManager.getProviders(true);
                long lastUpdate;
                long bestUpdateTime = -1;
                for (String provider : providers) {
                    if (ActivityCompat
                            .checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION)
                            != PackageManager.PERMISSION_GRANTED
                            && ActivityCompat
                            .checkSelfPermission(this, Manifest.permission.ACCESS_COARSE_LOCATION)
                            != PackageManager.PERMISSION_GRANTED) {
                        return;
                    }
                    Location lastKnownLocation = mapLocationManager.getLastKnownLocation(provider);
                    if (lastKnownLocation != null) {
                        lastUpdate = lastKnownLocation.getTime();
                        if (lastUpdate > bestUpdateTime) {
                            currentLocation = lastKnownLocation;
                            bestUpdateTime = lastUpdate;
                        }
                    }
                }
            } else {
                sCurrentLocationKnown = true;
            }

            if (currentLocation != null) {
                // if this map contains data to our current position, add it to
                // the MapsOfCurrentLocation-list
                for (int currentMapIndex = 0; currentMapIndex < NavitMapDownloader.osm_maps.length;
                        currentMapIndex++) {
                    if (NavitMapDownloader.osm_maps[currentMapIndex].isInMap(currentLocation)) {
                        HashMap<String, String> currentPositionMapChild = new HashMap<>();
                        currentPositionMapChild.put("map_name", NavitMapDownloader.osm_maps[currentMapIndex].mMapName
                                    + " "
                                    + (NavitMapDownloader.osm_maps[currentMapIndex].mEstSizeBytes / 1024 / 1024)
                                    + "MB");
                        currentPositionMapChild.put("map_index", String.valueOf(currentMapIndex));

                        sMapsCurrentPositionChilds.add(currentPositionMapChild);
                    }
                }
            }
        }
    }

    private SimpleExpandableListAdapter createAdapter() {

        ArrayList<HashMap<String, String>> resultGroups = new ArrayList<>();

        // add already downloaded maps (group and empty child list
        HashMap<String, String> downloadedMapsHash = new HashMap<>();
        downloadedMapsHash.put("category_name", getTstring(R.string.maps_installed));
        resultGroups.add(downloadedMapsHash);
        sDownloadedMapsChilds = new ArrayList<>();
        ArrayList<ArrayList<HashMap<String, String>>> resultChilds = new ArrayList<>();
        resultChilds.add(sDownloadedMapsChilds);

        ArrayList<HashMap<String, String>> secList = new ArrayList<>();
        sMapsCurrentPositionChilds = new ArrayList<>();
        // maps containing the current location
        HashMap<String, String> matchingMaps = new HashMap<>();
        matchingMaps.put("category_name",
                getTstring(R.string.maps_for_current_location));
        resultGroups.add(matchingMaps);
        resultChilds.add(sMapsCurrentPositionChilds);
        NavitMapDownloader.OsmMapValues[] osmMaps = NavitMapDownloader.osm_maps;
        // add all maps
        for (int currentMapIndex = 0; currentMapIndex < osmMaps.length; currentMapIndex++) {
            if (osmMaps[currentMapIndex].mLevel == 0) {
                if (secList.size() > 0) {
                    resultChilds.add(secList);
                }
                secList = new ArrayList<>();
                HashMap<String, String> mapInfoHash = new HashMap<>();
                mapInfoHash.put("category_name", osmMaps[currentMapIndex].mMapName);
                resultGroups.add(mapInfoHash);
            }

            HashMap<String, String> child = new HashMap<>();
            child.put("map_name", (osmMaps[currentMapIndex].mLevel > 1 ? MAP_BULLETPOINT : "")
                    + osmMaps[currentMapIndex].mMapName + " "
                    + (osmMaps[currentMapIndex].mEstSizeBytes / 1024 / 1024) + "MB");
            child.put("map_index", String.valueOf(currentMapIndex));

            secList.add(child);
        }
        resultChilds.add(secList);

        return new SimpleExpandableListAdapter(this, resultGroups,
                android.R.layout.simple_expandable_list_item_1,
                new String[] {"category_name"}, new int[] {android.R.id.text1}, resultChilds,
                android.R.layout.simple_expandable_list_item_1, new String[] {"map_name"},
                new int[] {android.R.id.text1});
    }

    @Override
    public boolean onChildClick(ExpandableListView parent, View v, int groupPosition,
            int childPosition, long id) {
        super.onChildClick(parent, v, groupPosition, childPosition, id);
        Log.d(TAG, "p:" + groupPosition + ", child_pos:" + childPosition);

        @SuppressWarnings("unchecked")
        HashMap<String, String> child = (HashMap<String, String>) sAdapter.getChild(groupPosition, childPosition);

        String mapIndex = child.get("map_index");
        if (mapIndex != null) {
            int mi = Integer.parseInt(mapIndex);
            if (NavitMapDownloader.osm_maps[mi].mEstSizeBytes / 1024 / 1024 / 950 >= 4) {
                NavitDialogs.sendDialogMessage(NavitDialogs.MSG_TOAST_LONG, null,
                        getTstring(R.string.map_download_oversize),
                        -1, 0, 0);
                return true;
            }
            Intent resultIntent = new Intent();
            resultIntent.putExtra("map_index", mi);
            setResult(Activity.RESULT_OK, resultIntent);
            finish();
        } else {
            // ask user if to delete this map
            askForMapDeletion(child.get("map_location"));
        }
        return true;
    }

    private void askForMapDeletion(final String mapLocation) {
        AlertDialog.Builder deleteMapBox = new AlertDialog.Builder(this);
        deleteMapBox.setTitle(getTstring(R.string.map_delete));
        deleteMapBox.setCancelable(true);

        NavitMap maptoDelete = new NavitMap(mapLocation);
        deleteMapBox.setMessage(
                maptoDelete.mMapName + " " + maptoDelete.size() / 1024 / 1024
                + "MB");

        // TRANS
        deleteMapBox.setPositiveButton(getTstring(R.string.yes),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface arg0, int arg1) {
                        Log.d(TAG, "Delete Map");
                        Message msg = Message.obtain(NavitCallbackHandler.sCallbackHandler,
                                    NavitCallbackHandler.MsgType.CLB_DELETE_MAP.ordinal());
                        Bundle b = new Bundle();
                        b.putString("title", mapLocation);
                        msg.setData(b);
                        msg.sendToTarget();
                        finish();
                    }
                });

        // TRANS
        deleteMapBox.setNegativeButton((getTstring(R.string.no)),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface arg0, int arg1) {
                        Log.d(TAG, "don't delete map");
                    }
                });
        deleteMapBox.show();
    }
}
