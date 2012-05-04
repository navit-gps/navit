package org.navitproject.navit;

import java.io.File;

public class NavitMap {
	String fileName;
	String mapName;
	String mapPath;

	public NavitMap(String path, String map_file_name) {
		mapPath = path;
		fileName = map_file_name;
		if (map_file_name.endsWith(".bin")) {
			mapName = map_file_name.substring(0, map_file_name.length() - 4);
		} else {
			mapName = map_file_name;
		}
	}

	public NavitMap(String map_location) {
		File mapFile = new File(map_location);
		
		mapPath = mapFile.getParent() + "/";
		fileName = mapFile.getName();
		if (fileName.endsWith(".bin")) {
			mapName = fileName.substring(0, fileName.length() - 4);
		} else {
			mapName = fileName;
		}
	}

	public long size() {
		File map_file = new File(mapPath + fileName);
		return map_file.length();
	}

	public String getLocation() {
		return mapPath + fileName;
	}
}
