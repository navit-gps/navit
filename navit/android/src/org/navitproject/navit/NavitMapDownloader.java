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

import android.location.Location;
import android.os.Bundle;
import android.os.Message;
import android.util.Log;

import org.json.*;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;
import java.lang.String;
import java.io.InputStreamReader;
import java.io.BufferedReader;



/**
 * @author hoehnp
 * @author rikky
 *
 */
public class NavitMapDownloader extends Thread {
    //we should try to resume
    private static final int SOCKET_CONNECT_TIMEOUT = 60000;          // 60 secs.
    private static final int SOCKET_READ_TIMEOUT = 120000;         // 120 secs.
    private static final int MAP_WRITE_FILE_BUFFER = 1024 * 64;
    private static final int MAP_WRITE_MEM_BUFFER = 1024 * 64;
    private static final int MAP_READ_FILE_BUFFER = 1024 * 64;
    private static final int UPDATE_PROGRESS_TIME_NS = 250 * 1000000; // 1ns=1E-9s
    private static final int MAX_RETRIES = 5;
    private static final String TAG = "NavitMapDownLoader";

    private final GithubMetadata.Artifact mGitHubArtifactMetadata;
    private final DownloadListener mListener;
    private volatile boolean mStopMe;
    private volatile HttpURLConnection mConnection;
    private long mUiLastUpdated = -1;
    private Boolean mRetryDownload = false; //Download failed, but
    private int mRetryCounter = 0;

    NavitMapDownloader(GithubMetadata.Artifact gitHubArtifactMetadata) {
        this(gitHubArtifactMetadata, null);
    }

    NavitMapDownloader(GithubMetadata.Artifact gitHubArtifactMetadata,
            DownloadListener listener) {
        this.mGitHubArtifactMetadata = gitHubArtifactMetadata;
        this.mListener = listener;
    }

    interface DownloadListener {
        void onProgress(long positionBytes, long totalBytes, String text);
        void onFinished(boolean success, boolean cancelled);
    }

    static NavitMap[] getAvailableMaps() {
        class FilterMaps implements FilenameFilter {

            public boolean accept(File dir, String filename) {
                return (filename.endsWith(".bin"));
            }
        }

        NavitMap[] maps = new NavitMap[0];
        File mapDir = new File(Navit.sMapFilenamePath);
        String[] mapFileNames = mapDir.list(new FilterMaps());
        if (mapFileNames != null) {
            maps = new NavitMap[mapFileNames.length];
            for (int mapFileIndex = 0; mapFileIndex < mapFileNames.length; mapFileIndex++) {
                maps[mapFileIndex] = new NavitMap(Navit.sMapFilenamePath,
                                                  mapFileNames[mapFileIndex]);
            }
        }
        return maps;
    }

    @Override
    public void run() {

        Log.v(TAG, "start download " + mGitHubArtifactMetadata.getFilename());
        updateProgress(0, mGitHubArtifactMetadata.getFileSize(),
                       getTstring(R.string.map_downloading) + ": " + mGitHubArtifactMetadata.getFilename());

        boolean success=false;
        mRetryCounter = 0;
        do {
            if (mStopMe) {
                break;
            }
            try {
                Thread.sleep(10 + mRetryCounter * 1000);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            if (mStopMe) {
                break;
            }
            mRetryDownload = false;
            success = download_osm_map();
        } while (!success
                && mRetryDownload
                && mRetryCounter < MAX_RETRIES
                && !mStopMe);

        if (success) {
            toast(mGitHubArtifactMetadata.getFilename() + " " + getTstring(R.string.map_download_ready));
            Log.d(TAG, "success");
        }
        if (mListener != null) {
            mListener.onFinished(success, mStopMe);
        } else if (success || mStopMe) {
            NavitDialogs.sendDialogMessage(NavitDialogs.MSG_MAP_DOWNLOAD_FINISHED,
                    Navit.sMapFilenamePath + mGitHubArtifactMetadata.getFilename(),
                    null,
                    -1,
                    success ? 1 : 0, 0);
        }

        if (success) {
            toast(mGitHubArtifactMetadata.getFilename() + " " + getTstring(R.string.map_download_ready));
        }

    }

    void stop_thread() {
        mStopMe = true;
        HttpURLConnection connection = mConnection;
        if (connection != null) {
            connection.disconnect();
        }
        Log.d(TAG, "mStopMe -> true");
    }

    private boolean checkFreeSpace(long neededBytes) {
        long freeSpace = NavitUtils.getFreeSpace(Navit.sMapFilenamePath);

        if (neededBytes <= 0) {
            neededBytes = MAP_WRITE_FILE_BUFFER;
        }

        if (freeSpace < neededBytes) {
            String msg;
            Log.e(TAG,
                    "Not enough free space or media not available. Please free at least "
                    + neededBytes / 1024 / 1024 + "Mb.");
            if (freeSpace < 0) {
                msg = getTstring(R.string.map_download_medium_unavailable);
            } else {
                msg = getTstring(R.string.map_download_not_enough_free_space);
            }
            updateProgress(freeSpace, neededBytes,
                           getTstring(R.string.map_download_download_error) + "\n" + msg);
            return false;
        }
        return true;
    }

    private boolean deleteMap() {
        File finalOutputFile = getMapFile();

        if (finalOutputFile.exists()) {
            Message msg = Message.obtain(NavitCallbackHandler.sCallbackHandler,
                                         NavitCallbackHandler.MsgType.CLB_DELETE_MAP.ordinal());
            Bundle b = new Bundle();
            b.putString("title", finalOutputFile.getAbsolutePath());
            msg.setData(b);
            msg.sendToTarget();
            return true;
        }
        return false;
    }

    private boolean download_osm_map() {
        long alreadyRead = 0;
        long realSizeBytes;
        boolean resume = true;

        File outputFile = getDestinationFile();
        long oldDownloadSize = outputFile.length();

        URL url = getDownloadURL();

        URLConnection c = initConnection(url);
        if (c == null) {
            return false;
        }

        try {
            c.setRequestProperty("Range", "bytes=" + oldDownloadSize + "-");
            alreadyRead = oldDownloadSize;
            try {
                realSizeBytes = Long.parseLong(c.getHeaderField("Content-Length")) + alreadyRead;
            } catch (Exception e) {
                realSizeBytes = -1;
            }

            long fileTime = c.getLastModified();

            if (!resume) {
                outputFile.delete();
            }

            if (realSizeBytes <= 0) {
                realSizeBytes = this.mGitHubArtifactMetadata.getFileSize();
            }

            Log.d(TAG, "size: " + realSizeBytes + ", read: " + alreadyRead + ", timestamp: "
                    + fileTime
                    + ", Connection ref: " + c.getURL());

            if (checkFreeSpace(realSizeBytes - alreadyRead)
                    && downloadData(c, alreadyRead, resume, outputFile)) {

                File finalOutputFile = getMapFile();
                if (mStopMe) {
                    return false;
                }
                // delete an already existing file first
                finalOutputFile.delete();
                // rename file to its final name
                outputFile.renameTo(finalOutputFile);
                return true;
            }
            return false;
        } finally {
            ((HttpURLConnection) c).disconnect();
            mConnection = null;
        }
    }

    private File getDestinationFile() {
        File outputFile = new File(Navit.sMapFilenamePath, mGitHubArtifactMetadata.getFilename() + ".tmp");
        outputFile.getParentFile().mkdir();
        return outputFile;
    }

    private boolean downloadData(URLConnection c, long alreadyRead, boolean resume, File outputFile) {
        boolean success = false;
        BufferedOutputStream buf = getOutputStream(outputFile, resume);
        BufferedInputStream bif = getInputStream(c);

        if (buf != null && bif != null) {
            success = readData(buf, bif, alreadyRead);
            // always cleanup, as we might get errors when trying to resume
            try {
                buf.flush();
                buf.close();

                bif.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        return success;
    }

    private URL getDownloadURL() {
        return mGitHubArtifactMetadata.getDowloadURL();
    }


    private BufferedInputStream getInputStream(URLConnection c) {
        BufferedInputStream bif;
        try {
            bif = new BufferedInputStream(c.getInputStream(), MAP_READ_FILE_BUFFER);
        } catch (FileNotFoundException e) {
            Log.e(TAG, "File not found on server: " + e);
            enableRetry();
            bif = null;
        } catch (IOException e) {
            Log.e(TAG, "Error reading from server: " + e);
            enableRetry();
            bif = null;
        }
        return bif;
    }

    private File getMapFile() {
        return new File(Navit.sMapFilenamePath, mGitHubArtifactMetadata.getFilename());
    }

    private static BufferedOutputStream getOutputStream(File outputFile, boolean resume) {
        BufferedOutputStream buf;
        try {
            buf = new BufferedOutputStream(new FileOutputStream(outputFile, resume), MAP_WRITE_FILE_BUFFER);
        } catch (FileNotFoundException e) {
            Log.e(TAG, "Could not open output file for writing: " + e);
            buf = null;
        }
        return buf;
    }

    private URLConnection initConnection(URL url) {
        HttpURLConnection c;
        try {
            c = (HttpURLConnection) url.openConnection();
            c.setRequestMethod("GET");
        } catch (Exception e) {
            Log.e(TAG, "Failed connecting server: " + e);
            enableRetry();
            return null;
        }

        mConnection = c;
        c.setReadTimeout(SOCKET_READ_TIMEOUT);
        c.setConnectTimeout(SOCKET_CONNECT_TIMEOUT);
        if (mStopMe) {
            c.disconnect();
        }
        return c;
    }

    private boolean readData(OutputStream buf, InputStream bif, long alreadyRead) {
        long startTimestamp = System.nanoTime();
        byte[] buffer = new byte[MAP_WRITE_MEM_BUFFER];
        int len1;
        long startOffset = alreadyRead;
        boolean success = false;


        long totalSize = mGitHubArtifactMetadata.getFileSize();
        if (totalSize==0) {
            return false;
        }

        try {
            while (!mStopMe && (len1 = bif.read(buffer)) != -1) {
                alreadyRead += len1;

                updateProgress(startTimestamp, startOffset, alreadyRead, totalSize);

                try {
                    buf.write(buffer, 0, len1);
                } catch (IOException e) {
                    Log.d(TAG, "Error: " + e);
                    if (!checkFreeSpace(totalSize - alreadyRead + MAP_WRITE_FILE_BUFFER)) {
                        if (deleteMap()) {
                            enableRetry();
                        } else {
                            updateProgress(alreadyRead, totalSize,
                                           getTstring(R.string.map_download_download_error) + "\n"
                                           + getTstring(R.string.map_download_not_enough_free_space));
                        }
                    } else {
                        updateProgress(alreadyRead, totalSize,
                                       getTstring(R.string.map_download_error_writing_map));
                    }

                    return false;
                }
            }

            if (mStopMe) {
                //toast(getTstring(R.string.map_download_download_aborted));
            } else if (alreadyRead < totalSize) {
                Log.d(TAG, "Server send only " + alreadyRead + " bytes of " + totalSize);
                enableRetry();
            } else {
                success = true;
            }
        } catch (IOException e) {
            Log.d(TAG, "Error: " + e);

            enableRetry();
            updateProgress(alreadyRead, totalSize,
                           getTstring(R.string.map_download_download_error));
        }

        return success;
    }

    private void toast(String message) {
        NavitDialogs.sendDialogMessage(NavitDialogs.MSG_TOAST, null, message, -1, 0, 0);
    }

    private void updateProgress(long startTime, long offsetBytes, long readBytes, long maxBytes) {
        long currentTime = System.nanoTime();

        if ((currentTime > mUiLastUpdated + UPDATE_PROGRESS_TIME_NS) && startTime != currentTime) {
            float perSecondOverall = (readBytes - offsetBytes) / ((currentTime - startTime) / 1000000000f);
            long bytesRemaining = maxBytes - readBytes;
            int etaSeconds = (int) (bytesRemaining / perSecondOverall);

            String etaString;
            if (etaSeconds > 60) {
                etaString = (int) (etaSeconds / 60f) + " m";
            } else {
                etaString = etaSeconds + " s";
            }
            String info = String.format("%s: %s\n %dMb / %dMb\n %.1f kb/s %s: %s",
                                        getTstring(R.string.map_downloading),
                                        mGitHubArtifactMetadata.getFilename(), readBytes / 1024 / 1024, maxBytes / 1024 / 1024,
                                        perSecondOverall / 1024f, getTstring(R.string.map_download_eta),
                                        etaString);

            if (mRetryCounter > 0) {
                info += "\n Retry " + mRetryCounter + "/" + MAX_RETRIES;
            }
            Log.d(TAG, "info: " + info);

            updateProgress(readBytes, maxBytes, info);
            mUiLastUpdated = currentTime;
        }
    }

    private void updateProgress(long positionBytes, long maximumBytes, String infoText) {
        if (mListener != null) {
            mListener.onProgress(positionBytes, maximumBytes, infoText);
        } else {
            NavitDialogs.sendDialogMessage(NavitDialogs.MSG_PROGRESS_BAR,
                                           getTstring(R.string.map_download_title), infoText,
                                           NavitDialogs.DIALOG_MAPDOWNLOAD,
                                           (int) (maximumBytes / 1024),
                                           (int) (positionBytes / 1024));
        }
    }

    private void enableRetry() {
        mRetryDownload = true;
        mRetryCounter++;
    }

    static class OsmMapValues {

        String mLon1;
        String mLat1;
        String mLon2;
        String mLat2;
        final String mMapName;
	String [] mSubMaps;
        int mLevel;

        private void setMapValues(String lon1, String lat1, String lon2, String lat2, int level) {
            this.mLon1 = lon1;
            this.mLat1 = lat1;
            this.mLon2 = lon2;
            this.mLat2 = lat2;
            this.mLevel = level;
        }

        private OsmMapValues(int id1, String lon1, String lat1, String lon2, String lat2,
                             int level, String[] subMaps) {

            this.mMapName = getTstring(id1);
            setMapValues(lon1, lat1, lon2, lat2, level);
	    this.mSubMaps = subMaps;
        }

	private OsmMapValues(int id1, int id2, String lon1, String lat1, String lon2, String lat2,
                             int level, String[] subMaps) {

            this.mMapName = getTstring(id1) + " + " + getTstring(id2);
            setMapValues(lon1, lat1, lon2, lat2, level);
	    this.mSubMaps = subMaps;
        }


        boolean isInMap(Location location) {

            if (location.getLongitude() < Double.parseDouble(this.mLon1)) {
                return false;
            }
            if (location.getLongitude() > Double.parseDouble(this.mLon2)) {
                return false;
            }
            if (location.getLatitude() < Double.parseDouble(this.mLat1)) {
                return false;
            }
            return !(location.getLatitude() > Double.parseDouble(this.mLat2));
        }
    }
}
