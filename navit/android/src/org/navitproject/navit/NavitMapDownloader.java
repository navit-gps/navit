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
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.net.HttpURLConnection;
import java.net.URL;

import android.os.Handler;
import android.os.StatFs;
import android.util.Log;

public class NavitMapDownloader extends Thread
{
	private static class osm_map_values
	{
		String  lon1;
		String  lat1;
		String  lon2;
		String  lat2;
		String  map_name                     = "";
		long    est_size_bytes               = 0;
		int     level                        = 0;


		public osm_map_values(String mapname, String lon_1, String lat_1, String lon_2, String lat_2,
				long bytes_est, int level)
		{
			this.map_name = mapname;
			this.lon1 = lon_1;
			this.lat1 = lat_1;
			this.lon2 = lon_2;
			this.lat2 = lat_2;
			this.est_size_bytes = bytes_est;
			this.level = level;
		}
	}
	//
	// define the maps here
	//
	private static final osm_map_values[] osm_maps = {
		new osm_map_values("Whole Planet", "-180", "-90", "180", "90", 5985878379L, 0),
		new osm_map_values("Africa", "-20.8", "-35.2", "52.5", "37.4", 180836389L, 0),
		new osm_map_values("Angola", "11.4", "-18.1", "24.2", "-5.3", 56041641L, 1),
		new osm_map_values("Burundi", "28.9", "-4.5", "30.9", "-2.2", 56512924L, 1),
		new osm_map_values("Democratic Republic of the Congo", "11.7", "-13.6", "31.5", "5.7",65026791L, 1),
		new osm_map_values("Kenya", "33.8","-5.2", "42.4", "4.9", 58545273L, 1),
		new osm_map_values("Lesotho", "26.9", "-30.7", "29.6","-28.4", 54791041L, 1),
		new osm_map_values("Madagascar", "43.0","-25.8", "50.8","-11.8", 56801099L, 1),
		new osm_map_values("Nambia+Botswana","11.4", "-29.1","29.5", "-16.9", 61807049L, 1),
		new osm_map_values("Reunion", "55.2","-21.4", "55.9","-20.9", 58537419L, 1),
		new osm_map_values("Rwanda", "28.8","-2.9", "30.9","-1.0", 56313710L, 1),
		new osm_map_values("South Africa","15.6", "-35.2","33.3", "-21.9", 73545245L, 1),
		new osm_map_values("Uganda", "29.3","-1.6", "35.1","4.3", 57376589L, 1),
		new osm_map_values("Asia", "23.8","0.1", "195.0","82.4", 797725952L, 0),
		new osm_map_values("China", "67.3","5.3", "135.0","54.5", 259945160L, 1),
		new osm_map_values("Cyprus", "32.0","34.5", "34.9","35.8", 58585278L, 1),
		new osm_map_values("India+Nepal","67.9", "5.5","89.6", "36.0", 82819344L, 1),
		new osm_map_values("Indonesia", "93.7","-17.3", "155.5","7.6", 74648081L, 1),
		new osm_map_values("Iran", "43.5","24.4", "63.6","40.4", 69561312L, 1),
		new osm_map_values("Iraq", "38.7","28.5", "49.2","37.4", 59146383L, 1),
		new osm_map_values("Israel", "33.99","29.8", "35.95","33.4", 65065351L, 1),
		new osm_map_values("Japan+Korea+Taiwan","117.6", "20.5","151.3", "47.1", 305538751L, 1),
		new osm_map_values("Malasia+Singapore","94.3", "-5.9","108.6", "6.8", 58849792L, 1),
		new osm_map_values("Mongolia", "87.5","41.4", "120.3","52.7", 60871187L, 1),
		new osm_map_values("Thailand", "97.5","5.7", "105.2","19.7", 62422864L, 1),
		new osm_map_values("Turkey", "25.1","35.8", "46.4","42.8", 81758047L, 1),
		new osm_map_values("UAE+Other", "51.5","22.6", "56.7","26.5", 57419510L, 1),
		new osm_map_values("Australia", "110.5","-44.2", "154.9","-9.2", 128502185L, 0),
		new osm_map_values("Australia", "110.5","-44.2", "154.9","-9.2", 128502185L, 1),
		new osm_map_values("Tasmania", "144.0","-45.1", "155.3","-24.8", 103573989L, 1),
		new osm_map_values("Victoria+New South Wales","140.7", "-39.4","153.7", "-26.9", 99307594L, 1),
		new osm_map_values("New Zealand","165.2", "-47.6","179.1", "-33.7", 64757454L, 1),
		new osm_map_values("Europe", "-12.97","33.59", "34.15","72.10", 2753910015L, 0),
		new osm_map_values("Western Europe","-17.6", "34.5","42.9", "70.9", 2832986851L, 1),
		new osm_map_values("Austria", "9.4","46.32", "17.21","49.1", 222359992L, 1),
		new osm_map_values("BeNeLux", "2.08","48.87", "7.78","54.52", 533865194L, 1),
		new osm_map_values("Faroe Islands","-7.8", "61.3","-6.1", "62.5", 54526101L, 1),
		new osm_map_values("France", "-5.45","42.00", "8.44","51.68", 1112047845L, 1),
		new osm_map_values("Germany", "5.18","46.84", "15.47","55.64", 944716238L, 1),
		new osm_map_values("Bavaria", "10.3","47.8", "13.6","49.7", 131799419L, 2),
		new osm_map_values("Saxonia", "11.8","50.1", "15.0","51.7", 112073909L, 2),
		new osm_map_values("Germany+Austria+Switzerland","3.4", "44.5","18.6", "55.1", 1385785353L, 1),
		new osm_map_values("Iceland", "-25.3","62.8", "-11.4","67.5", 57281405L, 1),
		new osm_map_values("Ireland", "-11.17","51.25", "-5.23","55.9", 70186936L, 1),
		new osm_map_values("Italy", "6.52","36.38", "18.96","47.19", 291401314L, 1),
		new osm_map_values("Spain+Portugal","-11.04", "34.87","4.62", "44.41", 292407746L, 1),
		new osm_map_values("Mallorca", "2.2","38.8", "4.7","40.2", 88100718L, 2),
		new osm_map_values("Galicia", "-10.0","41.7", "-6.3","44.1", 64605237L, 2),
		new osm_map_values("Scandinavia", "4.0","54.4", "32.1","71.5", 299021928L, 1),
		new osm_map_values("Finland", "18.6","59.2", "32.3","70.3", 128871467L, 1),
		new osm_map_values("Denmark", "7.49","54.33", "13.05","57.88", 120025875L, 1),
		new osm_map_values("Switzerland","5.79", "45.74","10.59", "47.84", 162616817L, 1),
		new osm_map_values("UK", "-9.7", "49.6","2.2", "61.2", 245161510L, 1),
		new osm_map_values("Bulgaria", "24.7","42.1", "24.8","42.1", 56607427L, 1),
		new osm_map_values("Czech Republic","11.91", "48.48","19.02", "51.17", 234138824L, 1),
		new osm_map_values("Croatia", "13.4","42.1", "19.4","46.9", 99183280L, 1),
		new osm_map_values("Estonia", "21.5","57.5", "28.2","59.6", 79276178L, 1),
		new osm_map_values("Greece", "28.9","37.8", "29.0","37.8", 55486527L, 1),
		new osm_map_values("Crete", "23.3","34.5", "26.8","36.0", 57032630L, 1),
		new osm_map_values("Hungary", "16.08","45.57", "23.03","48.39", 109831319L, 1),
		new osm_map_values("Latvia", "20.7","55.6", "28.3","58.1", 71490706L, 1),
		new osm_map_values("Lithuania", "20.9","53.8", "26.9","56.5", 67992457L, 1),
		new osm_map_values("Poland", "13.6","48.8", "24.5","55.0", 266136768L, 1),
		new osm_map_values("Romania", "20.3","43.5", "29.9","48.4", 134525863L, 1),
		new osm_map_values("North America","-178.1", "6.5","-10.4", "84.0", 2477309662L, 0),
		new osm_map_values("Alaska", "-179.5","49.5", "-129","71.6", 72320027L, 1),
		new osm_map_values("Canada", "-141.3","41.5", "-52.2","70.2", 937813467L, 1),
		new osm_map_values("Hawaii", "-161.07","18.49", "-154.45","22.85", 57311788L, 1),
		new osm_map_values("USA (except Alaska and Hawaii)","-125.4", "24.3","-66.5", "49.3", 2216912004L, 1),
		new osm_map_values("Nevada", "-120.2","35.0", "-113.8","42.1", 136754975L, 2),
		new osm_map_values("Oregon", "-124.8","41.8", "-116.3","46.3", 101627308L, 2),
		new osm_map_values("Washington State","-125.0", "45.5","-116.9", "49.0", 98178877L, 2),
		new osm_map_values("South+Middle America","-83.5", "-56.3","-30.8", "13.7", 159615197L, 0),
		new osm_map_values("Argentina", "-73.9","-57.3", "-51.6","-21.0", 87516152L, 1),
		new osm_map_values("Argentina+Chile","-77.2", "-56.3","-52.7", "-16.1", 91976696L, 1),
		new osm_map_values("Bolivia", "-70.5","-23.1", "-57.3","-9.3", 58242168L, 1),
		new osm_map_values("Brazil", "-71.4","-34.7", "-32.8","5.4", 105527899L, 1),
		new osm_map_values("Cuba", "-85.3","19.6", "-74.0","23.6", 56608942L, 1),
		new osm_map_values("Colombia", "-79.1","-4.0", "-66.7","12.6", 78658454L, 1),
		new osm_map_values("Ecuador", "-82.6","-5.4", "-74.4","2.3", 61501914L, 1),
		new osm_map_values("Guyana+Suriname+Guyane Francaise","-62.0", "1.0","-51.2", "8.9", 57040689L, 1),
		new osm_map_values("Haiti+Republica Dominicana","-74.8", "17.3","-68.2", "20.1", 63528584L, 1),
		new osm_map_values("Jamaica", "-78.6","17.4", "-75.9","18.9", 53958307L, 1),
		new osm_map_values("Mexico", "-117.6","14.1", "-86.4","32.8", 251108617L, 1),
		new osm_map_values("Paraguay", "-63.8","-28.1", "-53.6","-18.8", 57188715L, 1),
		new osm_map_values("Peru", "-82.4","-18.1", "-67.5","0.4", 65421441L, 1),
		new osm_map_values("Uruguay", "-59.2","-36.5", "-51.7","-29.7", 63542225L, 1),
		new osm_map_values("Venezuela", "-73.6","0.4", "-59.7","12.8", 64838882L, 1)
	};

	private static String[]             OSM_MAP_NAME_LIST_inkl_SIZE_ESTIMATE    = null;

	public static int[]                 OSM_MAP_NAME_ORIG_ID_LIST               = null;

	private Boolean                     stop_me                                 = false;
	private static final int            SOCKET_CONNECT_TIMEOUT                  = 25000;			// 25 secs.
	private static final int            SOCKET_READ_TIMEOUT                     = 15000;			// 15 secs.
	private static final int            MAP_WRITE_FILE_BUFFER                   = 1024 * 64;
	private static final int            MAP_WRITE_MEM_BUFFER                    = 1024 * 64;
	private static final int            MAP_READ_FILE_BUFFER                    = 1024 * 64;
	private static final int            UPDATE_PROGRESS_EVERY_CYCLE             = 8;
	private static final int            MAX_RETRIES                             = 5;
	private static final String         MAP_FILENAME_PRI                        = "navitmap.bin";
	private static final String         MAP_FILENAME_NUM                        = "navitmap_%03d.bin";
	private static final String         MAP_FILENAME_PATH                       = Navit.MAP_FILENAME_PATH;
	private static final String 		TAG 									= "NavitMapDownloader";

	private Handler                     mHandler;
	private osm_map_values              map_values;
	private int                         map_slot;
	private int                         dialog_num;
	
	public static final int             EXIT_SUCCESS                            = 0;
	public static final int             EXIT_RECOVERABLE_ERROR                  = 1;
	public static final int             EXIT_UNRECOVERABLE_ERROR                = 2;
	
	public static int                   retry_counter                           = 0;

	public void run()
	{
		stop_me = false;
		int exit_code;
		retry_counter = 0;

		Log.v(TAG, "map_num3=" + this.map_slot);

		NavitMessages.sendDialogMessage( mHandler, NavitMessages.DIALOG_PROGRESS_BAR
				, Navit.get_text("Mapdownload"), Navit.get_text("downloading") + ": " + map_values.map_name
				, Navit.MAPDOWNLOAD_DIALOG, 20 , 0);

		do
		{
			try
			{
				Thread.sleep(10 + retry_counter * 1000);
			} catch (InterruptedException e1)	{}
		} while ( ( exit_code = download_osm_map(mHandler, map_values, map_slot)) == EXIT_RECOVERABLE_ERROR
				&& retry_counter++ < MAX_RETRIES
				&& !stop_me);

		if (exit_code == EXIT_SUCCESS)
		{
			NavitMessages.sendDialogMessage( mHandler, NavitMessages.DIALOG_TOAST
					, null, map_values.map_name + " " + Navit.get_text("ready")
					, dialog_num , 0 , 0);

			Log.d(TAG, "success");
		}
		
		if (exit_code == EXIT_SUCCESS || stop_me )
		{
			NavitMessages.sendDialogMessage( mHandler , NavitMessages.DIALOG_REMOVE_PROGRESS_BAR, null, null, dialog_num 
						, exit_code , 0 );
		}
	}

	public void stop_thread()
	{
		stop_me = true;
		Log.d(TAG, "stop_me -> true");
	}

	public NavitMapDownloader(Navit main, Handler h, int map_id, int dialog_num, int map_slot)
	{
		this.mHandler = h;
		this.map_values = osm_maps[map_id];
		this.map_slot = map_slot;
	}

	public static String[] getMenu()
	{
		// need only init once
		if (OSM_MAP_NAME_LIST_inkl_SIZE_ESTIMATE != null) 
		{ 
			return OSM_MAP_NAME_LIST_inkl_SIZE_ESTIMATE;
		}
		
		String menu_temp[] = new String[osm_maps.length*2];
		OSM_MAP_NAME_ORIG_ID_LIST = new int[osm_maps.length*2];
		int counter = 0;
		int previous_level = -1;
		for (int i = 0; i < osm_maps.length; i++)
		{
			switch (osm_maps[i].level)
			{
			case 0: 
				if (previous_level > 0)
				{
					OSM_MAP_NAME_ORIG_ID_LIST[counter] = -1;
					menu_temp[counter++] = "======";
				}
				menu_temp[counter] = "";
				break;
			case 1:
				menu_temp[counter] = new String(" * ");
				break;
			default:
				menu_temp[counter] = new String(" ** ");
			}
			
			menu_temp[counter] = menu_temp[counter].concat(osm_maps[i].map_name + " " + (osm_maps[i].est_size_bytes / 1024 / 1024) + "MB");
			counter++;
			OSM_MAP_NAME_ORIG_ID_LIST[counter-1] = i;
			
			previous_level = osm_maps[i].level;
		}
		
		OSM_MAP_NAME_LIST_inkl_SIZE_ESTIMATE = new String[counter];
		for (int i = 0; i < counter; i++)
		{
			OSM_MAP_NAME_LIST_inkl_SIZE_ESTIMATE[i] = menu_temp[i];
		}
		return OSM_MAP_NAME_LIST_inkl_SIZE_ESTIMATE;
	}

	public int download_osm_map(Handler handler, osm_map_values map_values, int map_number)
	{
		int exit_code = EXIT_SUCCESS;
		boolean resume = false;
		HttpURLConnection c = null;
		BufferedOutputStream buf = null;
		BufferedInputStream bif = null;
		File outputFile = null;
		long already_read = 0;
		long real_size_bytes = 0;

		String fileName = map_values.map_name + ".tmp";

		try
		{
			long file_size_expected = -1;
			long file_time_expected = -1;
			outputFile = new File(MAP_FILENAME_PATH, fileName);

			long old_download_size = outputFile.length();
			long start_timestamp = System.nanoTime();

			outputFile.mkdir();
			if (old_download_size > 0)
			{
				ObjectInputStream infoStream = new ObjectInputStream(new FileInputStream(MAP_FILENAME_PATH + fileName + ".info"));
				file_size_expected = infoStream.readLong();
				file_time_expected = infoStream.readLong();
				infoStream.close();
			}
			URL url = new URL("http://maps.navit-project.org/api/map/?bbox=" + map_values.lon1 + ","
					+ map_values.lat1 + "," + map_values.lon2 + "," + map_values.lat2);

//			URL url = new URL("http://192.168.2.101:8080/zweibruecken.bin");
			c = (HttpURLConnection) url.openConnection();
			c.setRequestMethod("GET");
			c.setDoOutput(true);
			c.setReadTimeout(SOCKET_READ_TIMEOUT);
			c.setConnectTimeout(SOCKET_CONNECT_TIMEOUT);

			if ( file_size_expected > 0 || file_time_expected > 0 )
			{
				// looks like the same file, try to resume
				Log.v(TAG, "Try to resume download");
				resume = true;
				c.setRequestProperty("Range", "bytes=" + old_download_size + "-");
				already_read = old_download_size;
			}

			real_size_bytes = c.getContentLength();
			long fileTime = c.getLastModified();
			Log.d(TAG, "size: " + real_size_bytes 
					+ ", expected: " + file_size_expected
					+ ", read: " + already_read
					+ ", timestamp: " + fileTime
					+ ", timestamp_old: " + file_time_expected);
			
			if (resume && (fileTime != file_time_expected
					// some server return the remaining size to dl, other the file size
					|| ((real_size_bytes + already_read != file_size_expected) && real_size_bytes != file_size_expected)
				    || (real_size_bytes == -1 && fileTime == -1)))
			{
				Log.w(TAG, "Downloaded content to old. Resume not possible");
				outputFile.delete();
				return EXIT_RECOVERABLE_ERROR;
			}

			if (!resume)
			{
				outputFile.delete();
				old_download_size = 0;
				File infoFile = new File(MAP_FILENAME_PATH, fileName + ".info");
				ObjectOutputStream infoStream = new ObjectOutputStream(new FileOutputStream(infoFile));
				infoStream.writeLong(real_size_bytes);
				infoStream.writeLong(fileTime);
				infoStream.close();
			}
			else
			{
				real_size_bytes = file_size_expected;
			}
			
			if ( real_size_bytes <= 0)
				real_size_bytes = map_values.est_size_bytes;
			
			if (!checkFreeSpace(real_size_bytes - already_read))
			{
				return EXIT_UNRECOVERABLE_ERROR;
			}
			
			Log.d(TAG, "real size in bytes: " + real_size_bytes);

			buf = new BufferedOutputStream(new FileOutputStream(outputFile, resume) , MAP_WRITE_FILE_BUFFER);
			bif = new BufferedInputStream(c.getInputStream(), MAP_READ_FILE_BUFFER);

			byte[] buffer = new byte[MAP_WRITE_MEM_BUFFER];
			int len1 = 0;
			int alt_cur = 0;
			String eta_string = "";
			String info;
			float per_second_overall;
			long bytes_remaining = 0;
			int eta_seconds = 0;
			while (!stop_me && (len1 = bif.read(buffer)) != -1)
			{
				already_read += len1;
				if (alt_cur++ % UPDATE_PROGRESS_EVERY_CYCLE == 0)
				{

					if (already_read > real_size_bytes)
					{
						real_size_bytes = already_read;
					}
					
					bytes_remaining = real_size_bytes - already_read;
					per_second_overall = (already_read - old_download_size) / ((System.nanoTime() - start_timestamp) / 1000000000f);
					eta_seconds = (int) (bytes_remaining / per_second_overall);
					if (eta_seconds > 60)
					{
						eta_string = (int) (eta_seconds / 60f) + " m";
					}
					else
					{
						eta_string = eta_seconds + " s";
					}
					info = String.format("%s: %s\n %dMb / %dMb\n %.1f kb/s %s: %s"
							, Navit.get_text("downloading")
							, map_values.map_name
							, already_read / 1024 / 1024
							, real_size_bytes / 1024 / 1024
							, per_second_overall / 1024f
							, Navit.get_text("ETA")
							, eta_string);
					
					if (retry_counter > 0)
					{
						info += "\n Retry " + retry_counter + "/" + MAX_RETRIES;
					}
					Log.e(TAG, "info: " + info);

					NavitMessages.sendDialogMessage( handler, NavitMessages.DIALOG_PROGRESS_BAR
							, Navit.get_text("Mapdownload"), info
							, dialog_num, (int) (real_size_bytes / 1024), (int) (already_read / 1024));
				}
				buf.write(buffer, 0, len1);
			}

			Log.d(TAG, "Connectionerror: " + c.getResponseCode ());

			if (stop_me)
			{
				NavitMessages.sendDialogMessage( handler, NavitMessages.DIALOG_TOAST
								, null, Navit.get_text("Map download aborted!")
								, dialog_num , 0 , 0);
						
				exit_code = EXIT_UNRECOVERABLE_ERROR;
			}
			else if ( already_read < real_size_bytes )
			{
				Log.d(TAG, "Server send only " + already_read + " bytes of " + real_size_bytes);
				exit_code = EXIT_RECOVERABLE_ERROR;
			}
			else
			{
				exit_code = EXIT_SUCCESS;
			}
		}
		catch (IOException e)
		{
			Log.d(TAG, "Error: " + e);
			
			if ( !checkFreeSpace(real_size_bytes - already_read))
			{
				exit_code = EXIT_UNRECOVERABLE_ERROR;
			}
			else
			{
				NavitMessages.sendDialogMessage( mHandler, NavitMessages.DIALOG_PROGRESS_BAR
						, Navit.get_text("Mapdownload")
						, Navit.get_text("Error downloading map!")
						, dialog_num, (int) (real_size_bytes / 1024), (int) (already_read / 1024));
				exit_code = EXIT_RECOVERABLE_ERROR;
			}
		}
		catch (Exception e)
		{
			NavitMessages.sendDialogMessage( mHandler, NavitMessages.DIALOG_PROGRESS_BAR
					, Navit.get_text("Mapdownload")
					, Navit.get_text("Error downloading map!")
					, dialog_num, (int) (real_size_bytes / 1024), (int) (already_read / 1024));
			Log.d(TAG, "gerneral Error: " + e);
			exit_code = EXIT_RECOVERABLE_ERROR;
		}

		// always cleanup, as we might get errors when trying to resume
		if (buf!=null && bif!=null)
		{
			try {
				buf.flush();
				buf.close();
	
				bif.close();
			} catch (IOException e) { }
		}

		if (exit_code == EXIT_SUCCESS)
		{
			String final_fileName = MAP_FILENAME_PRI;
			
			if (map_number>0)
			{
				final_fileName = String.format(MAP_FILENAME_NUM, map_number);
			}

			File final_outputFile = new File(MAP_FILENAME_PATH, final_fileName);
			// delete an already final filename, first
			final_outputFile.delete();
			// rename file to final name
			outputFile.renameTo(final_outputFile);
		}
		
		return exit_code;
	}
	
	private boolean checkFreeSpace(long needed_bytes)
	{
		StatFs fsInfo = new StatFs(MAP_FILENAME_PATH);
		
		long free_space = fsInfo.getAvailableBlocks() * fsInfo.getBlockSize();
		
		if ( needed_bytes <= 0 )
			needed_bytes = MAP_WRITE_FILE_BUFFER;

		if (free_space < needed_bytes )
		{
			Log.e(TAG, "Not enough free space. Please free at least " + needed_bytes / 1024 /1024 + "Mb.");
			
			NavitMessages.sendDialogMessage( mHandler, NavitMessages.DIALOG_PROGRESS_BAR
					, Navit.get_text("Mapdownload")
					, Navit.get_text("Error downloading map!") + "\n" + Navit.get_text("Not enough free space")
					, dialog_num, (int)(needed_bytes / 1024), (int)(free_space / 1024));
			
			return false;
		}
		return true;
	}
}
