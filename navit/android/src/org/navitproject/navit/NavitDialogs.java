package org.navitproject.navit;

import static org.navitproject.navit.NavitAppConfig.getTstring;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.widget.ArrayAdapter;
import android.widget.Toast;
import java.io.File;


public class NavitDialogs extends Handler {

    // Dialogs
    static final int DIALOG_MAPDOWNLOAD = 1;
    static final int DIALOG_BACKUP_RESTORE = 2;
    // dialog messages
    static final int MSG_MAP_DOWNLOAD_FINISHED = 0;
    static final int MSG_PROGRESS_BAR = 1;
    static final int MSG_TOAST = 2;
    static final int MSG_TOAST_LONG = 3;
    static final int MSG_START_MAP_DOWNLOAD = 7;
    private static final int DIALOG_SELECT_BACKUP = 3;
    private static final int MSG_REMOVE_DIALOG_GENERIC = 99;
    private static Handler sHandler;
    private static final String TAG = "NavitDialogs";
    private ProgressDialog mMapdownloaderDialog = null;
    private NavitMapDownloader mMapdownloader = null;
    private final Navit mActivity;

    NavitDialogs(Navit activity) {
        super();
        mActivity = activity;
        sHandler = this;
    }

    static void sendDialogMessage(int what, String title, String text, int dialogNum,
            int value1, int value2) {
        Bundle data = new Bundle();
        data.putString("title", title);
        data.putString("text", text);
        data.putInt("value1", value1);
        data.putInt("value2", value2);
        data.putInt("dialog_num", dialogNum);
        Message msg = sHandler.obtainMessage(what);
        msg.setData(data);

        sHandler.sendMessage(msg);
    }

    @Override
    public void handleMessage(Message msg) {
        switch (msg.what) {
            case MSG_MAP_DOWNLOAD_FINISHED: {
                // dismiss dialog, remove dialog
                mActivity.dismissDialog(DIALOG_MAPDOWNLOAD);
                mActivity.removeDialog(DIALOG_MAPDOWNLOAD);
                if (msg.getData().getInt("value1") == 1) {
                    Message msgOut = Message.obtain(NavitGraphics.sCallbackHandler,
                                NavitGraphics.MsgType.CLB_LOAD_MAP.ordinal());
                    msgOut.setData(msg.getData());
                    msgOut.sendToTarget();

                    msgOut = Message
                        .obtain(NavitGraphics.sCallbackHandler,
                                NavitGraphics.MsgType.CLB_CALL_CMD.ordinal());
                    Bundle b = new Bundle();
                    int mi = msg.getData().getInt("value2");
                    double lon = (Double.parseDouble(NavitMapDownloader.osm_maps[mi].mLon1) + Double
                            .parseDouble(NavitMapDownloader.osm_maps[mi].mLon2)) / 2.0;
                    double lat = (Double.parseDouble(NavitMapDownloader.osm_maps[mi].mLat1) + Double
                            .parseDouble(NavitMapDownloader.osm_maps[mi].mLat2)) / 2.0;
                    b.putString("cmd", "set_center(\"" + lon + " " + lat + "\",1); zoom=256");
                    msgOut.setData(b);
                    msgOut.sendToTarget();
                }
                break;
            }
            case MSG_PROGRESS_BAR:
                // change progressbar values
                mMapdownloaderDialog.setMax(msg.getData().getInt("value1"));
                mMapdownloaderDialog.setProgress(msg.getData().getInt("value2"));
                mMapdownloaderDialog.setTitle(msg.getData().getString(("title")));
                mMapdownloaderDialog.setMessage(msg.getData().getString(("text")));
                break;
            case MSG_TOAST:
                Toast.makeText(mActivity, msg.getData().getString(("text")), Toast.LENGTH_SHORT).show();
                break;
            case MSG_TOAST_LONG:
                Toast.makeText(mActivity, msg.getData().getString(("text")), Toast.LENGTH_LONG).show();
                break;
            case MSG_START_MAP_DOWNLOAD: {
                int downloadMapId = msg.arg1;
                Log.d(TAG, "PRI id=" + downloadMapId);
                // set map id to download
                // show the map download progressbar, and download the map
                if (downloadMapId > -1) {
                    mMapdownloader = new NavitMapDownloader(downloadMapId);
                    mActivity.showDialog(NavitDialogs.DIALOG_MAPDOWNLOAD);
                    mMapdownloader.start();
                }
            }
                break;
            case MSG_REMOVE_DIALOG_GENERIC:
                // dismiss dialog, remove dialog - generic
                mActivity.dismissDialog(msg.getData().getInt("dialog_num"));
                mActivity.removeDialog(msg.getData().getInt("dialog_num"));
                break;
            default:
                Log.e(TAG,"Unexpected value: " + msg.what);
        }
    }

    Dialog createDialog(int id) {
        AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);

        switch (id) {
            case DIALOG_MAPDOWNLOAD:
                mMapdownloaderDialog = new ProgressDialog(mActivity);
                mMapdownloaderDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
                mMapdownloaderDialog.setTitle("--");
                mMapdownloaderDialog.setMessage("--");
                mMapdownloaderDialog.setCancelable(true);
                mMapdownloaderDialog.setProgress(0);
                mMapdownloaderDialog.setMax(200);
                DialogInterface.OnDismissListener onDismissListener = new DialogInterface.OnDismissListener() {
                    public void onDismiss(DialogInterface dialog) {
                        Log.e(TAG, "onDismiss: mMapdownloaderDialog");
                        if (mMapdownloader != null) {
                            mMapdownloader.stop_thread();
                        }
                    }
                };
                mMapdownloaderDialog.setOnDismissListener(onDismissListener);
                // show license for OSM maps
                Toast.makeText(mActivity.getApplicationContext(),
                        R.string.osm_copyright, Toast.LENGTH_LONG).show();
                return mMapdownloaderDialog;

            case DIALOG_BACKUP_RESTORE:
                /* Create a Dialog that Displays Options wether to Backup or Restore */
                builder.setTitle(getTstring(R.string.choose_an_action))
                        .setCancelable(true)
                        .setItems(R.array.dialog_backup_restore_items,
                            new OnClickListener() {

                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    /* Notify User if no SD Card present */
                                    if (!Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
                                        Toast.makeText(mActivity, getTstring(R.string.please_insert_an_sd_card),
                                                Toast.LENGTH_LONG).show();
                                    }
                                    if (which == 0) { /* Backup */
                                        new NavitBackupTask(mActivity).execute();
                                    } else if (which == 1) { /* Restore */
                                        mActivity.showDialog(DIALOG_SELECT_BACKUP);
                                    }
                                }
                            });
                return builder.create();

            case DIALOG_SELECT_BACKUP:
                /* Create a Dialog with a list from which the user selects the Backup to be restored */
                File mainBackupDir = new File(
                        Environment.getExternalStorageDirectory().getPath() + "/navit/backup/");

                String[] backups = null;
                if (mainBackupDir.isDirectory()) {
                    backups = mainBackupDir.list();
                }

                if (backups == null || backups.length == 0) {
                    /* No Backups were found */
                    builder.setTitle(getTstring(R.string.no_backup_found));
                    builder.setNegativeButton(getTstring(android.R.string.cancel), null);
                    return builder.create();
                }

                builder.setTitle(getTstring(R.string.select_backup));
                final ArrayAdapter<String> adapter = new ArrayAdapter<>(mActivity,
                        android.R.layout.simple_spinner_item, backups);
                builder.setAdapter(adapter, new OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        new NavitRestoreTask(mActivity, adapter.getItem(which)).execute();
                    }
                });
                builder.setNegativeButton(getTstring(android.R.string.cancel), null);

                return builder.create();
            default:
                Log.e(TAG,"Unexpected value: " + id);
        }
        // should never get here!!
        return null;
    }

    void prepareDialog(int id) {

        /* Remove the Dialog to force Android to rerun onCreateDialog */
        if (id == DIALOG_SELECT_BACKUP) {
            mActivity.removeDialog(id);
        }
    }
}
