package org.navitproject.navit;

import java.io.File;

public class NavitMap {
    String mMapName;
    private final String mFileName;
    private final String mMapPath;

    NavitMap(String path, String mapFileName) {
        mMapPath = path;
        mFileName = mapFileName;
        if (mapFileName.endsWith(".bin")) {
            mMapName = mapFileName.substring(0, mapFileName.length() - 4);
        } else {
            mMapName = mapFileName;
        }
    }

    NavitMap(String mapLocation) {
        File mapFile = new File(mapLocation);

        mMapPath = mapFile.getParent() + "/";
        mFileName = mapFile.getName();
        if (mFileName.endsWith(".bin")) {
            mMapName = mFileName.substring(0, mFileName.length() - 4);
        } else {
            mMapName = mFileName;
        }
    }

    public long size() {
        File mapFile = new File(mMapPath + mFileName);
        return mapFile.length();
    }

    public String getLocation() {
        return mMapPath + mFileName;
    }
}
