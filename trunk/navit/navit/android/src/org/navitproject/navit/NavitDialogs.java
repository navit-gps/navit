package org.navitproject.navit;


import java.io.File;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.widget.ArrayAdapter;
import android.widget.Toast;
import android.os.Environment;

public class NavitDialogs extends Handler{
	// Dialogs
	public static final int           DIALOG_MAPDOWNLOAD               = 1;
	public static final int           DIALOG_BACKUP_RESTORE            = 2;
	public static final int           DIALOG_SELECT_BACKUP             = 3;
	
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
			if (msg.getData().getInt("value1") == 1) {
				Message activate_map_msg =
				        Message.obtain(Navit.N_NavitGraphics.callback_handler,
				                NavitGraphics.msg_type.CLB_LOAD_MAP.ordinal());
				activate_map_msg.setData(msg.getData());
				activate_map_msg.sendToTarget();
			}
			break;
		}
		case MSG_PROGRESS_BAR :
			// change progressbar values
			mapdownloader_dialog.setMax(msg.getData().getInt("value1"));
			mapdownloader_dialog.setProgress(msg.getData().getInt("value2"));
			mapdownloader_dialog.setTitle(msg.getData().getString(("title")));
			mapdownloader_dialog.setMessage(msg.getData().getString(("text")));
			break;
		case MSG_TOAST :
			Toast.makeText(mActivity, msg.getData().getString(("text")), Toast.LENGTH_SHORT).show();
			break;
		case MSG_TOAST_LONG :
			Toast.makeText(mActivity, msg.getData().getString(("text")), Toast.LENGTH_LONG).show();
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
	    AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
	    
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
						Navit._("Map data (c) OpenStreetMap contributors, ODBL"),
						Toast.LENGTH_LONG).show(); //TRANS
				return mapdownloader_dialog;
				
			case DIALOG_BACKUP_RESTORE :
			    /* Create a Dialog that Displays Options wether to Backup or Restore */
			    builder.setTitle(mActivity.getString(R.string.choose_an_action)).
			        setCancelable(true).
			        setItems(R.array.dialog_backup_restore_items,
			                new DialogInterface.OnClickListener() {
			            
			            @Override
			            public void onClick(DialogInterface dialog, int which) {
			                /* Notify User if no SD Card present */
			                if(!Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED))
			                    Toast.makeText(mActivity,mActivity.getString(R.string.please_insert_an_sd_card), Toast.LENGTH_LONG).show();
			                
			                switch (which) {
			                    case 0:
			                        /* Backup */
			                        new NavitBackupTask(mActivity).execute();
			                        break;
			                    case 1:
			                        /* Restore */
			                        mActivity.showDialog(DIALOG_SELECT_BACKUP);
			                        break;
			                }
			            }});
			            return builder.create();
			            
			case DIALOG_SELECT_BACKUP :
			    /* Create a Dialog with a list from which the user selects the Backup to be restored */
			    File mainBackupDir = new File(Environment.getExternalStorageDirectory().getPath() + "/navit/backup/");
			    
			    String[] backups = null;
			    if(mainBackupDir.isDirectory())
			        backups = mainBackupDir.list();

			    if(backups == null || backups.length == 0) {
			        /* No Backups were found */
			        builder.setTitle(mActivity.getText(R.string.no_backup_found));
			        builder.setNegativeButton(mActivity.getText(android.R.string.cancel), null);
			        return builder.create();
			    }
			    
			    builder.setTitle(mActivity.getString(R.string.select_backup));
			    final ArrayAdapter<String> adapter = new ArrayAdapter<String>(mActivity, android.R.layout.simple_spinner_item, backups);			    
			    builder.setAdapter(adapter, new OnClickListener(){

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        new NavitRestoreTask(mActivity, adapter.getItem(which)).execute();
                    }});
                builder.setNegativeButton(mActivity.getString(android.R.string.cancel), null);

                return builder.create();
		}
		// should never get here!!
		return null;
	}

    public void prepareDialog(int id, Dialog dialog) {
        
        /* Remove the Dialog to force Android to rerun onCreateDialog */
        if(id == DIALOG_SELECT_BACKUP)
            mActivity.removeDialog(id);
    }
}
