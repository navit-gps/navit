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
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Bundle;
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


public class Navit extends Activity
{
	public static final class NavitAddress
	{
		String	result_type;	// TWN,STR,SHN
		String	item_id;	// H<ddddd>L<ddddd> -> item.id_hi item.id_lo
		float	lat;
		float	lon;
		String	addr;
	}

	public NavitDialogs              dialogs;
	private PowerManager.WakeLock    wl;
	private NavitActivityResult      ActivityResults[];
	public static InputMethodManager mgr                            = null;
	public static DisplayMetrics     metrics                        = null;
	public static Boolean            show_soft_keyboard             = false;
	public static Boolean            show_soft_keyboard_now_showing = false;
	public static long               last_pressed_menu_key          = 0L;
	public static long               time_pressed_menu_key          = 0L;
	private static Intent            startup_intent                 = null;
	private static long              startup_intent_timestamp       = 0L;
	public static String             my_display_density             = "mdpi";
	private boolean                  searchBoxShown                 = false;
	public static final int          ADDRESS_RESULTS_DIALOG_MAX     = 10;
	public static final int          NavitDownloaderSelectMap_id    = 967;
	public static int                search_results_towns           = 0;
	public static int                search_results_streets         = 0;
	public static int                search_results_streets_hn      = 0;
	public static final int          MAP_NUM_PRIMARY                = 11;
	public static final int          NavitAddressSearch_id          = 70;
	public static final int          NavitAddressResultList_id      = 71;
	public static String             NavitLanguage;

	public static List<NavitAddress> NavitAddressResultList_foundItems = new ArrayList<NavitAddress>();

	public static final int          MAP_NUM_SECONDARY              = 12;
	static final String              NAVIT_PACKAGE_NAME             = "org.navitproject.navit";
	static final String              TAG                            = "Navit";
	static final String              MAP_FILENAME_PATH              = "/sdcard/navit/";
	static final String              NAVIT_DATA_DIR                 = "/data/data/" + NAVIT_PACKAGE_NAME;
	static final String              NAVIT_DATA_SHARE_DIR           = NAVIT_DATA_DIR + "/share";
	static final String              FIRST_STARTUP_FILE             = NAVIT_DATA_SHARE_DIR + "/has_run_once.txt";
	public static final String       NAVIT_PREFS                    = "NavitPrefs";

	public static String get_text(String in)
	{
		return NavitTextTranslations.get_text(in);
	}

	private boolean extractRes(String resname, String result) {
		boolean needs_update = false;
		Resources res = getResources();
		Log.e(TAG, "Res Name " + resname + ", result " + result);
		int id = res.getIdentifier(resname, "raw", NAVIT_PACKAGE_NAME);
		Log.e(TAG, "Res ID " + id);
		if (id == 0)
			return false;

		File resultfile = new File(result);
		if (!resultfile.exists()) {
			needs_update = true;
			File path = resultfile.getParentFile();
			if ( !path.exists() && !resultfile.getParentFile().mkdirs())
				return false;
		} else {
			PackageManager pm = getPackageManager();
			ApplicationInfo appInfo;
			long apkUpdateTime = 0;
			try {
				appInfo = pm.getApplicationInfo(NAVIT_PACKAGE_NAME, 0);
				apkUpdateTime = new File(appInfo.sourceDir).lastModified();
			} catch (NameNotFoundException e) {
				Log.e(TAG, "Could not read package infos");
				e.printStackTrace();
			}
			if (apkUpdateTime > resultfile.lastModified())
				needs_update = true;
		}

		if (needs_update) {
			Log.e(TAG, "Extracting resource");

			try {
				InputStream resourcestream = res.openRawResource(id);
				FileOutputStream resultfilestream = new FileOutputStream(resultfile);
				byte[] buf = new byte[1024];
				int i = 0;
				while ((i = resourcestream.read(buf)) != -1) {
					resultfilestream.write(buf, 0, i);
				}
			} catch (Exception e) {
				Log.e(TAG, "Exception " + e.getMessage());
				return false;
			}
		}
		return true;
	}

	private void showInfos()
	{
		SharedPreferences settings = getSharedPreferences(NAVIT_PREFS, MODE_PRIVATE);
		boolean firstStart = settings.getBoolean("firstStart", true);

		if (firstStart)
		{
			AlertDialog.Builder infobox = new AlertDialog.Builder(this);
			infobox.setTitle(getString(R.string.initial_info_box_title)); // TRANS
			infobox.setCancelable(false);
			final TextView message = new TextView(this);
			message.setFadingEdgeLength(20);
			message.setVerticalFadingEdgeEnabled(true);
			// message.setVerticalScrollBarEnabled(true);
			RelativeLayout.LayoutParams rlp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.FILL_PARENT, RelativeLayout.LayoutParams.FILL_PARENT);
	
			message.setLayoutParams(rlp);
			final SpannableString s = new SpannableString(getString(R.string.initial_info_box_message)); // TRANS
			Linkify.addLinks(s, Linkify.WEB_URLS);
			message.setText(s);
			message.setMovementMethod(LinkMovementMethod.getInstance());
			infobox.setView(message);
	
			// TRANS
			infobox.setPositiveButton(getString(R.string.initial_info_box_OK), new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface arg0, int arg1) {
					Log.e("Navit", "Ok, user saw the infobox");
				}
			});

			// TRANS
			infobox.setNeutralButton(getString(R.string.initial_info_box_more_info), new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface arg0, int arg1) {
					Log.e("Navit", "user wants more info, show the website");
					String url = "http://wiki.navit-project.org/index.php/Navit_on_Android";
					Intent i = new Intent(Intent.ACTION_VIEW);
					i.setData(Uri.parse(url));
					startActivity(i);
				}
			});
			infobox.show();
			SharedPreferences.Editor edit_settings = settings.edit();
			edit_settings.putBoolean("firstStart", false);
			edit_settings.commit();
		}
	}

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		dialogs = new NavitDialogs(this);

		// only take arguments here, onResume gets called all the time (e.g. when screenblanks, etc.)
		Navit.startup_intent = this.getIntent();
		// hack! Remember time stamps, and only allow 4 secs. later in onResume to set target!
		Navit.startup_intent_timestamp = System.currentTimeMillis();
		Log.e("Navit", "**1**A " + startup_intent.getAction());
		Log.e("Navit", "**1**D " + startup_intent.getDataString());

		// init translated text
		NavitTextTranslations.init();
		
		// NOTIFICATION
		// Setup the status bar notification		
		// This notification is removed in the exit() function
		NotificationManager nm = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);	// Grab a handle to the NotificationManager
		Notification NavitNotification = new Notification(R.drawable.icon,"Navit started",System.currentTimeMillis());	// Create a new notification, with the text string to show when the notification first appears
		PendingIntent appIntent = PendingIntent.getActivity(getApplicationContext(), 0, getIntent(), 0);
		NavitNotification.setLatestEventInfo(getApplicationContext(), "Navit", "Navit running", appIntent);	// Set the text in the notification
		NavitNotification.flags|=Notification.FLAG_ONGOING_EVENT;	// Ensure that the notification appears in Ongoing
		nm.notify(R.string.app_name, NavitNotification);	// Set the notification
		
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
			NavitLanguage = langc + langu.substring(pos).toUpperCase(locale);
			Log.e("Navit", "substring lang " + NavitLanguage.substring(pos).toUpperCase(locale));
			// set lang. for translation
			NavitTextTranslations.main_language = langc;
			NavitTextTranslations.sub_language = NavitLanguage.substring(pos).toUpperCase(locale);
		}
		else
		{
			String country = locale.getCountry();
			Log.e("Navit", "Country1 " + country);
			Log.e("Navit", "Country2 " + country.toUpperCase(locale));
			NavitLanguage = langc + "_" + country.toUpperCase(locale);
			// set lang. for translation
			NavitTextTranslations.main_language = langc;
			NavitTextTranslations.sub_language = country.toUpperCase(locale);
		}
		Log.e("Navit", "Language " + lang);

		// make sure the new path for the navitmap.bin file(s) exist!!
		File navit_maps_dir = new File(MAP_FILENAME_PATH);
		navit_maps_dir.mkdirs();

		// make sure the share dir exists
		File navit_data_share_dir = new File(NAVIT_DATA_SHARE_DIR);
		navit_data_share_dir.mkdirs();

		Display display_ = getWindowManager().getDefaultDisplay();
		int width_ = display_.getWidth();
		int height_ = display_.getHeight();
		metrics = new DisplayMetrics();
		display_.getMetrics(Navit.metrics);
		int densityDpi = (int)(( Navit.metrics.density*160)+.5f);
		Log.e("Navit", "Navit -> pixels x=" + width_ + " pixels y=" + height_);
		Log.e("Navit", "Navit -> dpi=" + densityDpi);
		Log.e("Navit", "Navit -> density=" + Navit.metrics.density);
		Log.e("Navit", "Navit -> scaledDensity=" + Navit.metrics.scaledDensity);

		ActivityResults = new NavitActivityResult[16];
		setVolumeControlStream(AudioManager.STREAM_MUSIC);
		PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
		wl = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK | PowerManager.ON_AFTER_RELEASE,"NavitDoNotDimScreen");

		if (!extractRes(langc, NAVIT_DATA_DIR + "/locale/" + langc + "/LC_MESSAGES/navit.mo"))
		{
			Log.e("Navit", "Failed to extract language resource " + langc);
		}

		if (densityDpi <= 120)
		{
			my_display_density = "ldpi";
		}
		else if (densityDpi <= 160)
		{
			my_display_density = "mdpi";
		}
		else if (densityDpi < 320)
		{
			my_display_density = "hdpi";
		}
		else
		{
			Log.e("Navit", "found xhdpi device, this is not fully supported!!");
			Log.e("Navit", "using hdpi values");
			my_display_density = "hdpi";
		}

		if (!extractRes("navit" + my_display_density, NAVIT_DATA_DIR + "/share/navit.xml"))
		{
			Log.e("Navit", "Failed to extract navit.xml for " + my_display_density);
		}

		// --> dont use android.os.Build.VERSION.SDK_INT, needs API >= 4
		Log.e("Navit", "android.os.Build.VERSION.SDK_INT=" + Integer.valueOf(android.os.Build.VERSION.SDK));
		NavitMain(this, NavitLanguage, Integer.valueOf(android.os.Build.VERSION.SDK), my_display_density, NAVIT_DATA_DIR+"/bin/navit");

		activateAllMaps();

		showInfos();

		Navit.mgr = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
	}

	private void activateAllMaps()
	{
		NavitMap maps[] = NavitMapDownloader.getAvailableMaps();
		for (NavitMap map : maps) {
			Message msg = Message.obtain(N_NavitGraphics.callback_handler, NavitGraphics.msg_type.CLB_LOAD_MAP.ordinal());
			Bundle b = new Bundle();
			b.putString("title", map.getLocation());
			msg.setData(b);
			msg.sendToTarget();
		}
	}

	@Override
	public void onResume()
	{
		super.onResume();
		Log.e("Navit", "OnResume");
		//InputMethodManager mgr = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
		// DEBUG
		// intent_data = "google.navigation:q=Wien Burggasse 27";
		// intent_data = "google.navigation:q=48.25676,16.643";
		// intent_data = "google.navigation:ll=48.25676,16.643&q=blabla-strasse";
		// intent_data = "google.navigation:ll=48.25676,16.643";
		if (startup_intent != null)
		{
			if (System.currentTimeMillis() <= Navit.startup_intent_timestamp + 4000L)
			{
				Log.e("Navit", "**2**A " + startup_intent.getAction());
				Log.e("Navit", "**2**D " + startup_intent.getDataString());
				String navi_scheme = startup_intent.getScheme();
				if ( navi_scheme != null && navi_scheme.equals("google.navigation")) {
					parseNavigationURI(startup_intent.getData().getSchemeSpecificPart());
				}
			}
			else {
				Log.e("Navit", "timestamp for navigate_to expired! not using data");
			}
		}
	}

	private void parseNavigationURI(String schemeSpecificPart) {
		String naviData[]= schemeSpecificPart.split("&");
		Pattern p = Pattern.compile("(.*)=(.*)");
		Map<String,String> params = new HashMap<String,String>();
		for (int count=0; count < naviData.length; count++) {
			Matcher m = p.matcher(naviData[count]);
			
			if (m.matches()) {
				params.put(m.group(1), m.group(2));
			}
		}
		
		// d: google.navigation:q=blabla-strasse # (this happens when you are offline, or from contacts)
		// a: google.navigation:ll=48.25676,16.643&q=blabla-strasse
		// c: google.navigation:ll=48.25676,16.643
		// b: google.navigation:q=48.25676,16.643
		
		Float lat;
		Float lon;
		Bundle b = new Bundle();
		
		String geoString = params.get("ll");
		if (geoString != null) {
			String address = params.get("q");
			if (address != null) b.putString("q", address);
		}
		else {
			geoString = params.get("q");
		}
		
		if ( geoString != null) {
			if (geoString.matches("^[+-]{0,1}\\d+(|\\.\\d*),[+-]{0,1}\\d+(|\\.\\d*)$")) {
				String geo[] = geoString.split(",");
				if (geo.length == 2) {
					try {
						lat = Float.valueOf(geo[0]);
						lon = Float.valueOf(geo[1]);
						b.putFloat("lat", lat);
						b.putFloat("lon", lon);
						Message msg = Message.obtain(N_NavitGraphics.callback_handler, NavitGraphics.msg_type.CLB_SET_DESTINATION.ordinal());

						msg.setData(b);
						msg.sendToTarget();
						Log.e("Navit", "target found (b): " + geoString);
					} catch (NumberFormatException e) { } // nothing to do here
				}
			}
			else {
				start_targetsearch_from_intent(geoString);
			}
		}
	}

	public void setActivityResult(int requestCode, NavitActivityResult ActivityResult)
	{
		//Log.e("Navit", "setActivityResult " + requestCode);
		ActivityResults[requestCode] = ActivityResult;
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
		menu.add(1, 1, 100, getString(R.string.optionsmenu_zoom_in)); //TRANS
		menu.add(1, 2, 200, getString(R.string.optionsmenu_zoom_out)); //TRANS

		menu.add(1, 3, 300, getString(R.string.optionsmenu_download_maps)); //TRANS
		menu.add(1, 5, 400, getString(R.string.optionsmenu_toggle_poi)); //TRANS

		menu.add(1, 6, 500, getString(R.string.optionsmenu_address_search)); //TRANS

		menu.add(1, 99, 900, getString(R.string.optionsmenu_exit_navit)); //TRANS
		return true;
	}

	// define callback id here
	public static NavitGraphics N_NavitGraphics = null;

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

	public void start_targetsearch_from_intent(String target_address)
	{
		NavitDialogs.Navit_last_address_partial_match = false;
		NavitDialogs.Navit_last_address_search_string = target_address;

		// clear results
		Navit.NavitAddressResultList_foundItems.clear();

		if (NavitDialogs.Navit_last_address_search_string.equals(""))
		{
			// empty search string entered
			Toast.makeText(getApplicationContext(), getString(R.string.address_search_not_found), Toast.LENGTH_LONG).show(); //TRANS
		}
		else
		{
			// show dialog
			dialogs.obtainMessage(NavitDialogs.MSG_SEARCH).sendToTarget();
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
				Message.obtain(N_NavitGraphics.callback_handler, NavitGraphics.msg_type.CLB_ZOOM_IN.ordinal()).sendToTarget();
				// if we zoom, hide the bubble
				Log.e("Navit", "onOptionsItemSelected -> zoom in");
				break;
			case 2 :
				// zoom out
				Message.obtain(N_NavitGraphics.callback_handler, NavitGraphics.msg_type.CLB_ZOOM_OUT.ordinal()).sendToTarget();
				// if we zoom, hide the bubble
				Log.e("Navit", "onOptionsItemSelected -> zoom out");
				break;
			case 3 :
				// map download menu
				Intent map_download_list_activity = new Intent(this, NavitDownloadSelectMapActivity.class);
				startActivityForResult(map_download_list_activity, Navit.NavitDownloaderSelectMap_id);
				break;
			case 5 :
				// toggle the normal POI layers (to avoid double POIs)
				Message msg = Message.obtain(N_NavitGraphics.callback_handler, NavitGraphics.msg_type.CLB_CALL_CMD.ordinal());
				Bundle b = new Bundle();
				b.putString("cmd", "toggle_layer(\"POI Symbols\");");
				msg.setData(b);
				msg.sendToTarget();

				// toggle full POI icons on/off
				msg = Message.obtain(N_NavitGraphics.callback_handler, NavitGraphics.msg_type.CLB_CALL_CMD.ordinal());
				b = new Bundle();
				b.putString("cmd", "toggle_layer(\"Android-POI-Icons-full\");");
				msg.setData(b);
				msg.sendToTarget();

				break;
			case 6 :
				// ok startup address search activity
				Intent search_intent = new Intent(this, NavitAddressSearchActivity.class);
				search_intent.putExtra("title", getString(R.string.address_search_title)); //TRANS
				search_intent.putExtra("address_string", NavitDialogs.Navit_last_address_search_string);
				search_intent.putExtra("partial_match", NavitDialogs.Navit_last_address_partial_match);
				this.startActivityForResult(search_intent, NavitAddressSearch_id);
				break;
			case 99 :
				// exit
				this.onStop();
				this.exit();
				break;
		}
		return true;
	}

	void setDestination(float latitude, float longitude, String address) {
		Toast.makeText( getApplicationContext(),getString(R.string.address_search_set_destination) + "\n" + address, Toast.LENGTH_LONG).show(); //TRANS

		Message msg = Message.obtain(N_NavitGraphics.callback_handler, NavitGraphics.msg_type.CLB_SET_DESTINATION.ordinal());
		Bundle b = new Bundle();
		b.putFloat("lat", latitude);
		b.putFloat("lon", longitude);
		b.putString("q", address);
		msg.setData(b);
		msg.sendToTarget();
	}

	protected void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		switch (requestCode)
		{
		case Navit.NavitDownloaderSelectMap_id :
			if (resultCode == Activity.RESULT_OK)
			{
				Message msg = dialogs.obtainMessage(NavitDialogs.MSG_START_MAP_DOWNLOAD
						, data.getIntExtra("map_index", -1), 0);
				msg.sendToTarget();
			}
			break;
		case NavitAddressSearch_id :
			if (resultCode == Activity.RESULT_OK) {
				Boolean addr_selected = data.getBooleanExtra("addr_selected", false);
				
				// address already choosen, or do we have to search?
				if (addr_selected) {
					setDestination( NavitAddressResultList_foundItems .get(0).lat
					              , NavitAddressResultList_foundItems.get(0).lon
					              , NavitAddressResultList_foundItems.get(0).addr);
				} else {
					String addr = data.getStringExtra("address_string");
					Boolean partial_match = data.getBooleanExtra("partial_match", false);
					String country = data.getStringExtra("country");
	
					NavitDialogs.Navit_last_address_partial_match = partial_match;
					NavitDialogs.Navit_last_address_search_string = addr;
					NavitDialogs.Navit_last_country = country;
	
					// clear results
					Navit.NavitAddressResultList_foundItems.clear();
					Navit.search_results_towns = 0;
					Navit.search_results_streets = 0;
					Navit.search_results_streets_hn = 0;
	
					if (addr.equals("")) {
						// empty search string entered
						Toast.makeText(getApplicationContext(),getString(R.string.address_search_no_text_entered), Toast.LENGTH_LONG).show(); //TRANS
					} else {
						// show dialog, and start search for the results
						// make it indirect, to give our activity a chance to startup
						// (remember we come straight from another activity and ours is still paused!)
						dialogs.obtainMessage(NavitDialogs.MSG_SEARCH).sendToTarget();
					}
				}
			}
			break;
		case Navit.NavitAddressResultList_id :
			try
			{
				if (resultCode == Activity.RESULT_OK)
				{
					int destination_id = data.getIntExtra("selected_id", 0);
					
					setDestination( NavitAddressResultList_foundItems .get(destination_id).lat
					              , NavitAddressResultList_foundItems.get(destination_id).lon
					              , NavitAddressResultList_foundItems.get(destination_id).addr);
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

	protected Dialog onCreateDialog(int id)
	{
		return dialogs.createDialog(id);
	}

	@Override
	public void onDestroy()
	{
		super.onDestroy();
		Log.e("Navit", "OnDestroy");
		// TODO next call will kill our app the hard way. This should not be necessary, but ensures navit is
		// properly restarted and no resources are wasted with navit in background. Remove this call after 
		// code review
		NavitDestroy();
	}

	public void disableSuspend()
	{
		wl.acquire();
		wl.release();
	}

	public void exit()
	{
		NotificationManager nm = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
		nm.cancel(R.string.app_name);
		NavitVehicle.removeListener();
		finish();
	}

	public native void NavitMain(Navit x, String lang, int version, String display_density_string, String path);
	public native void NavitDestroy();

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
		search_intent.putExtra("title", getString(R.string.address_search_title)); //TRANS
		search_intent.putExtra("address_string", search);
		search_intent.putExtra("partial_match", NavitDialogs.Navit_last_address_partial_match);
		this.startActivityForResult(search_intent, NavitAddressSearch_id);
	}
}
