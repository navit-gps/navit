package org.navitproject.navit;


import android.app.Activity;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.widget.Toast;

public class NavitDialogs extends Handler{

	public static Boolean             NavitAddressSearchSpinnerActive  = false;
	public static String              Navit_last_address_search_string = "";
	public static Boolean             Navit_last_address_partial_match = false;
	public static String              Navit_last_country = "";

	// Dialogs
	public static final int           DIALOG_MAPDOWNLOAD               = 1;
	public static final int           DIALOG_SEARCHRESULTS_WAIT        = 3;

	// dialog messages
	static final int MSG_REMOVE_PROGRESS_BAR = 0;
	static final int MSG_PROGRESS_BAR = 1;
	static final int MSG_TOAST = 2;
	static final int MSG_TOAST_LONG = 3;
	static final int MSG_SEARCH = 11;
	static final int MSG_PROGRESS_BAR_SEARCH = 21;
	static final int MSG_POSITION_MENU = 22;
	static final int MSG_START_MAP_DOWNLOAD =23;
	static Handler mHandler;

	private ProgressDialog                    mapdownloader_dialog     = null;
	private ProgressDialog                    search_results_wait      = null;
	private SearchResultsThread               searchresultsThread      = null;
	private SearchResultsThreadSpinnerThread  spinner_thread           = null;
	private NavitMapDownloader                mapdownloader            = null;
	
	private Activity mActivity;
	
	public NavitDialogs(Activity activity) {
		super();
		mActivity = activity;
		mHandler = this;
	}

	static public void sendDialogMessage(int what, String title, String text, int dialog_num, int value1, int value2)
	{
		Message msg = mHandler.obtainMessage(what);
		Bundle data = new Bundle();
		
		data.putString("title", title);
		data.putString("text", text); 
		data.putInt("value1", value1); 
		data.putInt("value2", value2);
		data.putInt("dialog_num", dialog_num);
		msg.setData(data);
		
		mHandler.sendMessage(msg);
	}


	@Override
	public void handleMessage(Message msg)
	{
		switch (msg.what)
		{
		case MSG_REMOVE_PROGRESS_BAR :
			// dismiss dialog, remove dialog
			mActivity.dismissDialog(DIALOG_MAPDOWNLOAD);
			mActivity.removeDialog(DIALOG_MAPDOWNLOAD);

			// exit_code=0 -> OK, map was downloaded fine
			if (msg.getData().getInt("value1") == 0)
			{
				// try to use the new downloaded map (works fine now!)
				Log.d("Navit", "instance count=" + Navit.getInstanceCount());
				//mActivity.onStop();
				//mActivity.onCreate(mActivity.getIntent().getExtras());
			}
			break;
		case MSG_PROGRESS_BAR :
			// change progressbar values
			mapdownloader_dialog.setMax(msg.getData().getInt("value1"));
			mapdownloader_dialog.setProgress(msg.getData().getInt("value2"));
			mapdownloader_dialog.setTitle(msg.getData().getString("title"));
			mapdownloader_dialog.setMessage(msg.getData().getString("text"));
			break;
		case MSG_TOAST :
			Toast.makeText(mActivity, msg.getData().getString("text"), Toast.LENGTH_SHORT).show();
			break;
		case MSG_TOAST_LONG :
			Toast.makeText(mActivity, msg.getData().getString("text"), Toast.LENGTH_LONG).show();
			break;
		case MSG_PROGRESS_BAR_SEARCH :
			// change values - generic
			int what_dialog_generic = msg.getData().getInt("dialog_num");
			if (what_dialog_generic == DIALOG_SEARCHRESULTS_WAIT)
			{
				search_results_wait.setMax(msg.getData().getInt("value1"));
				search_results_wait.setProgress(msg.getData().getInt("value2"));
				search_results_wait.setTitle(msg.getData().getString("title"));
				search_results_wait.setMessage(msg.getData().getString("text"));
			}
			break;
		case MSG_SEARCH :
			// show dialog - generic
			mActivity.showDialog(DIALOG_SEARCHRESULTS_WAIT);
			break;
		case MSG_START_MAP_DOWNLOAD:
		{
			int map_selected = msg.arg1;
			int map_slot     = msg.arg2;
			Log.d("Navit", "PRI id=" + map_selected);
			// set map id to download

			int download_map_id = NavitMapDownloader.OSM_MAP_NAME_ORIG_ID_LIST[map_selected];
			// show the map download progressbar, and download the map
			if (download_map_id > -1)
			{
				mActivity.showDialog(NavitDialogs.DIALOG_MAPDOWNLOAD);

				mapdownloader = new NavitMapDownloader(download_map_id
						, NavitDialogs.DIALOG_MAPDOWNLOAD, map_slot);
				mapdownloader.start();

			}
		}
		break;
			
		case 99 :
			// dismiss dialog, remove dialog - generic
			mActivity.dismissDialog(msg.getData().getInt("dialog_num"));
			mActivity.removeDialog(msg.getData().getInt("dialog_num"));
			break;
		}
	}

	Dialog createDialog(int id)
	{
		switch (id)
		{
			case DIALOG_SEARCHRESULTS_WAIT :
				search_results_wait = new ProgressDialog(mActivity);
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
					}
				};
				search_results_wait.setOnDismissListener(mOnDismissListener3);
				searchresultsThread = new SearchResultsThread(this, DIALOG_SEARCHRESULTS_WAIT);
				searchresultsThread.start();
	
				NavitAddressSearchSpinnerActive = true;
				spinner_thread = new SearchResultsThreadSpinnerThread(this, DIALOG_SEARCHRESULTS_WAIT);
				spinner_thread.start();
	
				return search_results_wait;
			case DIALOG_MAPDOWNLOAD :
				mapdownloader_dialog = new ProgressDialog(mActivity);
				mapdownloader_dialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
				mapdownloader_dialog.setTitle("--");
				mapdownloader_dialog.setMessage("--");
				mapdownloader_dialog.setCancelable(true);
				mapdownloader_dialog.setProgress(0);
				mapdownloader_dialog.setMax(200);
				DialogInterface.OnDismissListener mOnDismissListener1 = new DialogInterface.OnDismissListener()
				{
					public void onDismiss(DialogInterface dialog)
					{
						Log.e("Navit", "onDismiss: mapdownloader_dialog");
						dialog.dismiss();
						dialog.cancel();
						// todo: do better
						mapdownloader.stop_thread();
					}
				};
				mapdownloader_dialog.setOnDismissListener(mOnDismissListener1);
				// show license for OSM maps
				Toast.makeText(mActivity.getApplicationContext(),
						Navit.get_text("Map data (c) OpenStreetMap contributors, CC-BY-SA"),
						Toast.LENGTH_LONG).show(); //TRANS
				return mapdownloader_dialog;
		}
		// should never get here!!
		return null;
	}
	
	public class SearchResultsThreadSpinnerThread extends Thread
	{
		int             dialog_num;
		int             spinner_current_value;
		private Boolean running;
		Handler         mHandler;
		
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
				if (NavitAddressSearchSpinnerActive == false)
				{
					this.running = false;
				}
				else
				{
					Message msg = mHandler.obtainMessage();
					Bundle b = new Bundle();
					msg.what = MSG_PROGRESS_BAR_SEARCH;
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
		Handler				mHandler;
		int					my_dialog_num;

		SearchResultsThread(Handler h, int dialog_num)
		{
			this.mHandler = h;
			this.my_dialog_num = dialog_num;
			Log.e("Navit", "SearchResultsThread created");
		}

		public void run()
		{
			Log.e("Navit", "SearchResultsThread started");

			// initialize the dialog with sane values
			Message msg = mHandler.obtainMessage();
			Bundle b = new Bundle();
			msg.what = MSG_PROGRESS_BAR_SEARCH;
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
			Navit.N_NavitGraphics.CallbackSearchResultList(partial_match_i, Navit_last_country, Navit_last_address_search_string);
			Log.e("Navit", "SearchResultsThread run2");
			NavitAddressSearchSpinnerActive = false;

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

		public String filter_bad_chars(String in)
		{
			String out = in;
			out = out.replaceAll("\\n", " "); // newline -> space
			out = out.replaceAll("\\r", " "); // return -> space
			out = out.replaceAll("\\t", " "); // tab -> space
			return out;
		}
	}
	
	public void open_search_result_list()
	{
		// open result list
		Intent address_result_list_activity = new Intent(mActivity, NavitAddressResultListActivity.class);
		mActivity.startActivityForResult(address_result_list_activity, Navit.NavitAddressResultList_id);
	}
}
