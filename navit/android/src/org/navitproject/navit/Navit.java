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
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.Menu;
import android.view.MenuItem;
import android.view.inputmethod.InputMethodManager;
import android.widget.Toast;


public class Navit extends Activity implements Handler.Callback
{
	public Handler								handler;
	private PowerManager.WakeLock			wl;
	private NavitActivityResult			ActivityResults[];
	public static InputMethodManager		mgr										= null;
	public static DisplayMetrics			metrics									= null;
	public static Boolean					show_soft_keyboard					= false;
	public static Boolean					show_soft_keyboard_now_showing	= false;
	public static long						last_pressed_menu_key				= 0L;
	public static long						time_pressed_menu_key				= 0L;
	private static Intent					startup_intent							= null;
	private static long						startup_intent_timestamp			= 0L;
	public static String						my_display_density					= "mdpi";
	private boolean							parseErrorShown						= false;
	private static NavitMapDownloader	map_download							= null;
	public static final int					MAPDOWNLOAD_DIALOG					= 1;
	public ProgressDialog					mapdownloader_dialog					= null;
	public static NavitMapDownloader		mapdownloader							= null;
	public static final int					NavitDownloaderSelectMap_id		= 967;
	public static int							download_map_id						= 0;
	ProgressThread								progressThread							= null;

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

		// only take arguments here, in onResume gets called all the time (e.g. when screenblanks, etc.)
		Navit.startup_intent = this.getIntent();
		// hack! remeber timstamp, and only allow 4 secs. later in onResume to set target!
		Navit.startup_intent_timestamp = System.currentTimeMillis();
		Log.e("Navit", "**1**A " + startup_intent.getAction());
		Log.e("Navit", "**1**D " + startup_intent.getDataString());

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
		Locale locale = java.util.Locale.getDefault();
		String lang = locale.getLanguage();
		String langu = lang;
		String langc = lang;
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

		if (!extractRes(langc, "/data/data/org.navitproject.navit/locale/" + langc
				+ "/LC_MESSAGES/navit.mo"))
		{
			Log.e("Navit", "Failed to extract language resource " + langc);
		}

		my_display_density = "mdpi";
		// hdpi display
		if (Navit.metrics.densityDpi == 240)
		{
			my_display_density = "hdpi";
			if (!extractRes("navithdpi", "/data/data/org.navitproject.navit/share/navit.xml"))
			{
				Log.e("Navit", "Failed to extract navit.xml for hdpi device(s)");
			}
		}
		// mdpi display
		else if (Navit.metrics.densityDpi == 160)
		{
			my_display_density = "mdpi";
			if (!extractRes("navitmdpi", "/data/data/org.navitproject.navit/share/navit.xml"))
			{
				Log.e("Navit", "Failed to extract navit.xml for mdpi device(s)");
			}
		}
		// ldpi display
		else if (Navit.metrics.densityDpi == 120)
		{
			my_display_density = "ldpi";
			if (!extractRes("navitldpi", "/data/data/org.navitproject.navit/share/navit.xml"))
			{
				Log.e("Navit", "Failed to extract navit.xml for ldpi device(s)");
			}
		}
		else
		{
			/* default, meaning we just dont know what display this is */
			if (!extractRes("navit", "/data/data/org.navitproject.navit/share/navit.xml"))
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

		this.mgr = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
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
			boolean parsable = true;

			// if b: then remodel the input string to look like a:
			if (intent_data.substring(0, 20).equals("google.navigation:q="))
			{
				intent_data = "ll=" + intent_data.split("q=", -1)[1] + "&q=Target";
			}
			// if c: then remodel the input string to look like a:
			else if ((intent_data.substring(0, 21).equals("google.navigation:ll="))
					&& (intent_data.split("&q=").length == 0))
			{
				intent_data = intent_data + "&q=Target";
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
				parsable = false;
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
		menu.add(1, 1, 10, "Zoom in");
		menu.add(1, 2, 20, "Zoom out");

		menu.add(1, 3, 22, "download map");

		menu.add(1, 99, 30, "Exit Navit");
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
				Intent foo2 = new Intent(this, NavitDownloadSelectMapActivity.class);
				foo2.putExtra("title", "bla bla");
				foo2.putExtra("some stuff", "bla bla");
				this.startActivityForResult(foo2, Navit.NavitDownloaderSelectMap_id);
				break;
			case 99 :
				this.exit();
				//				msg = new Message();
				//				b = new Bundle();
				//				b.putInt("Callback", 5);
				//				b.putString("cmd", "quit()");
				//				msg.setData(b);
				//				N_NavitGraphics.callback_handler.sendMessage(msg);
				break;
		}
		return true;
	}


	protected void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		switch (requestCode)
		{
			case Navit.NavitDownloaderSelectMap_id :
				try
				{
					if (resultCode == Activity.RESULT_OK)
					{
						try
						{
							Log.d("Navit", "id=" + Integer.parseInt(data.getStringExtra("selected_id")));
							// set map id to download
							Navit.download_map_id = Integer.parseInt(data.getStringExtra("selected_id"));
							// show the map download progressbar, and download the map
							showDialog(Navit.MAPDOWNLOAD_DIALOG);
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
																break;
															case 1 :
																// change progressbar values
																mapdownloader_dialog.setMax(msg.getData().getInt(
																		"max"));
																mapdownloader_dialog.setProgress(msg.getData()
																		.getInt("cur"));
																mapdownloader_dialog.setTitle(msg.getData()
																		.getString("title"));
																mapdownloader_dialog.setMessage(msg.getData()
																		.getString("text"));
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
			case Navit.MAPDOWNLOAD_DIALOG :

				mapdownloader_dialog = new ProgressDialog(this);
				mapdownloader_dialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
				mapdownloader_dialog.setTitle("--");
				mapdownloader_dialog.setMessage("--");
				mapdownloader_dialog.setCancelable(true);
				mapdownloader_dialog.setProgress(0);
				mapdownloader_dialog.setMax(200);
				DialogInterface.OnDismissListener mOnDismissListener = new DialogInterface.OnDismissListener()
				{
					public void onDismiss(DialogInterface dialog)
					{
						Log.e("Navit", "onDismiss: mapdownloader_dialog");
						dialog.dismiss();
						dialog.cancel();
						progressThread.stop_thread();
					}
				};
				mapdownloader_dialog.setOnDismissListener(mOnDismissListener);
				mapdownloader = new NavitMapDownloader(this);
				//map_download.download_osm_map(NavitMapDownloader.austria);
				progressThread = mapdownloader.new ProgressThread(progress_handler,
						NavitMapDownloader.OSM_MAPS[Navit.download_map_id]);
				progressThread.start();
				return mapdownloader_dialog;
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
