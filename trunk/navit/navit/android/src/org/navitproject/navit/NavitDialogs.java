package org.navitproject.navit;


import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.widget.Toast;

public class NavitDialogs extends Handler{
	// Dialogs
	public static final int           DIALOG_MAPDOWNLOAD               = 1;

	// dialog messages
	static final int MSG_MAP_DOWNLOAD_FINISHED   = 0;
	static final int MSG_PROGRESS_BAR          = 1;
	static final int MSG_TOAST                 = 2;
	static final int MSG_TOAST_LONG            = 3;
	static final int MSG_POSITION_MENU         = 6;
	static final int MSG_START_MAP_DOWNLOAD    = 7;
	static final int MSG_REMOVE_DIALOG_GENERIC = 99;
	static Handler mHandler;

	private ProgressDialog                    mapdownloader_dialog     = null;
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
		case MSG_START_MAP_DOWNLOAD:
		{
			int download_map_id = msg.arg1;
			Log.d("Navit", "PRI id=" + download_map_id);
			// set map id to download

			// show the map download progressbar, and download the map
			if (download_map_id > -1)
			{
				mapdownloader = new NavitMapDownloader(download_map_id);
				mActivity.showDialog(NavitDialogs.DIALOG_MAPDOWNLOAD);
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
}
