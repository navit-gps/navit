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

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ExpandableListActivity;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.Gravity;
import android.widget.ExpandableListView;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.ArrayDeque;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Set;

public class NavitDownloadSelectMapActivity extends ExpandableListActivity {

    private static final String TAG = "DownloadSelectMapAct";
    private static final String RELEASE_URL =
            "https://api.github.com/repos/navit-gps/gh-actions-mapserver/releases/latest";
    private static ArrayList<HashMap<String, String>> sDownloadedMapsChilds =
            new ArrayList<HashMap<String, String>>();
    private static GithubMetadata sMetadata;
    private GithubMetadataListAdapter adapter;
    private ExpandableListView mapList;
    private FrameLayout content;
    private LinearLayout mapPanel;
    private LinearLayout variantsPanel;
    private LinearLayout variantsRows;
    private TextView variantsTitle;
    private TextView mapListTitle;
    private TextView metadataStatus;
    private ArrayList<GithubMetadata.Artifact> selectedVariants;
    private final ArrayDeque<Integer> downloadQueue = new ArrayDeque<>();
    private final Set<Integer> queuedDownloads = new HashSet<>();
    private NavitMapDownloader currentDownloader;
    private int currentAssetIndex = -1;
    private String lastDownloadedMap;

    static GithubMetadata.Artifact[] getReleaseAssets() {
        return sMetadata == null ? null : sMetadata.getAssets();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        adapter = new GithubMetadataListAdapter(this);
        setListAdapter(adapter);
        mapList = getExpandableListView();
        ViewGroup listParent = (ViewGroup) mapList.getParent();
        listParent.removeView(mapList);
        content = new FrameLayout(this);
        mapPanel = new LinearLayout(this);
        mapPanel.setOrientation(LinearLayout.VERTICAL);
        mapListTitle = new TextView(this);
        mapListTitle.setTextSize(20);
        mapListTitle.setPadding(12, 0, 12, 0);
        mapPanel.addView(createHeader(mapListTitle, view -> finish(), false));
        metadataStatus = new TextView(this);
        metadataStatus.setText(getTstring(R.string.map_metadata_loading));
        metadataStatus.setPadding(16, 8, 16, 8);
        mapPanel.addView(metadataStatus);
        mapPanel.addView(mapList, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        content.addView(mapPanel, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        variantsPanel = createVariantsPanel();
        variantsPanel.setVisibility(View.GONE);
        content.addView(variantsPanel, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        setContentView(content);
        updateDownloadedMaps();
        updateGithubMetaData(adapter);

        try {
            String availableSpace = NavitUtils.getFreeSpace(Navit.sMapFilenamePath)
                    / 1024 / 1024 + "MB available";
            mapListTitle.setText(availableSpace);
        } catch (Exception e) {
            Log.e(TAG, "Exception " + e.getClass().getName()
                    + " during getFreeSpace, reporting 'no sdcard present'");
            NavitDialogs.sendDialogMessage(NavitDialogs.MSG_TOAST_LONG, null,
                    String.format(getTstring(R.string.map_location_unavailable),
                            Navit.sMapFilenamePath),
                    -1, 0, 0);
            finish();
        }
    }

    private void updateGithubMetaData(final GithubMetadataListAdapter adapter) {
        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                HttpURLConnection connection = null;
                try {
                    connection = (HttpURLConnection) new URL(RELEASE_URL).openConnection();
                    connection.setConnectTimeout(15000);
                    connection.setReadTimeout(30000);
                    int responseCode = connection.getResponseCode();
                    if (responseCode < 200 || responseCode >= 300) {
                        throw new IOException("GitHub API returned HTTP " + responseCode);
                    }

                    StringBuilder response = new StringBuilder();
                    try (BufferedReader reader = new BufferedReader(
                            new InputStreamReader(connection.getInputStream(), "UTF-8"))) {
                        String line;
                        while ((line = reader.readLine()) != null) {
                            response.append(line);
                        }
                    }

                    final GithubMetadata metadata = new GithubMetadata(response.toString());
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            if (!isFinishing()) {
                                sMetadata = metadata;
                                adapter.setMetadata(metadata);
                                metadataStatus.setVisibility(View.GONE);
                            }
                        }
                    });
                } catch (IOException | RuntimeException e) {
                    Log.e(TAG, "Failed to load GitHub release metadata", e);
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            if (!isFinishing()) {
                                metadataStatus.setText(getTstring(R.string.map_metadata_failed));
                                metadataStatus.setVisibility(View.VISIBLE);
                            }
                        }
                    });
                } finally {
                    if (connection != null) {
                        connection.disconnect();
                    }
                }
            }
        });
        thread.setName("github-map-metadata");
        thread.start();
    }

    private void updateDownloadedMaps() {
        sDownloadedMapsChilds.clear();
        for (NavitMap map : NavitMapDownloader.getAvailableMaps()) {
            HashMap<String, String> child = new HashMap<>();
            child.put("map_name", map.mMapName + " " + map.size() / 1024 / 1024 + "MB");
            child.put("map_location", map.getLocation());
            sDownloadedMapsChilds.add(child);
        }
    }

    private void downloadAsset(GithubMetadata.Artifact asset) {
        int assetIndex = adapter.getAssetIndex(asset);
        if (assetIndex < 0) {
            Log.e(TAG, "Selected asset is not present in the release metadata");
            return;
        }
        if ("Downloaded".equals(adapter.getDownloadState(asset))) {
            return;
        }
        if (queuedDownloads.add(assetIndex)) {
            downloadQueue.add(assetIndex);
            adapter.setDownloadState(asset, "Queued");
            refreshVariantRows();
            startNextDownload();
        }
    }

    private void cancelDownload(GithubMetadata.Artifact asset) {
        int assetIndex = adapter.getAssetIndex(asset);
        if (assetIndex < 0) {
            return;
        }
        if (assetIndex == currentAssetIndex && currentDownloader != null) {
            adapter.setDownloadState(asset, getTstring(R.string.map_download_cancelling));
            currentDownloader.stop_thread();
        } else if (downloadQueue.remove(assetIndex)) {
            queuedDownloads.remove(assetIndex);
            adapter.setDownloadState(asset, "Cancelled");
        }
        refreshVariantRows();
    }

    private LinearLayout createVariantsPanel() {
        LinearLayout panel = new LinearLayout(this);
        panel.setOrientation(LinearLayout.VERTICAL);

        variantsTitle = new TextView(this);
        variantsTitle.setTextSize(20);
        variantsTitle.setPadding(12, 0, 12, 0);
        panel.addView(createHeader(variantsTitle, view -> showMapList(), true));

        ScrollView scrollView = new ScrollView(this);
        variantsRows = new LinearLayout(this);
        variantsRows.setOrientation(LinearLayout.VERTICAL);
        scrollView.addView(variantsRows);
        panel.addView(scrollView, new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, 0, 1));
        return panel;
    }

    private LinearLayout createHeader(TextView title, View.OnClickListener backAction,
            boolean includeDownloadAll) {
        LinearLayout header = new LinearLayout(this);
        header.setGravity(Gravity.CENTER_VERTICAL);
        Button backButton = new Button(this);
        backButton.setText("Back");
        backButton.setOnClickListener(backAction);
        header.addView(backButton);

        header.addView(title, new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        if (includeDownloadAll) {
            Button downloadAllButton = new Button(this);
            downloadAllButton.setText("Download all");
            downloadAllButton.setOnClickListener(view -> {
                if (selectedVariants != null) {
                    for (GithubMetadata.Artifact asset : selectedVariants) {
                        downloadAsset(asset);
                    }
                }
            });
            header.addView(downloadAllButton);
        }
        return header;
    }

    private void showVariants(String countryName, ArrayList<GithubMetadata.Artifact> assets) {
        selectedVariants = assets;
        variantsTitle.setText(countryName);
        refreshVariantRows();
        mapPanel.setVisibility(View.GONE);
        variantsPanel.setVisibility(View.VISIBLE);
    }

    private void showMapList() {
        variantsPanel.setVisibility(View.GONE);
        mapPanel.setVisibility(View.VISIBLE);
    }

    private void refreshVariantRows() {
        if (variantsRows == null || selectedVariants == null) {
            return;
        }
        variantsRows.removeAllViews();
        for (GithubMetadata.Artifact asset : selectedVariants) {
            final int assetIndex = adapter.getAssetIndex(asset);
            String state = adapter.getDownloadState(asset);
            String details = String.format(java.util.Locale.getDefault(), "%.1f MB",
                    asset.getFileSize() / (1024.0 * 1024.0)) + " · "
                    + (state == null ? "Not downloaded" : state);
            boolean canCancel = "Queued".equals(state)
                    || (state != null && (state.startsWith("Downloading")
                    || state.equals(getTstring(R.string.map_download_cancelling))));
            boolean canDownload = assetIndex >= 0
                    && (state == null || "Failed".equals(state) || "Cancelled".equals(state));
            LinearLayout row = MapDownloadRow.create(this,
                    formatMapVariantName(asset.getFilename()), details,
                    canCancel ? R.drawable.map_cancel : "Downloaded".equals(state)
                            ? R.drawable.map_downloaded : R.drawable.map_download,
                    canCancel ? getTstring(R.string.map_download_cancel)
                            : getTstring(R.string.map_download_title),
                    canCancel ? view -> cancelDownload(asset)
                            : canDownload ? view -> downloadAsset(asset) : null);

            if (state != null && state.startsWith("Downloading")) {
                ProgressBar progress = new ProgressBar(this, null,
                        android.R.attr.progressBarStyleHorizontal);
                progress.setMax(100);
                progress.setProgress(MapDownloadRow.parseProgress(state));
                row.addView(progress);
            }
            variantsRows.addView(row);
        }
    }

    private void startNextDownload() {
        if (currentDownloader != null || downloadQueue.isEmpty()) {
            if (currentDownloader == null && downloadQueue.isEmpty()
                    && lastDownloadedMap != null) {
                Message loadMap = Message.obtain(NavitCallbackHandler.sCallbackHandler,
                        NavitCallbackHandler.MsgType.CLB_LOAD_MAP.ordinal());
                Bundle data = new Bundle();
                data.putString("title", lastDownloadedMap);
                loadMap.setData(data);
                loadMap.sendToTarget();
                lastDownloadedMap = null;
            }
            return;
        }

        currentAssetIndex = downloadQueue.remove();
        final int assetIndex = currentAssetIndex;
        GithubMetadata.Artifact[] assets = adapter.getMetadata().getAssets();
        GithubMetadata.Artifact asset = assets[assetIndex];
        adapter.setDownloadState(asset, "Downloading 0%");
        refreshVariantRows();
        currentDownloader = new NavitMapDownloader(asset,
                new NavitMapDownloader.DownloadListener() {
                    @Override
                    public void onProgress(long positionBytes, long totalBytes, String text) {
                        final int progress = totalBytes <= 0 ? 0
                                : (int) Math.min(100, positionBytes * 100 / totalBytes);
                        runOnUiThread(() -> {
                            if (isFinishing()) {
                                return;
                            }
                            if (getTstring(R.string.map_download_cancelling).equals(
                                    adapter.getDownloadState(asset))) {
                                return;
                            }
                            adapter.setDownloadState(asset, "Downloading " + progress + "%");
                            refreshVariantRows();
                        });
                    }

                    @Override
                    public void onFinished(boolean success, boolean cancelled) {
                        runOnUiThread(() -> {
                            if (isFinishing()) {
                                return;
                            }
                            adapter.setDownloadState(asset, cancelled ? "Cancelled"
                                    : success ? "Downloaded" : "Failed");
                            queuedDownloads.remove(assetIndex);
                            if (success) {
                                lastDownloadedMap = Navit.sMapFilenamePath + asset.getFilename();
                            }
                            currentDownloader = null;
                            currentAssetIndex = -1;
                            refreshVariantRows();
                            startNextDownload();
                        });
                    }
                });
        currentDownloader.start();
    }

    private static String formatMapVariantName(String filename) {
        String name = filename
                .replaceAll("-20\\d\\d-\\d\\d-\\d\\d\\.bin$", "")
                .replaceAll("^[^-]+-", "");
        StringBuilder result = new StringBuilder();
        for (String word : name.split("-")) {
            if (word.isEmpty()) {
                continue;
            }
            if (result.length() > 0) {
                result.append(' ');
            }
            result.append(Character.toUpperCase(word.charAt(0))).append(word.substring(1));
        }
        return result.length() == 0 ? filename : result.toString();
    }

    @Override
    public boolean onChildClick(ExpandableListView parent, View view, int groupPosition,
            int childPosition, long id) {
        Object child = adapter.getChild(groupPosition, childPosition);
        if (child instanceof GithubMetadataListAdapter.CountryGroup) {
            ArrayList<GithubMetadata.Artifact> assets =
                    new ArrayList<>(adapter.getCountryAssets(groupPosition, childPosition));
            if (assets.size() == 1) {
                GithubMetadata.Artifact asset = assets.get(0);
                String state = adapter.getDownloadState(asset);
                if ("Queued".equals(state)
                        || (state != null && (state.startsWith("Downloading")
                        || state.equals(getTstring(R.string.map_download_cancelling))))) {
                    cancelDownload(asset);
                } else {
                    downloadAsset(asset);
                }
            } else if (!assets.isEmpty()) {
                showVariants(((GithubMetadataListAdapter.CountryGroup) child).name, assets);
            }
            return true;
        }

        if (child instanceof HashMap) {
            @SuppressWarnings("unchecked")
            HashMap<String, String> map = (HashMap<String, String>) child;
            String mapIndex = map.get("map_index");
            if (mapIndex != null) {
                Intent resultIntent = new Intent();
                resultIntent.putExtra("map_index", Integer.parseInt(mapIndex));
                setResult(Activity.RESULT_OK, resultIntent);
                finish();
            } else if (map.get("map_location") != null) {
                askForMapDeletion(map.get("map_location"));
            }
            return true;
        }
        return super.onChildClick(parent, view, groupPosition, childPosition, id);
    }

    @Override
    public void onBackPressed() {
        if (variantsPanel != null && variantsPanel.getVisibility() == View.VISIBLE) {
            showMapList();
        } else {
            super.onBackPressed();
        }
    }

    private void askForMapDeletion(final String mapLocation) {
        AlertDialog.Builder deleteMapBox = new AlertDialog.Builder(this);
        deleteMapBox.setTitle(getTstring(R.string.map_delete));
        deleteMapBox.setCancelable(true);

        NavitMap mapToDelete = new NavitMap(mapLocation);
        deleteMapBox.setMessage(mapToDelete.mMapName + " "
                + mapToDelete.size() / 1024 / 1024 + "MB");
        deleteMapBox.setPositiveButton(getTstring(R.string.yes),
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        Log.d(TAG, "Delete Map");
                        Message msg = Message.obtain(NavitCallbackHandler.sCallbackHandler,
                                NavitCallbackHandler.MsgType.CLB_DELETE_MAP.ordinal());
                        Bundle data = new Bundle();
                        data.putString("title", mapLocation);
                        msg.setData(data);
                        msg.sendToTarget();
                        finish();
                    }
                });
        deleteMapBox.setNegativeButton(getTstring(R.string.no),
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        Log.d(TAG, "don't delete map");
                    }
                });
        deleteMapBox.show();
    }
}
