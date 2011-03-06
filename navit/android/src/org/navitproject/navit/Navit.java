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
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.URLDecoder;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import org.navitproject.navit.NavitMapDownloader.ProgressThread;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Resources;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.text.SpannableString;
import android.text.method.LinkMovementMethod;
import android.text.util.Linkify;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.Menu;
import android.view.MenuItem;
import android.view.inputmethod.InputMethodManager;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;


public class Navit extends Activity implements Handler.Callback
{
	public static final class Navit_Address_Result_Struct
	{
		String	result_type;	// TWN,STR,SHN
		String		item_id; // H<ddddd>L<ddddd> -> item.id_hi item.id_lo
		float		lat;
		float		lon;
		String	addr;
	}

	public Handler												handler;
	private PowerManager.WakeLock							wl;
	private NavitActivityResult							ActivityResults[];
	public static InputMethodManager						mgr											= null;
	public static DisplayMetrics							metrics										= null;
	public static Boolean									show_soft_keyboard						= false;
	public static Boolean									show_soft_keyboard_now_showing		= false;
	public static long										last_pressed_menu_key					= 0L;
	public static long										time_pressed_menu_key					= 0L;
	private static Intent									startup_intent								= null;
	private static long										startup_intent_timestamp				= 0L;
	public static String										my_display_density						= "mdpi";
	private boolean											searchBoxShown								= false;
	public static final int									MAPDOWNLOAD_PRI_DIALOG					= 1;
	public static final int									MAPDOWNLOAD_SEC_DIALOG					= 2;
	public static final int									SEARCHRESULTS_WAIT_DIALOG				= 3;
	public static final int									ADDRESS_RESULTS_DIALOG_MAX				= 10;
	public ProgressDialog									mapdownloader_dialog_pri				= null;
	public ProgressDialog									mapdownloader_dialog_sec				= null;
	public ProgressDialog									search_results_wait						= null;
	public static Handler									Navit_progress_h							= null;
	public static NavitMapDownloader						mapdownloader_pri							= null;
	public static NavitMapDownloader						mapdownloader_sec							= null;
	public static final int									NavitDownloaderPriSelectMap_id		= 967;
	public static final int									NavitDownloaderSecSelectMap_id		= 968;
	public static int											download_map_id							= 0;
	ProgressThread												progressThread_pri						= null;
	ProgressThread												progressThread_sec						= null;
	public static int											search_results_towns						= 0;
	public static int											search_results_streets					= 0;
	public static int											search_results_streets_hn				= 0;
	SearchResultsThread										searchresultsThread						= null;
	SearchResultsThreadSpinnerThread						spinner_thread								= null;
	public static Boolean									NavitAddressSearchSpinnerActive		= false;
	public static final int									MAP_NUM_PRIMARY							= 11;
	public static final int									NavitAddressSearch_id					= 70;
	public static final int									NavitAddressResultList_id				= 71;
	public static List<Navit_Address_Result_Struct>	NavitAddressResultList_foundItems	= new ArrayList<Navit_Address_Result_Struct>();

	public static String										Navit_last_address_search_string		= "";
	public static Boolean									Navit_last_address_partial_match		= false;

	public static final int									MAP_NUM_SECONDARY							= 12;
	static final String										MAP_FILENAME_PATH							= "/sdcard/navit/";
	static final String										NAVIT_DATA_DIR								= "/data/data/org.navitproject.navit";
	static final String										NAVIT_DATA_SHARE_DIR						= NAVIT_DATA_DIR
																															+ "/share";
	static final String										FIRST_STARTUP_FILE						= NAVIT_DATA_SHARE_DIR
																															+ "/has_run_once.txt";

	public static String get_text(String in)
	{
		return NavitTextTranslations.get_text(in);
	}

	private boolean extractRes(String resname, String result)
	{
		int slash = -1;
		boolean needs_update = false;
		File resultfile;
		Resources res = getResources();
		Log.e("Navit", "Res Name " + resname);
		Log.e("Navit", "result " + result);
		int id = res.getIdentifier(resname, "raw", "org.navitproject.navit");
		Log.e("Navit", "Res ID " + id);
		if (id == 0) return false;
		while ((slash = result.indexOf("/", slash + 1)) != -1)
		{
			if (slash != 0)
			{
				Log.e("Navit", "Checking " + result.substring(0, slash));
				resultfile = new File(result.substring(0, slash));
				if (!resultfile.exists())
				{
					Log.e("Navit", "Creating dir");
					if (!resultfile.mkdir()) return false;
					needs_update = true;
				}
			}
		}
		resultfile = new File(result);
		if (!resultfile.exists()) needs_update = true;
		if (!needs_update)
		{
			try
			{
				InputStream resourcestream = res.openRawResource(id);
				FileInputStream resultfilestream = new FileInputStream(resultfile);
				byte[] resourcebuf = new byte[1024];
				byte[] resultbuf = new byte[1024];
				int i = 0;
				while ((i = resourcestream.read(resourcebuf)) != -1)
				{
					if (resultfilestream.read(resultbuf) != i)
					{
						Log.e("Navit", "Result is too short");
						needs_update = true;
						break;
					}
					for (int j = 0; j < i; j++)
					{
						if (resourcebuf[j] != resultbuf[j])
						{
							Log.e("Navit", "Result is different");
							needs_update = true;
							break;
						}
					}
					if (needs_update) break;
				}
				if (!needs_update && resultfilestream.read(resultbuf) != -1)
				{
					Log.e("Navit", "Result is too long");
					needs_update = true;
				}

			}
			catch (Exception e)
			{
				Log.e("Navit", "Exception " + e.getMessage());
				return false;
			}
		}
		if (needs_update)
		{
			Log.e("Navit", "Extracting resource");

			try
			{
				InputStream resourcestream = res.openRawResource(id);
				FileOutputStream resultfilestream = new FileOutputStream(resultfile);
				byte[] buf = new byte[1024];
				int i = 0;
				while ((i = resourcestream.read(buf)) != -1)
				{
					resultfilestream.write(buf, 0, i);
				}
			}
			catch (Exception e)
			{
				Log.e("Navit", "Exception " + e.getMessage());
				return false;
			}
		}
		return true;
	}

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		// only take arguments here, onResume gets called all the time (e.g. when screenblanks, etc.)
		Navit.startup_intent = this.getIntent();
		// hack! remeber timstamp, and only allow 4 secs. later in onResume to set target!
		Navit.startup_intent_timestamp = System.currentTimeMillis();
		Log.e("Navit", "**1**A " + startup_intent.getAction());
		Log.e("Navit", "**1**D " + startup_intent.getDataString());

		// init translated text
		NavitTextTranslations.init();

		// get the local language -------------
		Locale locale = java.util.Locale.getDefault();
		String lang = locale.getLanguage();
		String langu = lang;
		String langc = lang;
		Log.e("Navit", "lang=" + lang);
		int pos = langu.indexOf('_');
		if (pos != -1)
		{
			langc = langu.substring(0, pos);
			langu = langc + langu.substring(pos).toUpperCase(locale);
			Log.e("Navit", "substring lang " + langu.substring(pos).toUpperCase(locale));
			// set lang. for translation
			NavitTextTranslations.main_language = langc;
			NavitTextTranslations.sub_language = langu.substring(pos).toUpperCase(locale);
		}
		else
		{
			String country = locale.getCountry();
			Log.e("Navit", "Country1 " + country);
			Log.e("Navit", "Country2 " + country.toUpperCase(locale));
			langu = langc + "_" + country.toUpperCase(locale);
			// set lang. for translation
			NavitTextTranslations.main_language = langc;
			NavitTextTranslations.sub_language = country.toUpperCase(locale);
		}
		Log.e("Navit", "Language " + lang);
		// get the local language -------------


		// make sure the new path for the navitmap.bin file(s) exist!!
		File navit_maps_dir = new File(MAP_FILENAME_PATH);
		navit_maps_dir.mkdirs();

		// make sure the share dir exists, otherwise the infobox will not show
		File navit_data_share_dir = new File(NAVIT_DATA_SHARE_DIR);
		navit_data_share_dir.mkdirs();


		// hardcoded strings for now, use routine down below later!!
		// hardcoded strings for now, use routine down below later!!
		// hardcoded strings for now, use routine down below later!!
		if (lang.compareTo("de") == 0)
		{
			NavitTextTranslations.NAVIT_JAVA_MENU_download_first_map = NavitTextTranslations.NAVIT_JAVA_MENU_download_first_map_de;
			NavitTextTranslations.NAVIT_JAVA_MENU_download_second_map = NavitTextTranslations.NAVIT_JAVA_MENU_download_second_map_de;
			NavitTextTranslations.INFO_BOX_TITLE = NavitTextTranslations.INFO_BOX_TITLE_de;
			NavitTextTranslations.INFO_BOX_TEXT = NavitTextTranslations.INFO_BOX_TEXT_de;
			NavitTextTranslations.NAVIT_JAVA_MENU_MOREINFO = NavitTextTranslations.NAVIT_JAVA_MENU_MOREINFO_de;
			NavitTextTranslations.NAVIT_JAVA_MENU_ZOOMIN = NavitTextTranslations.NAVIT_JAVA_MENU_ZOOMIN_de;
			NavitTextTranslations.NAVIT_JAVA_MENU_ZOOMOUT = NavitTextTranslations.NAVIT_JAVA_MENU_ZOOMOUT_de;
			NavitTextTranslations.NAVIT_JAVA_MENU_EXIT = NavitTextTranslations.NAVIT_JAVA_MENU_EXIT_de;
			NavitTextTranslations.NAVIT_JAVA_MENU_TOGGLE_POI = NavitTextTranslations.NAVIT_JAVA_MENU_TOGGLE_POI_de;
			NavitTextTranslations.NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE = NavitTextTranslations.NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE_de;
		}
		else if (lang.compareTo("fr") == 0)
		{
			NavitTextTranslations.NAVIT_JAVA_MENU_download_first_map = NavitTextTranslations.NAVIT_JAVA_MENU_download_first_map_fr;
			NavitTextTranslations.NAVIT_JAVA_MENU_download_second_map = NavitTextTranslations.NAVIT_JAVA_MENU_download_second_map_fr;
			NavitTextTranslations.INFO_BOX_TITLE = NavitTextTranslations.INFO_BOX_TITLE_fr;
			NavitTextTranslations.INFO_BOX_TEXT = NavitTextTranslations.INFO_BOX_TEXT_fr;
			NavitTextTranslations.NAVIT_JAVA_MENU_MOREINFO = NavitTextTranslations.NAVIT_JAVA_MENU_MOREINFO_fr;
			NavitTextTranslations.NAVIT_JAVA_MENU_ZOOMIN = NavitTextTranslations.NAVIT_JAVA_MENU_ZOOMIN_fr;
			NavitTextTranslations.NAVIT_JAVA_MENU_ZOOMOUT = NavitTextTranslations.NAVIT_JAVA_MENU_ZOOMOUT_fr;
			NavitTextTranslations.NAVIT_JAVA_MENU_EXIT = NavitTextTranslations.NAVIT_JAVA_MENU_EXIT_fr;
			NavitTextTranslations.NAVIT_JAVA_MENU_TOGGLE_POI = NavitTextTranslations.NAVIT_JAVA_MENU_TOGGLE_POI_fr;
			NavitTextTranslations.NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE = NavitTextTranslations.NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE_fr;
		}
		else if (lang.compareTo("nl") == 0)
		{
			NavitTextTranslations.NAVIT_JAVA_MENU_download_first_map = NavitTextTranslations.NAVIT_JAVA_MENU_download_first_map_nl;
			NavitTextTranslations.NAVIT_JAVA_MENU_download_second_map = NavitTextTranslations.NAVIT_JAVA_MENU_download_second_map_nl;
			NavitTextTranslations.INFO_BOX_TITLE = NavitTextTranslations.INFO_BOX_TITLE_nl;
			NavitTextTranslations.INFO_BOX_TEXT = NavitTextTranslations.INFO_BOX_TEXT_nl;
			NavitTextTranslations.NAVIT_JAVA_MENU_MOREINFO = NavitTextTranslations.NAVIT_JAVA_MENU_MOREINFO_nl;
			NavitTextTranslations.NAVIT_JAVA_MENU_ZOOMIN = NavitTextTranslations.NAVIT_JAVA_MENU_ZOOMIN_nl;
			NavitTextTranslations.NAVIT_JAVA_MENU_ZOOMOUT = NavitTextTranslations.NAVIT_JAVA_MENU_ZOOMOUT_nl;
			NavitTextTranslations.NAVIT_JAVA_MENU_EXIT = NavitTextTranslations.NAVIT_JAVA_MENU_EXIT_nl;
			NavitTextTranslations.NAVIT_JAVA_MENU_TOGGLE_POI = NavitTextTranslations.NAVIT_JAVA_MENU_TOGGLE_POI_nl;
			NavitTextTranslations.NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE = NavitTextTranslations.NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE_nl;
		}
		// hardcoded strings for now, use routine down below later!!
		// hardcoded strings for now, use routine down below later!!


		/*
		 * show info box for first time users
		 */
		AlertDialog.Builder infobox = new AlertDialog.Builder(this);
		infobox.setTitle(NavitTextTranslations.INFO_BOX_TITLE); //TRANS
		infobox.setCancelable(false);
		final TextView message = new TextView(this);
		message.setFadingEdgeLength(20);
		message.setVerticalFadingEdgeEnabled(true);
		//message.setVerticalScrollBarEnabled(true);
		RelativeLayout.LayoutParams rlp = new RelativeLayout.LayoutParams(
				RelativeLayout.LayoutParams.FILL_PARENT, RelativeLayout.LayoutParams.FILL_PARENT);

		// margins seem not to work, hmm strange
		// so add a " " at start of every line. well whadda you gonna do ...
		//rlp.leftMargin = 8; -> we use "m" string

		message.setLayoutParams(rlp);
		final SpannableString s = new SpannableString(NavitTextTranslations.INFO_BOX_TEXT); //TRANS
		Linkify.addLinks(s, Linkify.WEB_URLS);
		message.setText(s);
		message.setMovementMethod(LinkMovementMethod.getInstance());
		infobox.setView(message);

		//TRANS
		infobox.setPositiveButton(Navit.get_text("Ok"), new DialogInterface.OnClickListener()
		{
			public void onClick(DialogInterface arg0, int arg1)
			{
				Log.e("Navit", "Ok, user saw the infobox");
			}
		});

		//TRANS
		infobox.setNeutralButton(NavitTextTranslations.NAVIT_JAVA_MENU_MOREINFO,
				new DialogInterface.OnClickListener()
				{
					public void onClick(DialogInterface arg0, int arg1)
					{
						Log.e("Navit", "user wants more info, show the website");
						String url = "http://wiki.navit-project.org/index.php/Navit_on_Android";
						Intent i = new Intent(Intent.ACTION_VIEW);
						i.setData(Uri.parse(url));
						startActivity(i);
					}
				});

		File navit_first_startup = new File(FIRST_STARTUP_FILE);
		// if file does NOT exist, show the info box
		if (!navit_first_startup.exists())
		{
			FileOutputStream fos_temp;
			try
			{
				fos_temp = new FileOutputStream(navit_first_startup);
				fos_temp.write((int) 65); // just write an "A" to the file, but really doesnt matter
				fos_temp.flush();
				fos_temp.close();
				infobox.show();
			}
			catch (Exception e)
			{
				e.printStackTrace();
			}
		}
		/*
		 * show info box for first time users
		 */

		// make handler statically available for use in "msg_to_msg_handler"
		Navit_progress_h = this.progress_handler;

		Display display_ = getWindowManager().getDefaultDisplay();
		int width_ = display_.getWidth();
		int height_ = display_.getHeight();
		metrics = new DisplayMetrics();
		display_.getMetrics(Navit.metrics);
		Log.e("Navit", "Navit -> pixels x=" + width_ + " pixels y=" + height_);
		Log.e("Navit", "Navit -> dpi=" + Navit.metrics.densityDpi);
		Log.e("Navit", "Navit -> density=" + Navit.metrics.density);
		Log.e("Navit", "Navit -> scaledDensity=" + Navit.metrics.scaledDensity);

		ActivityResults = new NavitActivityResult[16];
		setVolumeControlStream(AudioManager.STREAM_MUSIC);
		PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
		wl = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK | PowerManager.ON_AFTER_RELEASE,
				"NavitDoNotDimScreen");

		if (!extractRes(langc, NAVIT_DATA_DIR + "/locale/" + langc + "/LC_MESSAGES/navit.mo"))
		{
			Log.e("Navit", "Failed to extract language resource " + langc);
		}

		my_display_density = "mdpi";
		// hdpi display
		if (Navit.metrics.densityDpi == 240)
		{
			my_display_density = "hdpi";
			if (!extractRes("navithdpi", NAVIT_DATA_DIR + "/share/navit.xml"))
			{
				Log.e("Navit", "Failed to extract navit.xml for hdpi device(s)");
			}
		}
		// mdpi display
		else if (Navit.metrics.densityDpi == 160)
		{
			my_display_density = "mdpi";
			if (!extractRes("navitmdpi", NAVIT_DATA_DIR + "/share/navit.xml"))
			{
				Log.e("Navit", "Failed to extract navit.xml for mdpi device(s)");
			}
		}
		// ldpi display
		else if (Navit.metrics.densityDpi == 120)
		{
			my_display_density = "ldpi";
			if (!extractRes("navitldpi", NAVIT_DATA_DIR + "/share/navit.xml"))
			{
				Log.e("Navit", "Failed to extract navit.xml for ldpi device(s)");
			}
		}
		// xhdpi display
		else if (Navit.metrics.densityDpi == 320)
		{
			Log.e("Navit", "found xhdpi device, this is not fully supported!!");
			Log.e("Navit", "using hdpi values");
			my_display_density = "hdpi";
			if (!extractRes("navithdpi", NAVIT_DATA_DIR + "/share/navit.xml"))
			{
				Log.e("Navit", "Failed to extract navit.xml for xhdpi device(s)");
			}
		}
		else
		{
			/* default, meaning we just dont know what display this is */
			if (!extractRes("navit", NAVIT_DATA_DIR + "/share/navit.xml"))
			{
				Log.e("Navit", "Failed to extract navit.xml (default version)");
			}
		}
		// Debug.startMethodTracing("calc");

		// --> dont use!! NavitMain(this, langu, android.os.Build.VERSION.SDK_INT);
		Log.e("Navit", "android.os.Build.VERSION.SDK_INT="
				+ Integer.valueOf(android.os.Build.VERSION.SDK));
		NavitMain(this, langu, Integer.valueOf(android.os.Build.VERSION.SDK), my_display_density);
		// CAUTION: don't use android.os.Build.VERSION.SDK_INT if <uses-sdk android:minSdkVersion="3" />
		// You will get exception on all devices with Android 1.5 and lower
		// because Build.VERSION.SDK_INT is since SDK 4 (Donut 1.6)

		//		Platform Version   API Level
		//		Android 2.2       8
		//		Android 2.1       7
		//		Android 2.0.1     6
		//		Android 2.0       5
		//		Android 1.6       4
		//		Android 1.5       3
		//		Android 1.1       2
		//		Android 1.0       1

		NavitActivity(3);

		Navit.mgr = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);

		// unpack some localized Strings
		// a test now, later we will unpack all needed string for java, here at this point!!
		String x = NavitGraphics.getLocalizedString("Austria");
		Log.e("Navit", "x=" + x);
	}

	@Override
	public void onStart()
	{
		super.onStart();
		Log.e("Navit", "OnStart");
		NavitActivity(2);
	}

	@Override
	public void onRestart()
	{
		super.onRestart();
		Log.e("Navit", "OnRestart");
		NavitActivity(0);
	}

	@Override
	public void onResume()
	{
		super.onResume();
		Log.e("Navit", "OnResume");
		//InputMethodManager mgr = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
		NavitActivity(1);

		String intent_data = null;
		if (startup_intent != null)
		{
			if (System.currentTimeMillis() <= Navit.startup_intent_timestamp + 4000L)
			{
				Log.e("Navit", "**2**A " + startup_intent.getAction());
				Log.e("Navit", "**2**D " + startup_intent.getDataString());
				intent_data = startup_intent.getDataString();
			}
			else
			{
				Log.e("Navit", "timestamp for navigate_to expired! not using data");
			}
		}

		if ((intent_data != null) && (intent_data.substring(0, 18).equals("google.navigation:")))
		{
			// better use regex later, but for now to test this feature its ok :-)
			// better use regex later, but for now to test this feature its ok :-)

			// d: google.navigation:q=blabla-strasse # (this happens when you are offline, or from contacts)
			// a: google.navigation:ll=48.25676,16.643&q=blabla-strasse
			// c: google.navigation:ll=48.25676,16.643
			// b: google.navigation:q=48.25676,16.643

			String lat;
			String lon;
			String q;

			String temp1 = null;
			String temp2 = null;
			String temp3 = null;
			boolean parsable = false;
			boolean unparsable_info_box = true;

			// DEBUG
			// DEBUG
			// DEBUG
			// intent_data = "google.navigation:q=Wien Burggasse 27";
			// intent_data = "google.navigation:q=48.25676,16.643";
			// intent_data = "google.navigation:ll=48.25676,16.643&q=blabla-strasse";
			// intent_data = "google.navigation:ll=48.25676,16.643";
			// DEBUG
			// DEBUG
			// DEBUG

			Log.e("Navit", "found DEBUG 1: " + intent_data.substring(0, 20));
			Log.e("Navit", "found DEBUG 2: " + intent_data.substring(20, 22));
			Log.e("Navit", "found DEBUG 3: " + intent_data.substring(20, 21));

			// if d: then start target search
			if ((intent_data.substring(0, 20).equals("google.navigation:q="))
					&& ((!intent_data.substring(20, 21).equals('+'))
							&& (!intent_data.substring(20, 21).equals('-')) && (!intent_data.substring(20,
							22).matches("[0-9][0-9]"))))
			{
				Log.e("Navit", "target found (d): " + intent_data.split("q=", -1)[1]);
				start_targetsearch_from_intent(intent_data.split("q=", -1)[1]);
				// dont use this here, already starting search, so set to "false"
				parsable = false;
				unparsable_info_box = false;
			}
			// if b: then remodel the input string to look like a:
			else if (intent_data.substring(0, 20).equals("google.navigation:q="))
			{
				intent_data = "ll=" + intent_data.split("q=", -1)[1] + "&q=Target";
				Log.e("Navit", "target found (b): " + intent_data);
				parsable = true;
			}
			// if c: then remodel the input string to look like a:
			else if ((intent_data.substring(0, 21).equals("google.navigation:ll="))
					&& (intent_data.split("&q=").length < 2))
			{
				intent_data = intent_data + "&q=Target";
				Log.e("Navit", "target found (c): " + intent_data);
				parsable = true;
			}
			// already looks like a: just set flag
			else if ((intent_data.substring(0, 21).equals("google.navigation:ll="))
					&& (intent_data.split("&q=").length > 1))
			{
				// dummy, just set the flag
				Log.e("Navit", "target found (a): " + intent_data);
				Log.e("Navit", "target found (a): " + intent_data.split("&q=").length);
				parsable = true;
			}


			if (parsable)
			{
				// now string should be in form --> a:
				// now split the parts off
				temp1 = intent_data.split("&q=", -1)[0];
				try
				{
					temp3 = temp1.split("ll=", -1)[1];
					temp2 = intent_data.split("&q=", -1)[1];
				}
				catch (Exception e)
				{
					// java.lang.ArrayIndexOutOfBoundsException most likely
					// so let's assume we dont have '&q=xxxx'
					temp3 = temp1;
				}

				if (temp2 == null)
				{
					// use some default name
					temp2 = "Target";
				}

				lat = temp3.split(",", -1)[0];
				lon = temp3.split(",", -1)[1];
				q = temp2;

				Message msg = new Message();
				Bundle b = new Bundle();
				b.putInt("Callback", 3);
				b.putString("lat", lat);
				b.putString("lon", lon);
				b.putString("q", q);
				msg.setData(b);
				N_NavitGraphics.callback_handler.sendMessage(msg);
			}
			else
			{
				if (unparsable_info_box && !searchBoxShown)
				{

					searchBoxShown = true;
					String searchString = intent_data.split("q=")[1];
					searchString = searchString.split("&")[0];
					searchString = URLDecoder.decode(searchString); // decode the URL: e.g. %20 -> space
					Log.e("Navit", "Search String :" + searchString);
					executeSearch(searchString);
				}
			}
		}
	}
	@Override
	public void onPause()
	{
		super.onPause();
		Log.e("Navit", "OnPause");
		NavitActivity(-1);
	}

	@Override
	public void onStop()
	{
		super.onStop();
		Log.e("Navit", "OnStop");
		NavitActivity(-2);
	}

	@Override
	public void onDestroy()
	{
		super.onDestroy();
		Log.e("Navit", "OnDestroy");
		NavitActivity(-3);
	}


	public void setActivityResult(int requestCode, NavitActivityResult ActivityResult)
	{
		//Log.e("Navit", "setActivityResult " + requestCode);
		ActivityResults[requestCode] = ActivityResult;
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu)
	{
		super.onCreateOptionsMenu(menu);
		//Log.e("Navit","onCreateOptionsMenu");
		return true;
	}

	@Override
	public boolean onPrepareOptionsMenu(Menu menu)
	{
		super.onPrepareOptionsMenu(menu);
		//Log.e("Navit","onPrepareOptionsMenu");
		// this gets called every time the menu is opened!!
		// change menu items here!
		menu.clear();

		// group-id,item-id,sort order number
		menu.add(1, 1, 100, get_text("zoom in")); //TRANS
		menu.add(1, 2, 200, get_text("zoom out")); //TRANS

		menu.add(1, 3, 300, NavitTextTranslations.NAVIT_JAVA_MENU_download_first_map); //TRANS
		menu.add(1, 5, 400, NavitTextTranslations.NAVIT_JAVA_MENU_TOGGLE_POI); //TRANS

		menu.add(1, 6, 500, get_text("address search")); //TRANS

		menu.add(1, 4, 600, NavitTextTranslations.NAVIT_JAVA_MENU_download_second_map); //TRANS
		menu.add(1, 88, 800, "--");
		menu.add(1, 99, 900, get_text("exit navit")); //TRANS
		return true;
	}

	// define callback id here
	public static NavitGraphics	N_NavitGraphics	= null;

	// callback id gets set here when called from NavitGraphics
	public static void setKeypressCallback(int kp_cb_id, NavitGraphics ng)
	{
		//Log.e("Navit", "setKeypressCallback -> id1=" + kp_cb_id);
		//Log.e("Navit", "setKeypressCallback -> ng=" + String.valueOf(ng));
		//N_KeypressCallbackID = kp_cb_id;
		N_NavitGraphics = ng;
	}

	public static void setMotionCallback(int mo_cb_id, NavitGraphics ng)
	{
		//Log.e("Navit", "setKeypressCallback -> id2=" + mo_cb_id);
		//Log.e("Navit", "setKeypressCallback -> ng=" + String.valueOf(ng));
		//N_MotionCallbackID = mo_cb_id;
		N_NavitGraphics = ng;
	}

	//public native void KeypressCallback(int id, String s);

	public void start_targetsearch_from_intent(String target_address)
	{
		Navit_last_address_partial_match = false;
		Navit_last_address_search_string = target_address;

		// clear results
		Navit.NavitAddressResultList_foundItems.clear();

		if (Navit_last_address_search_string.equals(""))
		{
			// empty search string entered
			Toast.makeText(getApplicationContext(), Navit.get_text("No address found"), Toast.LENGTH_LONG).show(); //TRANS
		}
		else
		{
			// show dialog
			Message msg = progress_handler.obtainMessage();
			Bundle b = new Bundle();
			msg.what = 11;
			b.putInt("dialog_num", Navit.SEARCHRESULTS_WAIT_DIALOG);
			msg.setData(b);
			progress_handler.sendMessage(msg);
		}
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		// Handle item selection
		switch (item.getItemId())
		{
			case 1 :
				// zoom in
				Message msg = new Message();
				Bundle b = new Bundle();
				b.putInt("Callback", 1);
				msg.setData(b);
				N_NavitGraphics.callback_handler.sendMessage(msg);
				// if we zoom, hide the bubble
				if (N_NavitGraphics.NavitAOverlay != null)
				{
					N_NavitGraphics.NavitAOverlay.hide_bubble();
				}
				Log.e("Navit", "onOptionsItemSelected -> zoom in");
				break;
			case 2 :
				// zoom out
				msg = new Message();
				b = new Bundle();
				b.putInt("Callback", 2);
				msg.setData(b);
				N_NavitGraphics.callback_handler.sendMessage(msg);
				// if we zoom, hide the bubble
				if (N_NavitGraphics.NavitAOverlay != null)
				{
					N_NavitGraphics.NavitAOverlay.hide_bubble();
				}
				Log.e("Navit", "onOptionsItemSelected -> zoom out");
				break;
			case 3 :
				// map download menu for primary
				Intent map_download_list_activity = new Intent(this,
						NavitDownloadSelectMapActivity.class);
				this.startActivityForResult(map_download_list_activity,
						Navit.NavitDownloaderPriSelectMap_id);
				break;
			case 4 :
				// map download menu for second map
				Intent map_download_list_activity2 = new Intent(this,
						NavitDownloadSelectMapActivity.class);
				this.startActivityForResult(map_download_list_activity2,
						Navit.NavitDownloaderSecSelectMap_id);
				break;
			case 5 :
				// toggle the normal POI layers (to avoid double POIs) --> why is this double ???
				msg = new Message();
				b = new Bundle();
				b.putInt("Callback", 5);
				b.putString("cmd", "toggle_layer(\"POI Symbols\");");
				msg.setData(b);
				N_NavitGraphics.callback_handler.sendMessage(msg);

				/*
				 * // toggle the normal POI layers (to avoid double POIs)
				 * msg = new Message();
				 * b = new Bundle();
				 * b.putInt("Callback", 5);
				 * b.putString("cmd", "toggle_layer(\"POI Labels\");");
				 * msg.setData(b);
				 * N_NavitGraphics.callback_handler.sendMessage(msg);
				 */

				// toggle full POI icons on/off --> why is this double ???
				msg = new Message();
				b = new Bundle();
				b.putInt("Callback", 5);
				b.putString("cmd", "toggle_layer(\"Android-POI-Icons-full\");");
				msg.setData(b);
				N_NavitGraphics.callback_handler.sendMessage(msg);

				/*
				 * // toggle full POI labels on/off
				 * msg = new Message();
				 * b = new Bundle();
				 * b.putInt("Callback", 5);
				 * b.putString("cmd", "toggle_layer(\"Android-POI-Labels-full\");");
				 * msg.setData(b);
				 * N_NavitGraphics.callback_handler.sendMessage(msg);
				 */
				break;
			case 6 :
				// ok startup address search activity
				Intent search_intent = new Intent(this, NavitAddressSearchActivity.class);
				search_intent.putExtra("title", Navit.get_text("Enter: City and Street")); //TRANS
				search_intent.putExtra("address_string", Navit_last_address_search_string);
				String pm_temp = "0";
				if (Navit_last_address_partial_match)
				{
					pm_temp = "1";
				}
				search_intent.putExtra("partial_match", pm_temp);
				this.startActivityForResult(search_intent, NavitAddressSearch_id);
				break;
			case 88 :
				// dummy entry, just to make "breaks" in the menu
				break;
			case 99 :
				// exit
				this.onStop();
				this.exit();
				//msg = new Message();
				//b = new Bundle();
				//b.putInt("Callback", 5);
				//b.putString("cmd", "quit();");
				//msg.setData(b);
				//N_NavitGraphics.callback_handler.sendMessage(msg);
				break;
		}
		return true;
	}


	protected void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		switch (requestCode)
		{
			case Navit.NavitDownloaderPriSelectMap_id :
				try
				{
					if (resultCode == Activity.RESULT_OK)
					{
						try
						{
							Log.d("Navit", "PRI id="
									+ Integer.parseInt(data.getStringExtra("selected_id")));
							// set map id to download
							Navit.download_map_id = NavitMapDownloader.OSM_MAP_NAME_ORIG_ID_LIST[Integer
									.parseInt(data.getStringExtra("selected_id"))];
							// show the map download progressbar, and download the map
							if (Navit.download_map_id > -1)
							{
								showDialog(Navit.MAPDOWNLOAD_PRI_DIALOG);
							}
						}
						catch (NumberFormatException e)
						{
							Log.d("Navit", "NumberFormatException selected_id");
						}
					}
					else
					{
						// user pressed back key
					}
				}
				catch (Exception e)
				{
					Log.d("Navit", "error on onActivityResult");
				}
				break;
			case Navit.NavitDownloaderSecSelectMap_id :
				try
				{
					if (resultCode == Activity.RESULT_OK)
					{
						try
						{
							Log.d("Navit", "SEC id="
									+ Integer.parseInt(data.getStringExtra("selected_id")));
							// set map id to download
							Navit.download_map_id = NavitMapDownloader.OSM_MAP_NAME_ORIG_ID_LIST[Integer
									.parseInt(data.getStringExtra("selected_id"))];
							// show the map download progressbar, and download the map
							if (Navit.download_map_id > -1)
							{
								showDialog(Navit.MAPDOWNLOAD_SEC_DIALOG);
							}
						}
						catch (NumberFormatException e)
						{
							Log.d("Navit", "NumberFormatException selected_id");
						}
					}
					else
					{
						// user pressed back key
					}
				}
				catch (Exception e)
				{
					Log.d("Navit", "error on onActivityResult");
				}
				break;
			case NavitAddressSearch_id :
				try
				{
					if (resultCode == Activity.RESULT_OK)
					{
						try
						{
							String addr = data.getStringExtra("address_string");
							Boolean partial_match = data.getStringExtra("partial_match").equals("1");

							Navit_last_address_partial_match = partial_match;
							Navit_last_address_search_string = addr;

							// clear results
							Navit.NavitAddressResultList_foundItems.clear();
							Navit.search_results_towns = 0;
							Navit.search_results_streets = 0;
							Navit.search_results_streets_hn = 0;

							if (addr.equals(""))
							{
								// empty search string entered
								Toast.makeText(getApplicationContext(), Navit.get_text("No search string entered"),
										Toast.LENGTH_LONG).show(); //TRANS
							}
							else
							{
								// show dialog, and start search for the results
								// make it indirect, to give our activity a chance to startup
								// (remember we come straight from another activity and ours is still paused!)
								Message msg = progress_handler.obtainMessage();
								Bundle b = new Bundle();
								msg.what = 11;
								b.putInt("dialog_num", Navit.SEARCHRESULTS_WAIT_DIALOG);
								msg.setData(b);
								progress_handler.sendMessage(msg);
							}
						}
						catch (NumberFormatException e)
						{
							Log.d("Navit", "NumberFormatException selected_id");
						}
					}
					else
					{
						// user pressed back key
					}
				}
				catch (Exception e)
				{
					Log.d("Navit", "error on onActivityResult");
				}
				break;
			case Navit.NavitAddressResultList_id :
				try
				{
					if (resultCode == Activity.RESULT_OK)
					{
						try
						{
							Log.d("Navit", "adress result list id="
									+ Integer.parseInt(data.getStringExtra("selected_id")));
							// get the coords for the destination
							int destination_id = Integer.parseInt(data.getStringExtra("selected_id"));

							// ok now set target
							Toast
									.makeText(
											getApplicationContext(),
											Navit.get_text("setting destination to")+"\n"
													+ Navit.NavitAddressResultList_foundItems
															.get(destination_id).addr, Toast.LENGTH_LONG).show(); //TRANS

							Message msg = new Message();
							Bundle b = new Bundle();
							b.putInt("Callback", 3);
							b.putString("lat", String.valueOf(Navit.NavitAddressResultList_foundItems
									.get(destination_id).lat));
							b.putString("lon", String.valueOf(Navit.NavitAddressResultList_foundItems
									.get(destination_id).lon));
							b.putString("q",
									Navit.NavitAddressResultList_foundItems.get(destination_id).addr);
							msg.setData(b);
							N_NavitGraphics.callback_handler.sendMessage(msg);
						}
						catch (NumberFormatException e)
						{
							Log.d("Navit", "NumberFormatException selected_id");
						}
					}
					else
					{
						// user pressed back key
					}
				}
				catch (Exception e)
				{
					Log.d("Navit", "error on onActivityResult");
				}
				break;
			default :
				//Log.e("Navit", "onActivityResult " + requestCode + " " + resultCode);
				ActivityResults[requestCode].onActivityResult(requestCode, resultCode, data);
				break;
		}
	}

	public class SearchResultsThreadSpinnerThread extends Thread
	{
		int					dialog_num;
		int					spinner_current_value;
		private Boolean	running;
		Handler				mHandler;
		SearchResultsThreadSpinnerThread(Handler h, int dialog_num)
		{
			this.dialog_num = dialog_num;
			this.mHandler = h;
			this.spinner_current_value = 0;
			this.running = true;
			Log.e("Navit", "SearchResultsThreadSpinnerThread created");
		}
		public void run()
		{
			Log.e("Navit", "SearchResultsThreadSpinnerThread started");
			while (this.running)
			{
				if (Navit.NavitAddressSearchSpinnerActive == false)
				{
					this.running = false;
				}
				else
				{
					Message msg = mHandler.obtainMessage();
					Bundle b = new Bundle();
					msg.what = 10;
					b.putInt("dialog_num", this.dialog_num);
					b.putInt("max", Navit.ADDRESS_RESULTS_DIALOG_MAX);
					b.putInt("cur", this.spinner_current_value % (Navit.ADDRESS_RESULTS_DIALOG_MAX + 1));
					b.putString("title", Navit.get_text("getting search results")); //TRANS
					b.putString("text", Navit.get_text("searching ...")); //TRANS
					msg.setData(b);
					mHandler.sendMessage(msg);
					try
					{
						Thread.sleep(700);
					}
					catch (InterruptedException e)
					{
						// e.printStackTrace();
					}
					this.spinner_current_value++;
				}
			}
			Log.e("Navit", "SearchResultsThreadSpinnerThread ended");
		}
	}


	public class SearchResultsThread extends Thread
	{
		private Boolean	running;
		Handler				mHandler;
		int					my_dialog_num;

		SearchResultsThread(Handler h, int dialog_num)
		{
			this.running = true;
			this.mHandler = h;
			this.my_dialog_num = dialog_num;
			Log.e("Navit", "SearchResultsThread created");
		}

		public void stop_me()
		{
			this.running = false;
		}

		public void run()
		{
			Log.e("Navit", "SearchResultsThread started");

			// initialize the dialog with sane values
			Message msg = mHandler.obtainMessage();
			Bundle b = new Bundle();
			msg.what = 10;
			b.putInt("dialog_num", this.my_dialog_num);
			b.putInt("max", Navit.ADDRESS_RESULTS_DIALOG_MAX);
			b.putInt("cur", 0);
			b.putString("title", Navit.get_text("getting search results")); //TRANS
			b.putString("text", Navit.get_text("searching ...")); //TRANS
			msg.setData(b);
			mHandler.sendMessage(msg);

			int partial_match_i = 0;
			if (Navit_last_address_partial_match)
			{
				partial_match_i = 1;
			}

			// start the search, this could take a long time!!
			Log.e("Navit", "SearchResultsThread run1");
			Navit_last_address_search_string = filter_bad_chars(Navit_last_address_search_string);
			N_NavitGraphics.SearchResultList(2, partial_match_i, Navit_last_address_search_string);
			Log.e("Navit", "SearchResultsThread run2");

			Navit.NavitAddressSearchSpinnerActive = false;

			if (Navit.NavitAddressResultList_foundItems.size() > 0)
			{
				open_search_result_list();
			}
			else
			{
				// not results found, show toast
				msg = mHandler.obtainMessage();
				b = new Bundle();
				msg.what = 3;
				b.putString("text", Navit.get_text("No Results found!")); //TRANS
				msg.setData(b);
				mHandler.sendMessage(msg);
			}

			// ok, remove dialog
			msg = mHandler.obtainMessage();
			b = new Bundle();
			msg.what = 99;
			b.putInt("dialog_num", this.my_dialog_num);
			msg.setData(b);
			mHandler.sendMessage(msg);

			Log.e("Navit", "SearchResultsThread ended");
		}
	}

	public static String filter_bad_chars(String in)
	{
		String out = in;
		out = out.replaceAll("\\n", " "); // newline -> space
		out = out.replaceAll("\\r", " "); // return -> space
		out = out.replaceAll("\\t", " "); // tab -> space
		return out;
	}

	public static void msg_to_msg_handler(Bundle b, int id)
	{
		Message msg = Navit_progress_h.obtainMessage();
		msg.what = id;
		msg.setData(b);
		Navit_progress_h.sendMessage(msg);
	}

	public void open_search_result_list()
	{
		// open result list
		Intent address_result_list_activity = new Intent(this, NavitAddressResultListActivity.class);
		this.startActivityForResult(address_result_list_activity, Navit.NavitAddressResultList_id);
	}

	public Handler	progress_handler	= new Handler()
												{
													public void handleMessage(Message msg)
													{
														switch (msg.what)
														{
															case 0 :
																// dismiss dialog, remove dialog
																dismissDialog(msg.getData().getInt("dialog_num"));
																removeDialog(msg.getData().getInt("dialog_num"));

																// exit_code=0 -> OK, map was downloaded fine
																if (msg.getData().getInt("exit_code") == 0)
																{
																	// try to use the new downloaded map (works fine now!)
																	Log.d("Navit", "instance count="
																			+ Navit.getInstanceCount());
																	onStop();
																	onCreate(getIntent().getExtras());

																	//Intent intent = this.getIntent();
																	//startActivity(intent);
																	//finish();

																	//Message msg2 = new Message();
																	//Bundle b2 = new Bundle();
																	//b2.putInt("Callback", 6);
																	//msg2.setData(b2);
																	//N_NavitGraphics.callback_handler.sendMessage(msg2);
																}
																break;
															case 1 :
																// change progressbar values
																int what_dialog = msg.getData()
																		.getInt("dialog_num");
																if (what_dialog == MAPDOWNLOAD_PRI_DIALOG)
																{
																	mapdownloader_dialog_pri.setMax(msg.getData()
																			.getInt("max"));
																	mapdownloader_dialog_pri.setProgress(msg
																			.getData().getInt("cur"));
																	mapdownloader_dialog_pri.setTitle(msg.getData()
																			.getString("title"));
																	mapdownloader_dialog_pri.setMessage(msg
																			.getData().getString("text"));
																}
																else if (what_dialog == MAPDOWNLOAD_SEC_DIALOG)
																{
																	mapdownloader_dialog_sec.setMax(msg.getData()
																			.getInt("max"));
																	mapdownloader_dialog_sec.setProgress(msg
																			.getData().getInt("cur"));
																	mapdownloader_dialog_sec.setTitle(msg.getData()
																			.getString("title"));
																	mapdownloader_dialog_sec.setMessage(msg
																			.getData().getString("text"));
																}
																break;
															case 2 :
																Toast.makeText(getApplicationContext(),
																		msg.getData().getString("text"),
																		Toast.LENGTH_SHORT).show();
																break;
															case 3 :
																Toast.makeText(getApplicationContext(),
																		msg.getData().getString("text"),
																		Toast.LENGTH_LONG).show();
																break;
															case 10 :
																// change values - generic
																int what_dialog_generic = msg.getData().getInt(
																		"dialog_num");
																if (what_dialog_generic == SEARCHRESULTS_WAIT_DIALOG)
																{
																	search_results_wait.setMax(msg.getData().getInt(
																			"max"));
																	search_results_wait.setProgress(msg.getData()
																			.getInt("cur"));
																	search_results_wait.setTitle(msg.getData()
																			.getString("title"));
																	search_results_wait.setMessage(msg.getData()
																			.getString("text"));
																}
																break;
															case 11 :
																// show dialog - generic
																showDialog(msg.getData().getInt("dialog_num"));
																break;
															case 99 :
																// dismiss dialog, remove dialog - generic
																dismissDialog(msg.getData().getInt("dialog_num"));
																removeDialog(msg.getData().getInt("dialog_num"));
																break;
														}
													}
												};

	protected Dialog onCreateDialog(int id)
	{
		switch (id)
		{
			case Navit.SEARCHRESULTS_WAIT_DIALOG :
				search_results_wait = new ProgressDialog(this);
				search_results_wait.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
				search_results_wait.setTitle("--");
				search_results_wait.setMessage("--");
				search_results_wait.setCancelable(false);
				search_results_wait.setProgress(0);
				search_results_wait.setMax(10);
				DialogInterface.OnDismissListener mOnDismissListener3 = new DialogInterface.OnDismissListener()
				{
					public void onDismiss(DialogInterface dialog)
					{
						Log.e("Navit", "onDismiss: search_results_wait");
						dialog.dismiss();
						dialog.cancel();
						searchresultsThread.stop_me();
					}
				};
				search_results_wait.setOnDismissListener(mOnDismissListener3);
				searchresultsThread = new SearchResultsThread(progress_handler,
						Navit.SEARCHRESULTS_WAIT_DIALOG);
				searchresultsThread.start();

				NavitAddressSearchSpinnerActive = true;
				spinner_thread = new SearchResultsThreadSpinnerThread(progress_handler,
						Navit.SEARCHRESULTS_WAIT_DIALOG);
				spinner_thread.start();

				return search_results_wait;
			case Navit.MAPDOWNLOAD_PRI_DIALOG :
				mapdownloader_dialog_pri = new ProgressDialog(this);
				mapdownloader_dialog_pri.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
				mapdownloader_dialog_pri.setTitle("--");
				mapdownloader_dialog_pri.setMessage("--");
				mapdownloader_dialog_pri.setCancelable(true);
				mapdownloader_dialog_pri.setProgress(0);
				mapdownloader_dialog_pri.setMax(200);
				DialogInterface.OnDismissListener mOnDismissListener1 = new DialogInterface.OnDismissListener()
				{
					public void onDismiss(DialogInterface dialog)
					{
						Log.e("Navit", "onDismiss: mapdownloader_dialog pri");
						dialog.dismiss();
						dialog.cancel();
						progressThread_pri.stop_thread();
					}
				};
				mapdownloader_dialog_pri.setOnDismissListener(mOnDismissListener1);
				mapdownloader_pri = new NavitMapDownloader(this);
				progressThread_pri = mapdownloader_pri.new ProgressThread(progress_handler,
						NavitMapDownloader.OSM_MAPS[Navit.download_map_id], MAP_NUM_PRIMARY);
				progressThread_pri.start();
				// show license for OSM maps
				Toast.makeText(getApplicationContext(),
						Navit.get_text("Map data (c) OpenStreetMap contributors, CC-BY-SA"), Toast.LENGTH_LONG).show(); //TRANS
				return mapdownloader_dialog_pri;
			case Navit.MAPDOWNLOAD_SEC_DIALOG :
				mapdownloader_dialog_sec = new ProgressDialog(this);
				mapdownloader_dialog_sec.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
				mapdownloader_dialog_sec.setTitle("--");
				mapdownloader_dialog_sec.setMessage("--");
				mapdownloader_dialog_sec.setCancelable(true);
				mapdownloader_dialog_sec.setProgress(0);
				mapdownloader_dialog_sec.setMax(200);
				DialogInterface.OnDismissListener mOnDismissListener2 = new DialogInterface.OnDismissListener()
				{
					public void onDismiss(DialogInterface dialog)
					{
						Log.e("Navit", "onDismiss: mapdownloader_dialog sec");
						dialog.dismiss();
						dialog.cancel();
						progressThread_sec.stop_thread();
					}
				};
				mapdownloader_dialog_sec.setOnDismissListener(mOnDismissListener2);
				mapdownloader_sec = new NavitMapDownloader(this);
				progressThread_sec = mapdownloader_sec.new ProgressThread(progress_handler,
						NavitMapDownloader.OSM_MAPS[Navit.download_map_id], MAP_NUM_SECONDARY);
				progressThread_sec.start();
				// show license for OSM maps
				Toast.makeText(getApplicationContext(),
						Navit.get_text("Map data (c) OpenStreetMap contributors, CC-BY-SA"), Toast.LENGTH_LONG).show(); //TRANS
				return mapdownloader_dialog_sec;
		}
		// should never get here!!
		return null;
	}

	public void disableSuspend()
	{
		wl.acquire();
		wl.release();
	}

	public void exit()
	{
		finish();
	}

	public boolean handleMessage(Message m)
	{
		//Log.e("Navit", "Handler received message");
		return true;
	}

	public native void NavitMain(Navit x, String lang, int version, String display_density_string);

	public native void NavitActivity(int activity);

	/*
	 * this is used to load the 'navit' native library on
	 * application startup. The library has already been unpacked at
	 * installation time by the package manager.
	 */
	static
	{
		System.loadLibrary("navit");
	}

	/*
	 * Show a search activity with the string "search" filled in
	 */
	private void executeSearch(String search)
	{
		Intent search_intent = new Intent(this, NavitAddressSearchActivity.class);
		search_intent.putExtra("title", Navit.get_text("Enter: City and Street")); //TRANS
		search_intent.putExtra("address_string", search);
		String pm_temp = "0";
		if (Navit_last_address_partial_match)
		{
			pm_temp = "1";
		}
		search_intent.putExtra("partial_match", pm_temp);
		this.startActivityForResult(search_intent, NavitAddressSearch_id);
	}
}
