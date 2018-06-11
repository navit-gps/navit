/**
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
import android.os.StatFs;
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
    private static SimpleExpandableListAdapter adapter = null;
    private static ArrayList<HashMap<String, String>> downloaded_maps_childs = null;
    private static ArrayList<HashMap<String, String>> maps_current_position_childs = null;
    private static boolean currentLocationKnown = false;
    private final String TAG = this.getClass().getName();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (adapter == null) {
            adapter = createAdapter();
        }
        updateDownloadedMaps();
        updateMapsForLocation(NavitMapDownloader.osm_maps);
        setListAdapter(adapter);
        try {
            setTitle(String.valueOf(getFreeSpace() / 1024 / 1024) + "MB available");
        } catch (Exception e) {
            Log.e(TAG, "Exception " + e.getClass().getName()
                  + " during getFreeSpace, reporting 'no sdcard present'");
            NavitDialogs.sendDialogMessage(NavitDialogs.MSG_TOAST_LONG, null,
                                           String.format(
                                               (Navit.getInstance().getTstring(R.string.map_location_unavailable)),
                                               Navit.map_filename_path),
                                           -1, 0, 0);
            finish();
        }
    }

    private long getFreeSpace() {
        StatFs fsInfo = new StatFs(Navit.map_filename_path);
        return (long) fsInfo.getAvailableBlocks() * fsInfo.getBlockSize();
    }

    private void updateDownloadedMaps() {
        downloaded_maps_childs.clear();
        for (NavitMap map : NavitMapDownloader.getAvailableMaps()) {
            HashMap<String, String> child = new HashMap<String, String>();
            child.put("map_name", map.mapName + " " + (map.size() / 1024 / 1024) + "MB");
            child.put("map_location", map.getLocation());
            downloaded_maps_childs.add(child);
        }
    }

    private void updateMapsForLocation(NavitMapDownloader.osm_map_values[] osm_maps) {
        Location currentLocation = NavitVehicle.lastLocation;
        if (maps_current_position_childs.size() == 0 || (currentLocation != null
                && !currentLocationKnown)) {
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
                currentLocationKnown = true;
            }

            if (currentLocation != null) {
                // if this map contains data to our current position, add it to
                // the MapsOfCurrentLocation-list
                for (int currentMapIndex = 0; currentMapIndex < osm_maps.length;
                        currentMapIndex++) {
                    if (osm_maps[currentMapIndex].isInMap(currentLocation)) {
                        HashMap<String, String> currentPositionMapChild = new HashMap<String, String>();
                        currentPositionMapChild
                        .put("map_name", osm_maps[currentMapIndex].map_name + " "
                             + (osm_maps[currentMapIndex].est_size_bytes / 1024 / 1024)
                             + "MB");
                        currentPositionMapChild.put("map_index", String.valueOf(currentMapIndex));

                        maps_current_position_childs.add(currentPositionMapChild);
                    }
                }
            }
        }
    }

    private SimpleExpandableListAdapter createAdapter() {

        NavitMapDownloader.osm_map_values[] osm_maps = NavitMapDownloader.osm_maps;

        ArrayList<HashMap<String, String>> resultGroups = new ArrayList<HashMap<String, String>>();
        ArrayList<ArrayList<HashMap<String, String>>> resultChilds =
            new ArrayList<ArrayList<HashMap<String, String>>>();

        // add already downloaded maps (group and empty child list
        HashMap<String, String> downloaded_maps_hash = new HashMap<String, String>();
        downloaded_maps_hash
        .put("category_name", Navit.getInstance().getTstring(R.string.maps_installed));
        resultGroups.add(downloaded_maps_hash);
        downloaded_maps_childs = new ArrayList<HashMap<String, String>>();
        resultChilds.add(downloaded_maps_childs);

        ArrayList<HashMap<String, String>> secList = new ArrayList<HashMap<String, String>>();
        maps_current_position_childs = new ArrayList<HashMap<String, String>>();
        // maps containing the current location
        HashMap<String, String> matching_maps = new HashMap<String, String>();
        matching_maps.put("category_name",
                          Navit.getInstance().getTstring(R.string.maps_for_current_location));
        resultGroups.add(matching_maps);
        resultChilds.add(maps_current_position_childs);

        // add all maps
        for (int currentMapIndex = 0; currentMapIndex < osm_maps.length; currentMapIndex++) {
            if (osm_maps[currentMapIndex].level == 0) {
                if (secList.size() > 0) {
                    resultChilds.add(secList);
                }
                secList = new ArrayList<HashMap<String, String>>();
                HashMap<String, String> map_info_hash = new HashMap<String, String>();
                map_info_hash.put("category_name", osm_maps[currentMapIndex].map_name);
                resultGroups.add(map_info_hash);
            }

            HashMap<String, String> child = new HashMap<String, String>();
            child.put("map_name", (osm_maps[currentMapIndex].level > 1 ? MAP_BULLETPOINT : "")
                      + osm_maps[currentMapIndex].map_name + " "
                      + (osm_maps[currentMapIndex].est_size_bytes / 1024 / 1024) + "MB");
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
        HashMap<String, String> child = (HashMap<String, String>) adapter
                                        .getChild(groupPosition, childPosition);

        String map_index = child.get("map_index");
        if (map_index != null) {
            int mi = Integer.parseInt(map_index);
            if (NavitMapDownloader.osm_maps[mi].est_size_bytes / 1024 / 1024 / 950 >= 4) {
                NavitDialogs.sendDialogMessage(NavitDialogs.MSG_TOAST_LONG, null,
                                               Navit.getInstance().getTstring(R.string.map_download_oversize),
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

    private void askForMapDeletion(final String map_location) {
        AlertDialog.Builder deleteMapBox = new AlertDialog.Builder(this);
        deleteMapBox.setTitle(Navit.getInstance().getTstring(R.string.map_delete));
        deleteMapBox.setCancelable(true);

        NavitMap maptoDelete = new NavitMap(map_location);
        deleteMapBox.setMessage(
            maptoDelete.mapName + " " + String.valueOf(maptoDelete.size() / 1024 / 1024)
            + "MB");

        // TRANS
        deleteMapBox.setPositiveButton(Navit.getInstance().getTstring(R.string.yes),
        new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface arg0, int arg1) {
                Log.d(TAG, "Delete Map");
                Message msg =
                    Message.obtain(
                        Navit.getInstance().getNavitGraphics().callback_handler,
                        NavitGraphics.msg_type.CLB_DELETE_MAP.ordinal());
                Bundle b = new Bundle();
                b.putString("title", map_location);
                msg.setData(b);
                msg.sendToTarget();
                finish();
            }
        });

        // TRANS
        deleteMapBox.setNegativeButton((Navit.getInstance().getTstring(R.string.no)),
        new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface arg0, int arg1) {
                Log.d(TAG, "don't delete map");
            }
        });
        deleteMapBox.show();
    }
}
