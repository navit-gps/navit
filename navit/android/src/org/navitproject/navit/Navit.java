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
	public Handler							handler;
	private PowerManager.WakeLock		wl;
	private NavitActivityResult		ActivityResults[];
	public static InputMethodManager	mgr												= null;
	public static DisplayMetrics		metrics											= null;
	public static Boolean				show_soft_keyboard							= false;
	public static Boolean				show_soft_keyboard_now_showing			= false;
	public static long					last_pressed_menu_key						= 0L;
	public static long					time_pressed_menu_key						= 0L;
	private static Intent				startup_intent									= null;
	private static long					startup_intent_timestamp					= 0L;
	public static String					my_display_density							= "mdpi";
	private boolean						parseErrorShown								= false;
	//private static NavitMapDownloader	map_download							= null;
	public static final int				MAPDOWNLOAD_PRI_DIALOG						= 1;
	public static final int				MAPDOWNLOAD_SEC_DIALOG						= 2;
	public ProgressDialog				mapdownloader_dialog_pri					= null;
	public ProgressDialog				mapdownloader_dialog_sec					= null;
	public static NavitMapDownloader	mapdownloader_pri								= null;
	public static NavitMapDownloader	mapdownloader_sec								= null;
	public static final int				NavitDownloaderPriSelectMap_id			= 967;
	public static final int				NavitDownloaderSecSelectMap_id			= 968;
	public static int						download_map_id								= 0;
	ProgressThread							progressThread_pri							= null;
	ProgressThread							progressThread_sec							= null;
	public static final int				MAP_NUM_PRIMARY								= 11;
	public static final int				MAP_NUM_SECONDARY								= 12;
	static final String					MAP_FILENAME_PATH								= "/sdcard/navit/";
	static final String					NAVIT_DATA_DIR									= "/data/data/org.navitproject.navit";
	static final String					NAVIT_DATA_SHARE_DIR							= NAVIT_DATA_DIR
																											+ "/share";
	static final String					FIRST_STARTUP_FILE							= NAVIT_DATA_SHARE_DIR
																											+ "/has_run_once.txt";


	// space !!
	static final String					m													= " ";

	static final String					NAVIT_JAVA_MENU_download_first_map_en	= "download first map";
	static final String					NAVIT_JAVA_MENU_download_first_map_fr	= "télécharchez 1ere carte";
	static final String					NAVIT_JAVA_MENU_download_first_map_nl	= "download eerste kaart";
	static final String					NAVIT_JAVA_MENU_download_first_map_de	= "1te karte runterladen";

	static final String					INFO_BOX_TITLE_en								= "Welcome to Navit";
	static final String					INFO_BOX_TITLE_fr								= "Bienvenue chez Navit";
	static final String					INFO_BOX_TITLE_nl								= "Welkom bij Navit";
	static final String					INFO_BOX_TITLE_de								= "Willkommen bei Navit";

	static final String					INFO_BOX_TEXT_en								= m
																											+ "You are running Navit for the first time!\n\n"
																											+ m
																											+ "To start select \""
																											+ NAVIT_JAVA_MENU_download_first_map_en
																											+ "\"\n"
																											+ m
																											+ "from the menu, and download a map\n"
																											+ m
																											+ "for your current Area.\n"
																											+ m
																											+ "This will download a large file, so please\n"
																											+ m
																											+ "make sure you have a flatrate or similar!\n\n"
																											+ m
																											+ "For more information on Navit\n"
																											+ m
																											+ "visit our Website\n"
																											+ m
																											+ "http://wiki.navit-project.org/\n"
																											+ "\n"
																											+ m
																											+ "      Have fun using Navit.";
	static final String					INFO_BOX_TEXT_fr								= m

																											+ "Vous exécutez Navit pour la première fois\n\n"
																											+ m
																											+ "Pour commencer, sélectionnez \n \""

																											+ NAVIT_JAVA_MENU_download_first_map_fr
																											+ "\"\n"
																											+ m

																											+ "du menu et télechargez une carte\n de votre région.\n"
																											+ m

																											+ "Les cartes sont volumineux, donc\n il est préférable d'avoir une connection\n internet illimitée!\n\n"
																											+ m

																											+ "Pour plus d'infos sur Navit\n"
																											+ m
																											+ "visitez notre site internet\n"
																											+ m

																											+ "http://wiki.navit-project.org/\n"
																											+ "\n"
																											+ m
																											+ "      Amusez vous avec Navit.";

	static final String					INFO_BOX_TEXT_de								= m
																											+ "Sie starten Navit zum ersten Mal!\n\n"
																											+ m
																											+ "Zum loslegen im Menu \""
																											+ NAVIT_JAVA_MENU_download_first_map_en
																											+ "\"\n"
																											+ m
																											+ "auswählen und Karte für die\n"
																											+ m
																											+ "gewünschte Region downloaden.\n"
																											+ m
																											+ "Die Kartendatei ist sehr gross,\n"
																											+ m
																											+ "bitte flatrate oder ähnliches aktivieren!\n\n"
																											+ m
																											+ "Für mehr Infos zu Navit\n"
																											+ m
																											+ "bitte die Website besuchen\n"
																											+ m
																											+ "http://wiki.navit-project.org/\n"
																											+ "\n"
																											+ m
																											+ "      Viel Spaß mit Navit.";
	static final String					INFO_BOX_TEXT_nl								= m

																											+ "U voert Navit voor de eerste keer uit.\n\n"
																											+ m
																											+ "Om te beginnen, selecteer  \n \""

																											+ NAVIT_JAVA_MENU_download_first_map_nl
																											+ "\"\n"
																											+ m

																											+ "uit het menu en download een kaart\n van je regio.\n"
																											+ m

																											+ "De kaarten zijn groot,\n het is dus aangeraden om een \n ongelimiteerde internetverbinding te hebben!\n\n"
																											+ m

																											+ "Voor meer info over Navit\n"
																											+ m
																											+ "bezoek onze site\n"
																											+ m

																											+ "http://wiki.navit-project.org/\n"
																											+ "\n"
																											+ m
																											+ "      Nog veel plezier met Navit.";

	static final String					NAVIT_JAVA_MENU_MOREINFO_en				= "More info";
	static final String					NAVIT_JAVA_MENU_MOREINFO_fr				= "plus d'infos";
	static final String					NAVIT_JAVA_MENU_MOREINFO_nl				= "meer info";
	static final String					NAVIT_JAVA_MENU_MOREINFO_de				= "Mehr infos";

	static final String					NAVIT_JAVA_MENU_ZOOMIN_en					= "zoom in";
	static final String					NAVIT_JAVA_MENU_ZOOMIN_fr					= "zoom-avant";
	static final String					NAVIT_JAVA_MENU_ZOOMIN_nl					= "inzoomen";
	static final String					NAVIT_JAVA_MENU_ZOOMIN_de					= "zoom in";

	static final String					NAVIT_JAVA_MENU_ZOOMOUT_en					= "zoom out";
	static final String					NAVIT_JAVA_MENU_ZOOMOUT_fr					= "zoom-arrière";
	static final String					NAVIT_JAVA_MENU_ZOOMOUT_nl					= "uitzoomen";
	static final String					NAVIT_JAVA_MENU_ZOOMOUT_de					= "zoom out";

	static final String					NAVIT_JAVA_MENU_EXIT_en						= "Exit Navit";
	static final String					NAVIT_JAVA_MENU_EXIT_fr						= "quittez Navit";
	static final String					NAVIT_JAVA_MENU_EXIT_nl						= "Navit afsluiten";
	static final String					NAVIT_JAVA_MENU_EXIT_de						= "Navit Beenden";

	static final String					NAVIT_JAVA_MENU_TOGGLE_POI_en				= "toggle POI";
	static final String					NAVIT_JAVA_MENU_TOGGLE_POI_fr				= "POI on/off";
	static final String					NAVIT_JAVA_MENU_TOGGLE_POI_nl				= "POI aan/uit";
	static final String					NAVIT_JAVA_MENU_TOGGLE_POI_de				= "POI ein/aus";

	static final String					NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE_en	= "drive here";
	static final String					NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE_fr	= "conduisez";
	static final String					NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE_nl	= "Ga naar hier";
	static final String					NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE_de	= "Ziel setzen";

	static final String					NAVIT_JAVA_MENU_download_second_map_en	= "download 2nd map";
	static final String					NAVIT_JAVA_MENU_download_second_map_fr	= "télécharchez 2ème carte";
	static final String					NAVIT_JAVA_MENU_download_second_map_nl	= "download 2de kaart";
	static final String					NAVIT_JAVA_MENU_download_second_map_de	= "2te karte runterladen";

	static String							NAVIT_JAVA_MENU_download_first_map		= NAVIT_JAVA_MENU_download_first_map_en;
	static String							NAVIT_JAVA_MENU_download_second_map		= NAVIT_JAVA_MENU_download_second_map_en;
	static String							INFO_BOX_TITLE									= INFO_BOX_TITLE_en;
	static String							INFO_BOX_TEXT									= INFO_BOX_TEXT_en;
	static String							NAVIT_JAVA_MENU_MOREINFO					= NAVIT_JAVA_MENU_MOREINFO_en;
	static String							NAVIT_JAVA_MENU_ZOOMIN						= NAVIT_JAVA_MENU_ZOOMIN_en;
	static String							NAVIT_JAVA_MENU_ZOOMOUT						= NAVIT_JAVA_MENU_ZOOMOUT_en;
	static String							NAVIT_JAVA_MENU_EXIT							= NAVIT_JAVA_MENU_EXIT_en;
	static String							NAVIT_JAVA_MENU_TOGGLE_POI					= NAVIT_JAVA_MENU_TOGGLE_POI_en;
	static String							NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE		= NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE_en;


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

		Locale locale = java.util.Locale.getDefault();
		String lang = locale.getLanguage();
		String langu = lang;
		String langc = lang;

		Log.e("Navit", "lang=" + lang);

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
			NAVIT_JAVA_MENU_download_first_map = NAVIT_JAVA_MENU_download_first_map_de;
			NAVIT_JAVA_MENU_download_second_map = NAVIT_JAVA_MENU_download_second_map_de;
			INFO_BOX_TITLE = INFO_BOX_TITLE_de;
			INFO_BOX_TEXT = INFO_BOX_TEXT_de;
			NAVIT_JAVA_MENU_MOREINFO = NAVIT_JAVA_MENU_MOREINFO_de;
			NAVIT_JAVA_MENU_ZOOMIN = NAVIT_JAVA_MENU_ZOOMIN_de;
			NAVIT_JAVA_MENU_ZOOMOUT = NAVIT_JAVA_MENU_ZOOMOUT_de;
			NAVIT_JAVA_MENU_EXIT = NAVIT_JAVA_MENU_EXIT_de;
			NAVIT_JAVA_MENU_TOGGLE_POI = NAVIT_JAVA_MENU_TOGGLE_POI_de;
			NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE = NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE_de;
		}
		else if (lang.compareTo("fr") == 0)
		{
			NAVIT_JAVA_MENU_download_first_map = NAVIT_JAVA_MENU_download_first_map_fr;
			NAVIT_JAVA_MENU_download_second_map = NAVIT_JAVA_MENU_download_second_map_fr;
			INFO_BOX_TITLE = INFO_BOX_TITLE_fr;
			INFO_BOX_TEXT = INFO_BOX_TEXT_fr;
			NAVIT_JAVA_MENU_MOREINFO = NAVIT_JAVA_MENU_MOREINFO_fr;
			NAVIT_JAVA_MENU_ZOOMIN = NAVIT_JAVA_MENU_ZOOMIN_fr;
			NAVIT_JAVA_MENU_ZOOMOUT = NAVIT_JAVA_MENU_ZOOMOUT_fr;
			NAVIT_JAVA_MENU_EXIT = NAVIT_JAVA_MENU_EXIT_fr;
			NAVIT_JAVA_MENU_TOGGLE_POI = NAVIT_JAVA_MENU_TOGGLE_POI_fr;
			NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE = NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE_fr;
		}
		else if (lang.compareTo("nl") == 0)
		{
			NAVIT_JAVA_MENU_download_first_map = NAVIT_JAVA_MENU_download_first_map_nl;
			NAVIT_JAVA_MENU_download_second_map = NAVIT_JAVA_MENU_download_second_map_nl;
			INFO_BOX_TITLE = INFO_BOX_TITLE_nl;
			INFO_BOX_TEXT = INFO_BOX_TEXT_nl;
			NAVIT_JAVA_MENU_MOREINFO = NAVIT_JAVA_MENU_MOREINFO_nl;
			NAVIT_JAVA_MENU_ZOOMIN = NAVIT_JAVA_MENU_ZOOMIN_nl;
			NAVIT_JAVA_MENU_ZOOMOUT = NAVIT_JAVA_MENU_ZOOMOUT_nl;
			NAVIT_JAVA_MENU_EXIT = NAVIT_JAVA_MENU_EXIT_nl;
			NAVIT_JAVA_MENU_TOGGLE_POI = NAVIT_JAVA_MENU_TOGGLE_POI_nl;
			NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE = NAVIT_JAVA_OVERLAY_BUBBLE_DRIVEHERE_nl;
		}
		// hardcoded strings for now, use routine down below later!!
		// hardcoded strings for now, use routine down below later!!


		/*
		 * show info box for first time users
		 */
		AlertDialog.Builder infobox = new AlertDialog.Builder(this);
		infobox.setTitle(INFO_BOX_TITLE);
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
		final SpannableString s = new SpannableString(INFO_BOX_TEXT);
		Linkify.addLinks(s, Linkify.WEB_URLS);
		message.setText(s);
		message.setMovementMethod(LinkMovementMethod.getInstance());
		infobox.setView(message);

		infobox.setPositiveButton("Ok", new DialogInterface.OnClickListener()
		{
			public void onClick(DialogInterface arg0, int arg1)
			{
				Log.e("Navit", "Ok, user saw the infobox");
			}
		});
		infobox.setNeutralButton(NAVIT_JAVA_MENU_MOREINFO, new DialogInterface.OnClickListener()
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

		int pos = langu.indexOf('_');
		if (pos != -1)
		{
			langc = langu.substring(0, pos);
			langu = langc + langu.substring(pos).toUpperCase(locale);
		}
		else
		{
			String country = locale.getCountry();
			Log.e("Navit", "Country " + country);
			langu = langc + "_" + country.toUpperCase(locale);
		}
		Log.e("Navit", "Language " + lang);

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

			// a: google.navigation:ll=48.25676,16.643&q=blabla-strasse
			// b: google.navigation:q=48.25676,16.643
			// c: google.navigation:ll=48.25676,16.643
			String lat;
			String lon;
			String q;

			String temp1 = null;
			String temp2 = null;
			String temp3 = null;
			boolean parsable = false;

			// if b: then remodel the input string to look like a:
			if (intent_data.substring(0, 20).equals("google.navigation:q="))
			{
				intent_data = "ll=" + intent_data.split("q=", -1)[1] + "&q=Target";
				parsable = true;
			}
			// if c: then remodel the input string to look like a:
			else if ((intent_data.substring(0, 21).equals("google.navigation:ll="))
					&& (intent_data.split("&q=").length == 0))
			{
				intent_data = intent_data + "&q=Target";
				parsable = true;
			}
			// already looks like a: just set flag
			else if ((intent_data.substring(0, 21).equals("google.navigation:ll="))
					&& (intent_data.split("&q=").length > 0))
			{
				// dummy, just set the flag
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
				// string not parsable, display alert and continue w/o string
				AlertDialog.Builder alertbox = new AlertDialog.Builder(this);
				alertbox.setMessage("Navit recieved the query " + intent_data
						+ "\nThis is not yet parsable.");
				alertbox.setPositiveButton("Ok", new DialogInterface.OnClickListener()
				{
					public void onClick(DialogInterface arg0, int arg1)
					{
						Log.e("Navit", "Accepted non-parsable string");
					}
				});
				alertbox.setNeutralButton("More info", new DialogInterface.OnClickListener()
				{
					public void onClick(DialogInterface arg0, int arg1)
					{
						String url = "http://wiki.navit-project.org/index.php/Navit_on_Android#Parse_error";
						Intent i = new Intent(Intent.ACTION_VIEW);
						i.setData(Uri.parse(url));
						startActivity(i);
					}
				});
				if (!parseErrorShown)
				{
					alertbox.show();
					parseErrorShown = true;
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
		menu.add(1, 1, 10, NAVIT_JAVA_MENU_ZOOMIN);
		menu.add(1, 2, 20, NAVIT_JAVA_MENU_ZOOMOUT);

		menu.add(1, 3, 22, NAVIT_JAVA_MENU_download_first_map);
		menu.add(1, 4, 23, NAVIT_JAVA_MENU_download_second_map);

		menu.add(1, 5, 40, NAVIT_JAVA_MENU_TOGGLE_POI);
		menu.add(1, 99, 45, NAVIT_JAVA_MENU_EXIT);
		return true;
	}

	//public native void KeypressCallback(int id, String s);

	// define callback id here
	//static int				N_KeypressCallbackID;
	//static int				N_MotionCallbackID;
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
				// toggle the normal POI layers (to avoid double POIs)
				msg = new Message();
				b = new Bundle();
				b.putInt("Callback", 5);
				b.putString("cmd", "toggle_layer(\"POI Symbols\");");
				msg.setData(b);
				N_NavitGraphics.callback_handler.sendMessage(msg);
				// toggle the normal POI layers (to avoid double POIs)
				msg = new Message();
				b = new Bundle();
				b.putInt("Callback", 5);
				b.putString("cmd", "toggle_layer(\"POI Labels\");");
				msg.setData(b);
				N_NavitGraphics.callback_handler.sendMessage(msg);


				// toggle full POI icons on/off
				msg = new Message();
				b = new Bundle();
				b.putInt("Callback", 5);
				b.putString("cmd", "toggle_layer(\"Android-POI-Icons-full\");");
				msg.setData(b);
				N_NavitGraphics.callback_handler.sendMessage(msg);
				// toggle full POI labels on/off
				msg = new Message();
				b = new Bundle();
				b.putInt("Callback", 5);
				b.putString("cmd", "toggle_layer(\"Android-POI-Labels-full\");");
				msg.setData(b);
				N_NavitGraphics.callback_handler.sendMessage(msg);
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
							Navit.download_map_id = Integer.parseInt(data.getStringExtra("selected_id"));
							// show the map download progressbar, and download the map
							showDialog(Navit.MAPDOWNLOAD_PRI_DIALOG);
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
							Navit.download_map_id = Integer.parseInt(data.getStringExtra("selected_id"));
							// show the map download progressbar, and download the map
							showDialog(Navit.MAPDOWNLOAD_SEC_DIALOG);
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
														}
													}
												};

	protected Dialog onCreateDialog(int id)
	{
		switch (id)
		{
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

	/*
	 * A native method that is implemented by the
	 * 'hello-jni' native library, which is packaged
	 * with this application.
	 */
	public native void NavitMain(Navit x, String lang, int version, String display_density_string);
	public native void NavitActivity(int activity);

	/*
	 * this is used to load the 'hello-jni' library on application
	 * startup. The library has already been unpacked into
	 * /data/data/com.example.Navit/lib/libhello-jni.so at
	 * installation time by the package manager.
	 */
	static
	{
		System.loadLibrary("navit");
	}
}
