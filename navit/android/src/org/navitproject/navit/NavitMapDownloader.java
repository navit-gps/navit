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


/*
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
    private static final int UPDATE_PROGRESS_TIME_NS = 1000 * 1000000; // 1ns=1E-9s
    private static final int MAX_RETRIES = 5;
    private static final String TAG = "NavitMapDownLoader";
    private final OsmMapValues mMapValues;
    private final int mMapId;
    private Boolean mStopMe = false;
    private long mUiLastUpdated = -1;
    private Boolean mRetryDownload = false; //Download failed, but
    private int mRetryCounter = 0;

    NavitMapDownloader(int mapId) {
        this.mMapValues = osm_maps[mapId];
        this.mMapId = mapId;
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
        mStopMe = false;
        mRetryCounter = 0;

        Log.v(TAG, "start download " + mMapValues.mMapName);
        updateProgress(0, mMapValues.mEstSizeBytes,
                       getTstring(R.string.map_downloading) + ": " + mMapValues.mMapName);

        boolean success;
        do {
            try {
                Thread.sleep(10 + mRetryCounter * 1000);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            mRetryDownload = false;
            success = download_osm_map();
        } while (!success
                 && mRetryDownload
                 && mRetryCounter < MAX_RETRIES
                 && !mStopMe);

        if (success) {
            toast(mMapValues.mMapName + " " + getTstring(R.string.map_download_ready));
            getMapInfoFile().delete();
            Log.d(TAG, "success");
        }

        if (success || mStopMe) {
            NavitDialogs.sendDialogMessage(NavitDialogs.MSG_MAP_DOWNLOAD_FINISHED,
                                           Navit.sMapFilenamePath + mMapValues.mMapName + ".bin",
                                           null,
                                           -1,
                                           success ? 1 : 0,
                                           mMapId);
        }
    }

    void stop_thread() {
        mStopMe = true;
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

        URL url = null;
        if (oldDownloadSize > 0) {
            url = readFileInfo();
        }

        if (url == null) {
            resume = false;
            url = getDownloadURL();
        }

        URLConnection c = initConnection(url);
        if (c != null) {

            if (resume) {
                c.setRequestProperty("Range", "bytes=" + oldDownloadSize + "-");
                alreadyRead = oldDownloadSize;
            }
            try {
                realSizeBytes = Long.parseLong(c.getHeaderField("Content-Length")) + alreadyRead;
            } catch (Exception e) {
                realSizeBytes = -1;
            }

            long fileTime = c.getLastModified();

            if (!resume) {
                outputFile.delete();
                writeFileInfo(c, realSizeBytes);
            }

            if (realSizeBytes <= 0) {
                realSizeBytes = mMapValues.mEstSizeBytes;
            }

            Log.d(TAG, "size: " + realSizeBytes + ", read: " + alreadyRead + ", timestamp: "
                    + fileTime
                    + ", Connection ref: " + c.getURL());

            if (checkFreeSpace(realSizeBytes - alreadyRead)
                    && downloadData(c, alreadyRead, realSizeBytes, resume, outputFile)) {

                File finalOutputFile = getMapFile();
                // delete an already existing file first
                finalOutputFile.delete();
                // rename file to its final name
                outputFile.renameTo(finalOutputFile);
                return true;
            }
        }
        return false;
    }

    private File getDestinationFile() {
        File outputFile = new File(Navit.sMapFilenamePath, mMapValues.mMapName + ".tmp");
        outputFile.getParentFile().mkdir();
        return outputFile;
    }

    private boolean downloadData(URLConnection c, long alreadyRead, long realSizeBytes, boolean resume,
                                 File outputFile) {
        boolean success = false;
        BufferedOutputStream buf = getOutputStream(outputFile, resume);
        BufferedInputStream bif = getInputStream(c);

        if (buf != null && bif != null) {
            success = readData(buf, bif, alreadyRead, realSizeBytes);
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
        URL url;
        try {
            url =
                new URL("http://maps.navit-project.org/api/map/?bbox=" + mMapValues.mLon1 + ","
                        + mMapValues.mLat1
                        + "," + mMapValues.mLon2 + "," + mMapValues.mLat2);
        } catch (MalformedURLException e) {
            Log.e(TAG, "We failed to create a URL to " + mMapValues.mMapName);
            e.printStackTrace();
            return null;
        }
        Log.v(TAG, "connect to " + url.toString());
        return url;
    }


    private BufferedInputStream getInputStream(URLConnection c) {
        BufferedInputStream bif;
        try {
            bif = new BufferedInputStream(c.getInputStream(), MAP_READ_FILE_BUFFER);
        } catch (FileNotFoundException e) {
            Log.e(TAG, "File not found on server: " + e);
            if (mRetryCounter > 0) {
                getMapInfoFile().delete();
            }
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
        return new File(Navit.sMapFilenamePath, mMapValues.mMapName + ".bin");
    }

    private File getMapInfoFile() {
        return new File(Navit.sMapFilenamePath, mMapValues.mMapName + ".tmp.info");
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

        c.setReadTimeout(SOCKET_READ_TIMEOUT);
        c.setConnectTimeout(SOCKET_CONNECT_TIMEOUT);
        return c;
    }

    private boolean readData(OutputStream buf, InputStream bif, long alreadyRead,
                             long realSizeBytes) {
        long startTimestamp = System.nanoTime();
        byte[] buffer = new byte[MAP_WRITE_MEM_BUFFER];
        int len1;
        long startOffset = alreadyRead;
        boolean success = false;

        try {
            while (!mStopMe && (len1 = bif.read(buffer)) != -1) {
                alreadyRead += len1;
                updateProgress(startTimestamp, startOffset, alreadyRead, realSizeBytes);

                try {
                    buf.write(buffer, 0, len1);
                } catch (IOException e) {
                    Log.d(TAG, "Error: " + e);
                    if (!checkFreeSpace(realSizeBytes - alreadyRead + MAP_WRITE_FILE_BUFFER)) {
                        if (deleteMap()) {
                            enableRetry();
                        } else {
                            updateProgress(alreadyRead, realSizeBytes,
                                           getTstring(R.string.map_download_download_error) + "\n"
                                           + getTstring(R.string.map_download_not_enough_free_space));
                        }
                    } else {
                        updateProgress(alreadyRead, realSizeBytes,
                                       getTstring(R.string.map_download_error_writing_map));
                    }

                    return false;
                }
            }

            if (mStopMe) {
                toast(getTstring(R.string.map_download_download_aborted));
            } else if (alreadyRead < realSizeBytes) {
                Log.d(TAG, "Server send only " + alreadyRead + " bytes of " + realSizeBytes);
                enableRetry();
            } else {
                success = true;
            }
        } catch (IOException e) {
            Log.d(TAG, "Error: " + e);

            enableRetry();
            updateProgress(alreadyRead, realSizeBytes,
                           getTstring(R.string.map_download_download_error));
        }

        return success;
    }

    private URL readFileInfo() {
        URL url = null;
        try {
            ObjectInputStream infoStream = new ObjectInputStream(
                    new FileInputStream(getMapInfoFile()));
            infoStream.readUTF(); // read the host name (unused for now)
            String resumeFile = infoStream.readUTF();
            infoStream.close();
            // looks like the same file, try to resume
            Log.v(TAG, "Try to resume download");
            url = new URL("https://" + "maps.navit-project.org" + resumeFile);
        } catch (Exception e) {
            getMapInfoFile().delete();
        }
        return url;
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
                                        mMapValues.mMapName, readBytes / 1024 / 1024, maxBytes / 1024 / 1024,
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
        NavitDialogs.sendDialogMessage(NavitDialogs.MSG_PROGRESS_BAR,
                                       getTstring(R.string.map_download_title), infoText,
                                       NavitDialogs.DIALOG_MAPDOWNLOAD, (int) (maximumBytes / 1024),
                                       (int) (positionBytes / 1024));
    }

    private void writeFileInfo(URLConnection c, long sizeInBytes) {
        ObjectOutputStream infoStream;
        try {
            infoStream = new ObjectOutputStream(new FileOutputStream(getMapInfoFile()));
            infoStream.writeUTF(c.getURL().getProtocol());
            infoStream.writeUTF(c.getURL().getHost());
            infoStream.writeUTF(c.getURL().getFile());
            infoStream.writeLong(sizeInBytes);
            infoStream.close();
        } catch (Exception e) {
            Log.e(TAG,
                    "Could not write info file for map download. Resuming will not be possible. ("
                    + e.getMessage() + ")");
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
        long mEstSizeBytes;
        int mLevel;

        private void setMapValues(String lon1, String lat1, String lon2, String lat2, long bytesEst, int level) {
            this.mLon1 = lon1;
            this.mLat1 = lat1;
            this.mLon2 = lon2;
            this.mLat2 = lat2;
            this.mEstSizeBytes = bytesEst;
            this.mLevel = level;
        }

        private OsmMapValues(int id1, String lon1, String lat1, String lon2, String lat2,
                             long bytesEst, int level) {

            this.mMapName = getTstring(id1);
            setMapValues(lon1, lat1, lon2, lat2, bytesEst, level);
        }

        private OsmMapValues(int id1, int id2, String lon1, String lat1, String lon2, String lat2,
                             long bytesEst, int level) {

            this.mMapName = getTstring(id1) + " + " + getTstring(id2);
            setMapValues(lon1, lat1, lon2, lat2, bytesEst, level);
        }


        private OsmMapValues(int id1, int id2, int id3, String lon1, String lat1, String lon2, String lat2,
                             long bytesEst, int level) {

            mMapName = getTstring(id1) + " + " + getTstring(id2) + " + " + getTstring(id3);
            setMapValues(lon1, lat1, lon2, lat2, bytesEst, level);
        }


        boolean isInMap(Location location) {

            if (location.getLongitude() < Double.valueOf(this.mLon1)) {
                return false;
            }
            if (location.getLongitude() > Double.valueOf(this.mLon2)) {
                return false;
            }
            if (location.getLatitude() < Double.valueOf(this.mLat1)) {
                return false;
            }
            return !(location.getLatitude() > Double.valueOf(this.mLat2));
        }
    }
}
