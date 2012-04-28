package org.navitproject.navit;


import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
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
	static final int MSG_MAP_DOWNLOAD_FINISHED   = 0;
	static final int MSG_PROGRESS_BAR          = 1;
	static final int MSG_TOAST                 = 2;
	static final int MSG_TOAST_LONG            = 3;
	static final int MSG_SEARCH                = 4;
	static final int MSG_PROGRESS_BAR_SEARCH   = 5;
	static final int MSG_POSITION_MENU         = 6;
	static final int MSG_START_MAP_DOWNLOAD    = 7;
	static final int MSG_REMOVE_DIALOG_GENERIC = 99;
	static Handler mHandler;

	private ProgressDialog                    mapdownloader_dialog     = null;
	private ProgressDialog                    search_results_wait      = null;
	private SearchResultsThread               searchresultsThread      = null;
	private SearchResultsThreadSpinner        searchresultsSpinner     = null;
	private NavitMapDownloader                mapdownloader            = null;

	private Navit mActivity;

	public NavitDialogs(Navit activity) {
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
		case MSG_MAP_DOWNLOAD_FINISHED :
		{
			// dismiss dialog, remove dialog
			mActivity.dismissDialog(DIALOG_MAPDOWNLOAD);
			mActivity.removeDialog(DIALOG_MAPDOWNLOAD);
			Message activate_map_msg = Message.obtain(Navit.N_NavitGraphics.callback_handler, NavitGraphics.msg_type.CLB_LOAD_MAP.ordinal());
			activate_map_msg.setData(msg.getData());
			activate_map_msg.sendToTarget();
			break;
		}
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
			int download_map_id = msg.arg1;
			Log.d("Navit", "PRI id=" + download_map_id);
			// set map id to download

			// show the map download progressbar, and download the map
			if (download_map_id > -1)
			{
				mActivity.showDialog(NavitDialogs.DIALOG_MAPDOWNLOAD);

				mapdownloader = new NavitMapDownloader(download_map_id, NavitDialogs.DIALOG_MAPDOWNLOAD);
				mapdownloader.start();
			}
		}
		break;
		case MSG_REMOVE_DIALOG_GENERIC :
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
				searchresultsThread = new SearchResultsThread(this, DIALOG_SEARCHRESULTS_WAIT);
				searchresultsThread.start();

				NavitAddressSearchSpinnerActive = true;
				searchresultsSpinner = new SearchResultsThreadSpinner(this, DIALOG_SEARCHRESULTS_WAIT);
				post(searchresultsSpinner);

				return search_results_wait;
			case DIALOG_MAPDOWNLOAD :
				mapdownloader_dialog = new ProgressDialog(mActivity);
				mapdownloader_dialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
				mapdownloader_dialog.setTitle("--");
				mapdownloader_dialog.setMessage("--");
				mapdownloader_dialog.setCancelable(true);
				mapdownloader_dialog.setProgress(0);
				mapdownloader_dialog.setMax(200);
				DialogInterface.OnDismissListener onDismissListener = new DialogInterface.OnDismissListener()
				{
					public void onDismiss(DialogInterface dialog)
					{
						Log.e("Navit", "onDismiss: mapdownloader_dialog");
						mapdownloader.stop_thread();
					}
				};
				mapdownloader_dialog.setOnDismissListener(onDismissListener);
				// show license for OSM maps
				Toast.makeText(mActivity.getApplicationContext(),
						Navit.get_text("Map data (c) OpenStreetMap contributors, CC-BY-SA"),
						Toast.LENGTH_LONG).show(); //TRANS
				return mapdownloader_dialog;
		}
		// should never get here!!
		return null;
	}

	public class SearchResultsThreadSpinner implements Runnable
	{
		int             dialog_num;
		int             spinner_current_value;

		SearchResultsThreadSpinner(Handler h, int dialog_num)
		{
			this.dialog_num = dialog_num;
			this.spinner_current_value = 0;
			Log.e("Navit", "SearchResultsThreadSpinnerThread created");
		}
		public void run()
		{
			if ( NavitAddressSearchSpinnerActive ) {
				
				sendDialogMessage( MSG_PROGRESS_BAR_SEARCH
				                 , Navit.get_text("getting search results")
				                 , Navit.get_text("searching ...")
				                 , dialog_num
				                 , Navit.ADDRESS_RESULTS_DIALOG_MAX
				                 , spinner_current_value % (Navit.ADDRESS_RESULTS_DIALOG_MAX + 1));
				
				spinner_current_value++;
				postDelayed(this, 700);
			}
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
			Message msg;
			Bundle  bundle;
			// initialize the dialog with sane values
			sendDialogMessage( MSG_PROGRESS_BAR_SEARCH
			                 , Navit.get_text("getting search results")
			                 , Navit.get_text("searching ...")
			                 , my_dialog_num
			                 , Navit.ADDRESS_RESULTS_DIALOG_MAX
			                 , 0);

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
				msg = mHandler.obtainMessage(MSG_TOAST);
				bundle = new Bundle();
				bundle.putString("text", Navit.get_text("No Results found!")); //TRANS
				msg.setData(bundle);
				mHandler.sendMessage(msg);
			}

			// ok, remove dialog
			msg = mHandler.obtainMessage(MSG_REMOVE_DIALOG_GENERIC);
			bundle = new Bundle();
			bundle.putInt("dialog_num", this.my_dialog_num);
			msg.setData(bundle);
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
