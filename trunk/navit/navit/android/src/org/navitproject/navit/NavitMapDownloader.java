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

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.text.DecimalFormat;
import java.text.NumberFormat;

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
		String	map_name			= "";
		int		est_size_bytes	= 0;

		public osm_map_values(String mapname, String lon_1, String lat_1, String lon_2, String lat_2,
				int bytes_est)
		{
			this.map_name = mapname;
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
	//                                                          name     , lon1 , lat1   , lon2   , lat2, est. size in bytes
	public static osm_map_values		austria								= new osm_map_values("Austria",
																								"9.4", "46.32", "17.21",
																								"49.1", 222000000);
	public static osm_map_values		benelux								= new osm_map_values("BeNeLux",
																								"2.08", "48.87", "7.78",
																								"54.52", 530000000);
	public static osm_map_values		germany								= new osm_map_values("Germany",
																								"5.18", "46.84", "15.47",
																								"55.64", 943000000);
	public static osm_map_values[]	OSM_MAPS								= new osm_map_values[]{
			NavitMapDownloader.austria, NavitMapDownloader.germany, NavitMapDownloader.benelux};
	public static String[]				OSM_MAP_NAME_LIST					= new String[]{
			NavitMapDownloader.OSM_MAPS[0].map_name, NavitMapDownloader.OSM_MAPS[1].map_name,
			NavitMapDownloader.OSM_MAPS[2].map_name						};

	public Boolean							stop_me								= false;
	static final int						SOCKET_CONNECT_TIMEOUT			= 6000;
	static final int						SOCKET_READ_TIMEOUT				= 6000;
	static final int						MAP_WRITE_FILE_BUFFER			= 1024 * 64;
	static final int						MAP_WRITE_MEM_BUFFER				= 1024 * 64;
	static final int						MAP_READ_FILE_BUFFER				= 1024 * 64;
	static final int						UPDATE_PROGRESS_EVERY_CYCLE	= 2;


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
			stop_me = false;
			download_osm_map(mHandler, map_values);

			// ok, remove dialog
			Message msg = mHandler.obtainMessage();
			Bundle b = new Bundle();
			msg.what = 0;
			b.putInt("dialog_num", Navit.MAPDOWNLOAD_DIALOG);
			msg.setData(b);
			mHandler.sendMessage(msg);
		}

		public void stop_thread()
		{
			stop_me = true;
			Log.d("NavitMapDownloader", "stop_me -> true");
		}
	}

	public Navit	navit_jmain	= null;

	public NavitMapDownloader(Navit main)
	{
		this.navit_jmain = main;
	}

	public void download_osm_map(Handler handler, osm_map_values map_values)
	{
		Message msg = handler.obtainMessage();
		Bundle b = new Bundle();
		msg.what = 1;
		b.putInt("max", map_values.est_size_bytes);
		b.putInt("cur", 0);
		b.putString("title", "Mapdownload");
		b.putString("text", "downloading: " + map_values.map_name);
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

		// output filename
		String fileName = "navitmap.tmp";
		String final_fileName = "navitmap.bin";
		// output path for output filename
		// String PATH = Environment.getExternalStorageDirectory() + "/download/";
		String PATH = "/sdcard/";
		Log.v("log_tag", "mapfilename tmp: " + PATH + fileName);

		try
		{
			URL url = new URL("http://maps.navit-project.org/api/map/?bbox=" + map_values.lon1 + ","
					+ map_values.lat1 + "," + map_values.lon2 + "," + map_values.lat2);
			HttpURLConnection c = (HttpURLConnection) url.openConnection();
			c.setRequestMethod("GET");
			c.setDoOutput(true);
			c.setReadTimeout(SOCKET_READ_TIMEOUT);
			c.setConnectTimeout(SOCKET_CONNECT_TIMEOUT);
			int real_size_bytes = c.getContentLength();
			c.connect();

			if (real_size_bytes > 0)
			{
				// change the estimated filesize to reported filesize
				map_values.est_size_bytes = real_size_bytes;
			}

			File file = new File(PATH);
			File outputFile = new File(file, fileName);
			File final_outputFile = new File(file, final_fileName);
			// tests have shown that deleting the file first is sometimes faster -> so we delete it (who cares)
			outputFile.delete();
			// seems this command overwrites the output file anyway
			FileOutputStream fos = new FileOutputStream(outputFile);
			BufferedOutputStream buf = new BufferedOutputStream(fos, MAP_WRITE_FILE_BUFFER); // buffer

			InputStream is = c.getInputStream();
			BufferedInputStream bif = new BufferedInputStream(is, MAP_READ_FILE_BUFFER); // buffer

			byte[] buffer = new byte[MAP_WRITE_MEM_BUFFER]; // buffer
			int len1 = 0;
			int already_read = 0;
			int alt = UPDATE_PROGRESS_EVERY_CYCLE; // show progress about every xx cylces
			int alt_cur = 0;
			String kbytes_per_second = "";
			//long last_timestamp = 0;
			long start_timestamp = System.currentTimeMillis();
			//int last_bytes = 0;
			NumberFormat formatter = new DecimalFormat("00000.0");
			String eta_string = "";
			float per_second_overall = 0f;
			int bytes_remaining = 0;
			int eta_seconds = 0;
			//while ((len1 = is.read(buffer)) != -1)
			while ((len1 = bif.read(buffer)) != -1)
			{
				if (stop_me)
				{
					// ok we need to be stopped! close all files and end
					Log.d("NavitMapDownloader", "stop_me 1");
					buf.flush();
					buf.close();
					fos.close();
					bif.close();
					is.close();
					Log.d("NavitMapDownloader", "stop_me 2");
					c.disconnect();
					Log.d("NavitMapDownloader", "stop_me 3");
					return;
				}
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
					b.putString("title", "Map download");
					//					if (last_timestamp == 0)
					//					{
					//						kbytes_per_second = "--";
					//					}
					//					else
					//					{
					//						//float temp = (((already_read - last_bytes) / 1024f) / ((System
					//						//		.currentTimeMillis() - last_timestamp) / 1000f));
					//						//kbytes_per_second = formatter.format(temp);
					//					}
					//last_timestamp = System.currentTimeMillis();
					//last_bytes = already_read;
					per_second_overall = (float) already_read
							/ (float) ((System.currentTimeMillis() - start_timestamp) / 1000);
					kbytes_per_second = formatter.format((per_second_overall / 1024f));
					// Log.d("NavitMapDownloader", "k " + kbytes_per_second);
					bytes_remaining = map_values.est_size_bytes - already_read;
					eta_seconds = (int) ((float) bytes_remaining / (float) per_second_overall);
					if (eta_seconds > 60)
					{
						eta_string = (int) (eta_seconds / 60f) + " m";
					}
					else
					{
						eta_string = eta_seconds + " s";
					}
					b.putString("text", "downloading: " + map_values.map_name + "\n" + " "
							+ (int) (already_read / 1024f / 1024f) + "Mb / "
							+ (int) (map_values.est_size_bytes / 1024f / 1024f) + "Mb" + "\n" + " "
							+ kbytes_per_second + "kb/s" + " ETA: " + eta_string);
					msg.setData(b);
					handler.sendMessage(msg);
					//					try
					//					{
					//						// little pause here
					//						Thread.sleep(50);
					//					}
					//					catch (InterruptedException e1)
					//					{
					//					}
				}
				//fos.write(buffer, 0, len1);
				buf.write(buffer, 0, len1);
			}
			buf.flush();

			buf.close();
			fos.close();

			bif.close();
			is.close();

			c.disconnect();

			// delete an already final filename, first
			final_outputFile.delete();
			// rename file to final name
			outputFile.renameTo(final_outputFile);
		}
		catch (IOException e)
		{
			msg = handler.obtainMessage();
			b = new Bundle();
			msg.what = 2;
			b.putString("text", "Error downloading map!");
			msg.setData(b);
			handler.sendMessage(msg);

			Log.d("NavitMapDownloader", "Error: " + e);
		}
		catch (Exception e)
		{
			msg = handler.obtainMessage();
			b = new Bundle();
			msg.what = 2;
			b.putString("text", "Error downloading map!");
			msg.setData(b);
			handler.sendMessage(msg);

			Log.d("NavitMapDownloader", "gerneral Error: " + e);
		}

		msg = handler.obtainMessage();
		b = new Bundle();
		msg.what = 1;
		b.putInt("max", map_values.est_size_bytes);
		b.putInt("cur", map_values.est_size_bytes);
		b.putString("title", "Mapdownload");
		b.putString("text", map_values.map_name + " ready");
		msg.setData(b);
		handler.sendMessage(msg);


		Log.d("NavitMapDownloader", "success");
	}
}
