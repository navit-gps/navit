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
		String	map_name								= "";
		long		est_size_bytes						= 0;
		String	est_size_bytes_human_string	= "";
		String	text_for_select_list				= "";
		Boolean	is_continent						= false;
		int		continent_id						= 0;


		public osm_map_values(String mapname, String lon_1, String lat_1, String lon_2, String lat_2,
				long bytes_est, Boolean is_con, int con_id)
		{
			this.is_continent = is_con;
			this.continent_id = con_id;
			this.map_name = mapname;
			this.lon1 = lon_1;
			this.lat1 = lat_1;
			this.lon2 = lon_2;
			this.lat2 = lat_2;
			this.est_size_bytes = bytes_est;
			this.est_size_bytes_human_string = " ca. "
					+ (int) ((float) (this.est_size_bytes) / 1024f / 1024f) + "MB";
			this.text_for_select_list = this.map_name + " " + this.est_size_bytes_human_string;
		}
	}
	//
	// define the maps here
	//
	static final osm_map_values[] osm_maps = {
		new osm_map_values("Whole Planet", "-180", "-90", "180", "90", 5985878379L, true, 0),
		new osm_map_values("Africa", "-20.8", "-35.2", "52.5", "37.4", 180836389L, true, 1),
		new osm_map_values("Angola", "11.4", "-18.1", "24.2", "-5.3", 56041641L, false, 1),
		new osm_map_values("Burundi", "28.9", "-4.5", "30.9", "-2.2", 56512924L, false, 1),
		new osm_map_values("Democratic Republic of the Congo", "11.7", "-13.6", "31.5", "5.7",65026791L, false, 1),
		new osm_map_values("Kenya", "33.8","-5.2", "42.4", "4.9", 58545273L, false, 1),
		new osm_map_values("Lesotho", "26.9", "-30.7", "29.6","-28.4", 54791041L, false, 1),
		new osm_map_values("Madagascar", "43.0","-25.8", "50.8","-11.8", 56801099L, false, 1),
		new osm_map_values("Nambia+Botswana","11.4", "-29.1","29.5", "-16.9", 61807049L, false, 1),
		new osm_map_values("Reunion", "55.2","-21.4", "55.9","-20.9", 58537419L, false, 1),
		new osm_map_values("Rwanda", "28.8","-2.9", "30.9","-1.0", 56313710L, false, 1),
		new osm_map_values("South Africa","15.6", "-35.2","33.3", "-21.9", 73545245L, false, 1),
		new osm_map_values("Uganda", "29.3","-1.6", "35.1","4.3", 57376589L, false, 1),
		new osm_map_values("Asia", "23.8","0.1", "195.0","82.4", 797725952L, true, 2),
		new osm_map_values("China", "67.3","5.3", "135.0","54.5", 259945160L, false, 2),
		new osm_map_values("Cyprus", "32.0","34.5", "34.9","35.8", 58585278L, false, 2),
		new osm_map_values("India+Nepal","67.9", "5.5","89.6", "36.0", 82819344L, false, 2),
		new osm_map_values("Indonesia", "93.7","-17.3", "155.5","7.6", 74648081L, false, 2),
		new osm_map_values("Iran", "43.5","24.4", "63.6","40.4", 69561312L, false, 2),
		new osm_map_values("Iraq", "38.7","28.5", "49.2","37.4", 59146383L, false, 2),
		new osm_map_values("Israel", "33.99","29.8", "35.95","33.4", 65065351L, false, 2),
		new osm_map_values("Japan+Korea+Taiwan","117.6", "20.5","151.3", "47.1", 305538751L, false, 2),
		new osm_map_values("Malasia+Singapore","94.3", "-5.9","108.6", "6.8", 58849792L, false, 2),
		new osm_map_values("Mongolia", "87.5","41.4", "120.3","52.7", 60871187L, false, 2),
		new osm_map_values("Thailand", "97.5","5.7", "105.2","19.7", 62422864L, false, 2),
		new osm_map_values("Turkey", "25.1","35.8", "46.4","42.8", 81758047L, false, 2),
		new osm_map_values("UAE+Other", "51.5","22.6", "56.7","26.5", 57419510L, false, 2),
		new osm_map_values("Australia", "110.5","-44.2", "154.9","-9.2", 128502185L, true, 3),
		new osm_map_values("Australia", "110.5","-44.2", "154.9","-9.2", 128502185L, false, 3),
		new osm_map_values("Tasmania", "144.0","-45.1", "155.3","-24.8", 103573989L, false, 3),
		new osm_map_values("Victoria+New South Wales","140.7", "-39.4","153.7", "-26.9", 99307594L, false, 3),
		new osm_map_values("New Zealand","165.2", "-47.6","179.1", "-33.7", 64757454L, false, 3),
		new osm_map_values("Europe", "-12.97","33.59", "34.15","72.10", 2753910015L, true, 4),
		new osm_map_values("Western Europe","-17.6", "34.5","42.9", "70.9", 2832986851L, false, 4),
		new osm_map_values("Austria", "9.4","46.32", "17.21","49.1", 222359992L, false, 4),
		new osm_map_values("BeNeLux", "2.08","48.87", "7.78","54.52", 533865194L, false, 4),
		new osm_map_values("Faroe Islands","-7.8", "61.3","-6.1", "62.5", 54526101L, false, 4),
		new osm_map_values("France", "-5.45","42.00", "8.44","51.68", 1112047845L, false, 4),
		new osm_map_values("Germany", "5.18","46.84", "15.47","55.64", 944716238L, false, 4),
		new osm_map_values("Bavaria", "10.3","47.8", "13.6","49.7", 131799419L, false, 4),
		new osm_map_values("Saxonia", "11.8","50.1", "15.0","51.7", 112073909L, false, 4),
		new osm_map_values("Germany+Austria+Switzerland","3.4", "44.5","18.6", "55.1", 1385785353L, false, 4),
		new osm_map_values("Iceland", "-25.3","62.8", "-11.4","67.5", 57281405L, false, 4),
		new osm_map_values("Ireland", "-11.17","51.25", "-5.23","55.9", 70186936L, false, 4),
		new osm_map_values("Italy", "6.52","36.38", "18.96","47.19", 291401314L, false, 4),
		new osm_map_values("Spain+Portugal","-11.04", "34.87","4.62", "44.41", 292407746L, false, 4),
		new osm_map_values("Mallorca", "2.2","38.8", "4.7","40.2", 59700600L, false, 4),
		new osm_map_values("Galicia", "-10.0","41.7", "-6.3","44.1", 64605237L, false, 4),
		new osm_map_values("Scandinavia", "4.0","54.4", "32.1","71.5", 299021928L, false, 4),
		new osm_map_values("Finland", "18.6","59.2", "32.3","70.3", 128871467L, false, 4),
		new osm_map_values("Denmark", "7.49","54.33", "13.05","57.88", 120025875L, false, 4),
		new osm_map_values("Switzerland","5.79", "45.74","10.59", "47.84", 162616817L, false, 4),
		new osm_map_values("UK", "-9.7", "49.6","2.2", "61.2", 245161510L, false, 4),
		new osm_map_values("Bulgaria", "24.7","42.1", "24.8","42.1", 56607427L, false, 4),
		new osm_map_values("Czech Republic","11.91", "48.48","19.02", "51.17", 234138824L, false, 4),
		new osm_map_values("Croatia", "13.4","42.1", "19.4","46.9", 99183280L, false, 4),
		new osm_map_values("Estonia", "21.5","57.5", "28.2","59.6", 79276178L, false, 4),
		new osm_map_values("Greece", "28.9","37.8", "29.0","37.8", 55486527L, false, 4),
		new osm_map_values("Crete", "23.3","34.5", "26.8","36.0", 57032630L, false, 4),
		new osm_map_values("Hungary", "16.08","45.57", "23.03","48.39", 109831319L, false, 4),
		new osm_map_values("Latvia", "20.7","55.6", "28.3","58.1", 71490706L, false, 4),
		new osm_map_values("Lithuania", "20.9","53.8", "26.9","56.5", 67992457L, false, 4),
		new osm_map_values("Poland", "13.6","48.8", "24.5","55.0", 266136768L, false, 4),
		new osm_map_values("Romania", "20.3","43.5", "29.9","48.4", 134525863L, false, 4),
		new osm_map_values("North America","-178.1", "6.5","-10.4", "84.0", 2477309662L, true, 5),
		new osm_map_values("Alaska", "-179.5","49.5", "-129","71.6", 72320027L, false, 5),
		new osm_map_values("Canada", "-141.3","41.5", "-52.2","70.2", 937813467L, false, 5),
		new osm_map_values("Hawaii", "-161.07","18.49", "-154.45","22.85", 57311788L, false, 5),
		new osm_map_values("USA (except Alaska and Hawaii)","-125.4", "24.3","-66.5", "49.3", 2216912004L, false, 5),
		new osm_map_values("Nevada", "-120.2","35.0", "-113.8","42.1", 136754975L, false, 5),
		new osm_map_values("Oregon", "-124.8","41.8", "-116.3","46.3", 101627308L, false, 5),
		new osm_map_values("Washington State","-125.0", "45.5","-116.9", "49.0", 98178877L, false, 5),
		new osm_map_values("South+Middle America","-83.5", "-56.3","-30.8", "13.7", 159615197L, true, 6),
		new osm_map_values("Argentina", "-73.9","-57.3", "-51.6","-21.0", 87516152L, false, 6),
		new osm_map_values("Argentina+Chile","-77.2", "-56.3","-52.7", "-16.1", 91976696L, false, 6),
		new osm_map_values("Bolivia", "-70.5","-23.1", "-57.3","-9.3", 58242168L, false, 6),
		new osm_map_values("Brazil", "-71.4","-34.7", "-32.8","5.4", 105527899L, false, 6),
		new osm_map_values("Cuba", "-85.3","19.6", "-74.0","23.6", 56608942L, false, 6),
		new osm_map_values("Colombia", "-79.1","-4.0", "-66.7","12.6", 78658454L, false, 6),
		new osm_map_values("Ecuador", "-82.6","-5.4", "-74.4","2.3", 61501914L, false, 6),
		new osm_map_values("Guyana+Suriname+Guyane Francaise","-62.0", "1.0","-51.2", "8.9", 57040689L, false, 6),
		new osm_map_values("Haiti+Republica Dominicana","-74.8", "17.3","-68.2", "20.1", 63528584L, false, 6),
		new osm_map_values("Jamaica", "-78.6","17.4", "-75.9","18.9", 53958307L, false, 6),
		new osm_map_values("Mexico", "-117.6","14.1", "-86.4","32.8", 251108617L, false, 6),
		new osm_map_values("Paraguay", "-63.8","-28.1", "-53.6","-18.8", 57188715L, false, 6),
		new osm_map_values("Peru", "-82.4","-18.1", "-67.5","0.4", 65421441L, false, 6),
		new osm_map_values("Uruguay", "-59.2","-36.5", "-51.7","-29.7", 63542225L, false, 6),
		new osm_map_values("Venezuela", "-73.6","0.4", "-59.7","12.8", 64838882L, false, 6)
	};

	public static String[]			OSM_MAP_NAME_LIST_inkl_SIZE_ESTIMATE	= null;

	public static int[]				OSM_MAP_NAME_ORIG_ID_LIST					= null;

	private static Boolean			already_inited									= false;

	public Boolean						stop_me											= false;
	static final int					SOCKET_CONNECT_TIMEOUT						= 25000;							// 25 secs.
	static final int					SOCKET_READ_TIMEOUT							= 15000;							// 15 secs.
	static final int					MAP_WRITE_FILE_BUFFER						= 1024 * 64;
	static final int					MAP_WRITE_MEM_BUFFER							= 1024 * 64;
	static final int					MAP_READ_FILE_BUFFER							= 1024 * 64;
	static final int					UPDATE_PROGRESS_EVERY_CYCLE				= 8;

	static final String				DOWNLOAD_FILENAME								= "navitmap.tmp";
	static final String				MAP_FILENAME_PRI								= "navitmap.bin";
	static final String				MAP_FILENAME_SEC								= "navitmap_002.bin";
	static final String				MAP_FILENAME_PATH								= Navit.MAP_FILENAME_PATH;

	static final int					MAX_MAP_COUNT									= 200;

	public class ProgressThread extends Thread
	{
		Handler			mHandler;
		osm_map_values	map_values;
		int				map_num;
		int				my_dialog_num;

		ProgressThread(Handler h, osm_map_values map_values, int map_num2)
		{
			this.mHandler = h;
			this.map_values = map_values;
			this.map_num = map_num2;
			if (this.map_num == Navit.MAP_NUM_PRIMARY)
			{
				this.my_dialog_num = Navit.MAPDOWNLOAD_PRI_DIALOG;
			}
			else if (this.map_num == Navit.MAP_NUM_SECONDARY)
			{
				this.my_dialog_num = Navit.MAPDOWNLOAD_SEC_DIALOG;
			}
		}

		public void run()
		{
			stop_me = false;
			int exit_code = download_osm_map(mHandler, map_values, this.map_num);

			// ok, remove dialog
			Message msg = mHandler.obtainMessage();
			Bundle b = new Bundle();
			msg.what = 0;
			b.putInt("dialog_num", this.my_dialog_num);
			b.putInt("exit_code", exit_code);
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

	public static void init()
	{
		// need only init once
		if (already_inited) { return; }

		//String[] temp_m = new String[MAX_MAP_COUNT];
		String[] temp_ml = new String[MAX_MAP_COUNT];
		int[] temp_i = new int[MAX_MAP_COUNT];
		Boolean[] already_added = new Boolean[osm_maps.length];
		int cur_continent = -1;
		int count = 0;
		Log.v("NavitMapDownloader", "init maps");
		for (int i = 0; i < osm_maps.length; i++)
		{
			already_added[i] = false;
		}
		for (int i = 0; i < osm_maps.length; i++)
		{
			//Log.v("NavitMapDownloader", "i=" + i);
			// look for continents only
			if (osm_maps[i].is_continent)
			{
				if (count > 0)
				{
					// add a break into list
					//temp_m[count] = "*break*";
					temp_ml[count] = "======";
					temp_i[count] = -1;
					count++;
				}

				cur_continent = osm_maps[i].continent_id;
				//Log.v("NavitMapDownloader", "found cont=" + cur_continent);
				// add this cont.
				//temp_m[count] = OSM_MAPS[i].map_name;
				temp_ml[count] = osm_maps[i].text_for_select_list;
				temp_i[count] = i;
				count++;
				already_added[i] = true;
				for (int j = 0; j < osm_maps.length; j++)
				{
					// if (already_added[j] == null)
					if (!already_added[j])
					{
						// look for maps in that continent
						if ((osm_maps[j].continent_id == cur_continent) && (!osm_maps[j].is_continent))
						{
							//Log.v("NavitMapDownloader", "found map=" + j + " c=" + cur_continent);
							// add this map.
							//temp_m[count] = OSM_MAPS[j].map_name;
							temp_ml[count] = " * " + osm_maps[j].text_for_select_list;
							temp_i[count] = j;
							count++;
							already_added[j] = true;
						}
					}
				}
			}
		}
		// add the rest of the list (dont have a continent)
		cur_continent = 9999; // unknown
		int found = 0;
		for (int i = 0; i < osm_maps.length; i++)
		{
			if (!already_added[i])
			{
				if (found == 0)
				{
					found = 1;
					// add a break into list
					//temp_m[count] = "*break*";
					temp_ml[count] = "======";
					temp_i[count] = -1;
					count++;
				}

				//Log.v("NavitMapDownloader", "found map(loose)=" + i + " c=" + cur_continent);
				// add this map.
				//temp_m[count] = OSM_MAPS[i].map_name;
				temp_ml[count] = " # " + osm_maps[i].text_for_select_list;
				temp_i[count] = i;
				count++;
				already_added[i] = true;
			}
		}

		Log.e("NavitMapDownloader", "count=" + count);
		Log.e("NavitMapDownloader", "size1 " + osm_maps.length);
		//Log.e("NavitMapDownloader", "size2 " + temp_m.length);
		Log.e("NavitMapDownloader", "size3 " + temp_ml.length);

		//OSM_MAP_NAME_LIST = new String[count];
		OSM_MAP_NAME_LIST_inkl_SIZE_ESTIMATE = new String[count];
		OSM_MAP_NAME_ORIG_ID_LIST = new int[count];

		for (int i = 0; i < count; i++)
		{
			//OSM_MAP_NAME_LIST[i] = temp_m[i];
			OSM_MAP_NAME_ORIG_ID_LIST[i] = temp_i[i];
			OSM_MAP_NAME_LIST_inkl_SIZE_ESTIMATE[i] = temp_ml[i];
		}

		already_inited = true;
	}

	public int download_osm_map(Handler handler, osm_map_values map_values, int map_num3)
	{
		int exit_code = 1;

		//Log.v("NavitMapDownloader", "map_num3=" + map_num3);
		int my_dialog_num = 0;
		if (map_num3 == Navit.MAP_NUM_PRIMARY)
		{
			my_dialog_num = Navit.MAPDOWNLOAD_PRI_DIALOG;
			//Log.v("NavitMapDownloader", "PRI");
		}
		else if (map_num3 == Navit.MAP_NUM_SECONDARY)
		{
			my_dialog_num = Navit.MAPDOWNLOAD_SEC_DIALOG;
			//Log.v("NavitMapDownloader", "SEC");
		}
		//Log.v("NavitMapDownloader", "map_num3=" + map_num3);

		Message msg = handler.obtainMessage();
		Bundle b = new Bundle();
		msg.what = 1;
		b.putInt("max", 20); // use a dummy number here
		b.putInt("cur", 0);
		b.putInt("dialog_num", my_dialog_num);
		b.putString("title", Navit.get_text("Mapdownload")); //TRANS
		b.putString("text", Navit.get_text("downloading") + ": " + map_values.map_name); //TRANS
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
		String PATH = MAP_FILENAME_PATH;
		String fileName = DOWNLOAD_FILENAME;
		String final_fileName = "xxx";
		//Log.v("NavitMapDownloader", "map_num3=" + map_num3);
		if (map_num3 == Navit.MAP_NUM_SECONDARY)
		{
			final_fileName = MAP_FILENAME_SEC;
		}
		else if (map_num3 == Navit.MAP_NUM_PRIMARY)
		{
			final_fileName = MAP_FILENAME_PRI;
		}
		// output path for output filename
		// String PATH = Environment.getExternalStorageDirectory() + "/download/";

		try
		{
			URL url = new URL("http://maps.navit-project.org/api/map/?bbox=" + map_values.lon1 + ","
					+ map_values.lat1 + "," + map_values.lon2 + "," + map_values.lat2);
			HttpURLConnection c = (HttpURLConnection) url.openConnection();
			c.setRequestMethod("GET");
			c.setDoOutput(true);
			c.setReadTimeout(SOCKET_READ_TIMEOUT);
			c.setConnectTimeout(SOCKET_CONNECT_TIMEOUT);
			long real_size_bytes = c.getContentLength();
			c.connect();

			Log.d("NavitMapDownloader", "real size in bytes: " + real_size_bytes);
			if (real_size_bytes > 20)
			{
				// change the estimated filesize to reported filesize
				map_values.est_size_bytes = real_size_bytes;
			}
			Log.d("NavitMapDownloader", "size in bytes: " + map_values.est_size_bytes);

			File file = new File(PATH);
			File outputFile = new File(file, fileName);
			File final_outputFile = new File(file, final_fileName);
			// tests have shown that deleting the file first is sometimes faster -> so we delete it (who knows)
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
			long start_timestamp = System.currentTimeMillis();
			NumberFormat formatter = new DecimalFormat("00000.0");
			String eta_string = "";
			float per_second_overall = 0f;
			long bytes_remaining = 0;
			int eta_seconds = 0;
			while ((len1 = bif.read(buffer)) != -1)
			{
				if (stop_me)
				{
					// ok we need to be stopped! close all files and end
					buf.flush();
					buf.close();
					fos.close();
					bif.close();
					is.close();
					c.disconnect();
					return 2;
				}
				already_read = already_read + len1;
				alt_cur++;
				if (alt_cur > alt)
				{
					alt_cur = 0;

					msg = handler.obtainMessage();
					b = new Bundle();
					msg.what = 1;
					b.putInt("max", (int) (map_values.est_size_bytes / 1024));
					b.putInt("cur", (int) (already_read / 1024));
					b.putInt("dialog_num", my_dialog_num);
					b.putString("title", Navit.get_text("Mapdownload")); //TRANS
					per_second_overall = (float) already_read
							/ (float) ((System.currentTimeMillis() - start_timestamp) / 1000);
					kbytes_per_second = formatter.format((per_second_overall / 1024f));
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
					b
							.putString("text", Navit.get_text("downloading") + ": " + map_values.map_name
									+ "\n" + " " + (int) (already_read / 1024f / 1024f) + "Mb / "
									+ (int) (map_values.est_size_bytes / 1024f / 1024f) + "Mb" + "\n" + " "
									+ kbytes_per_second + "kb/s" + " " + Navit.get_text("ETA") + ": "
									+ eta_string); //TRANS
					msg.setData(b);
					handler.sendMessage(msg);
				}
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
			b.putInt("dialog_num", my_dialog_num);
			b.putString("text", Navit.get_text("Error downloading map!")); //TRANS
			msg.setData(b);
			handler.sendMessage(msg);

			Log.d("NavitMapDownloader", "Error: " + e);
			exit_code = 3;
		}
		catch (Exception e)
		{
			msg = handler.obtainMessage();
			b = new Bundle();
			msg.what = 2;
			b.putInt("dialog_num", my_dialog_num);
			b.putString("text", Navit.get_text("Error downloading map!")); //TRANS
			msg.setData(b);
			handler.sendMessage(msg);

			Log.d("NavitMapDownloader", "gerneral Error: " + e);
			exit_code = 4;
		}

		msg = handler.obtainMessage();
		b = new Bundle();
		msg.what = 1;
		b.putInt("max", (int) (map_values.est_size_bytes / 1024));
		b.putInt("cur", (int) (map_values.est_size_bytes / 1024));
		b.putInt("dialog_num", my_dialog_num);
		b.putString("title", Navit.get_text("Mapdownload")); //TRANS
		b.putString("text", map_values.map_name + " "+Navit.get_text("ready")); //TRANS
		msg.setData(b);
		handler.sendMessage(msg);


		Log.d("NavitMapDownloader", "success");
		exit_code = 0;
		return exit_code;
	}
}
