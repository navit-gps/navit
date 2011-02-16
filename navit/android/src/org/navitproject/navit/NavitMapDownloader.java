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

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class NavitMapDownloader
{
	public static class osm_map_values
	{
		String	lon1;
		String	lat1;
		String	lon2;
		String	lat2;
		int		est_size_bytes	= 0;

		public osm_map_values(String lon_1, String lat_1, String lon_2, String lat_2, int bytes_est)
		{
			this.lon1 = lon_1;
			this.lat1 = lat_1;
			this.lon2 = lon_2;
			this.lat2 = lat_2;
			this.est_size_bytes = bytes_est;
		}
	}

	/*
	 * coords for predefined maps:
	 * 
	 * <a onclick='predefined(5.18,46.84,15.47,55.64)'>Germany</a>
	 * <a onclick='predefined(2.08,48.87,7.78,54.52)'>BeNeLux</a>
	 * <a onclick='predefined(9.4,46.32,17.21,49.1)'>Austria</a> 209 MBytes = 219152384 Bytes
	 */
	//
	// define the maps here
	//                                                          lon1 , lat1   , lon2   , lat2, est. size in bytes
	public static osm_map_values	austria	= new osm_map_values("9.4", "46.32", "17.21", "49.1",
																219152384);

	public class ProgressThread extends Thread
	{
		Handler			mHandler;
		osm_map_values	map_values;

		ProgressThread(Handler h, osm_map_values map_values)
		{
			this.mHandler = h;
			this.map_values = map_values;
		}

		public void run()
		{
			Log.d("NavitMapDownloader", "run 1");
			download_osm_map(mHandler, map_values);
			Log.d("NavitMapDownloader", "run 2");

			// ok, remove dialog
			Message msg = mHandler.obtainMessage();
			Bundle b = new Bundle();
			msg.what = 0;
			b.putInt("dialog_num", Navit.MAPDOWNLOAD_DIALOG);
			msg.setData(b);
			Log.d("NavitMapDownloader", "run 3");
			mHandler.sendMessage(msg);
			Log.d("NavitMapDownloader", "run 4");

		}
	}

	public void download_osm_map(Handler handler, osm_map_values map_values)
	{
		Message msg = handler.obtainMessage();
		Bundle b = new Bundle();
		msg.what = 1;
		b.putInt("max", map_values.est_size_bytes);
		b.putInt("cur", 0);
		b.putString("title", "Mapdownload");
		b.putString("text", "downloading OSM map");
		msg.setData(b);
		handler.sendMessage(msg);
		try
		{
			// little pause here
			Thread.sleep(10);
		}
		catch (InterruptedException e1)
		{
		}

		try
		{
			URL url = new URL("http://maps.navit-project.org/api/map/?bbox=" + map_values.lon1 + ","
					+ map_values.lat1 + "," + map_values.lon2 + "," + map_values.lat2);
			HttpURLConnection c = (HttpURLConnection) url.openConnection();
			c.setRequestMethod("GET");
			c.setDoOutput(true);
			c.setReadTimeout(5000);
			c.setConnectTimeout(5000);
			c.connect();

			// output filename
			String fileName = "navitmap.tmp";
			// output path for output filename
			// String PATH = Environment.getExternalStorageDirectory() + "/download/";
			String PATH = "/sdcard/";
			Log.v("log_tag", "mapfilename tmp: " + PATH + fileName);
			File file = new File(PATH);
			File outputFile = new File(file, fileName);
			//outputFile.delete();
			// seems this command overwrites the output file anyway
			FileOutputStream fos = new FileOutputStream(outputFile);

			InputStream is = c.getInputStream();

			byte[] buffer = new byte[1024 * 10]; // 10k buffer
			int len1 = 0;
			int already_read = 0;
			int alt = 10; // show progress about every 100k Bytes
			int alt_cur = 0;
			while ((len1 = is.read(buffer)) != -1)
			{
				already_read = already_read + len1;
				alt_cur++;
				if (alt_cur > alt)
				{
					alt_cur = 0;

					msg = handler.obtainMessage();
					b = new Bundle();
					msg.what = 1;
					b.putInt("max", map_values.est_size_bytes);
					b.putInt("cur", already_read);
					b.putString("title", "Mapdownload");
					b.putString("text", "downloading OSM map\n" + (int) (already_read / 1024f / 1024f)
							+ "Mb / " + (int) (map_values.est_size_bytes / 1024f / 1024f) + "Mb");
					msg.setData(b);
					handler.sendMessage(msg);
				}
				fos.write(buffer, 0, len1);
			}
			fos.close();
			is.close();
		}
		catch (IOException e)
		{
			Log.d("NavitMapDownloader", "Error: " + e);
		}

		msg = handler.obtainMessage();
		b = new Bundle();
		msg.what = 1;
		b.putInt("max", map_values.est_size_bytes);
		b.putInt("cur", map_values.est_size_bytes);
		b.putString("title", "Mapdownload");
		b.putString("text", "ready");
		msg.setData(b);
		handler.sendMessage(msg);


		Log.d("NavitMapDownloader", "success");
	}

}
