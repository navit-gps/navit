package org.navitproject.navit;

import android.app.NotificationManager;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.SharedPreferences.Editor;
import android.os.AsyncTask;
import android.os.Environment;
import android.widget.Toast;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.util.Map;
import java.util.Map.Entry;

public class NavitRestoreTask extends AsyncTask<Void, Void, String> {

    private Navit mActivity;

    private ProgressDialog mDialog;

    private String mTimestamp;

    public NavitRestoreTask(Navit context, String timestamp) {
        mActivity = context;
        mTimestamp = timestamp;
    }

    @Override
    protected void onPreExecute() {
        super.onPreExecute();

        /* Create a Wait Progress Dialog to inform the User that we are working */
        mDialog = new ProgressDialog(mActivity);
        mDialog.setIndeterminate(true);
        mDialog.setMessage(mActivity.getTstring(R.string.restoring));
        mDialog.show();
    }

    @SuppressWarnings("unchecked")
    @Override
    protected String doInBackground(Void... v) {

        /* This is the Directory where all Subdirectories are stored by date */
        File backupDir = new File(
            Environment.getExternalStorageDirectory().getPath() + "/navit/backup/"
            + mTimestamp);

        /* Check if there is a Backup Directory */
        if (!backupDir.isDirectory()) {
            return mActivity.getTstring(R.string.backup_not_found);
        }

        ObjectInputStream preferenceOIS = null;
        try {
            /* Delete all old Files in Home */
            mActivity.removeFileIfExists(Navit.NAVIT_DATA_DIR + "/home/bookmark.txt");
            mActivity.removeFileIfExists(Navit.NAVIT_DATA_DIR + "/home/destination.txt");
            mActivity.removeFileIfExists(Navit.NAVIT_DATA_DIR + "/home/gui_internal.txt");


            /* Restore Files in home */
            mActivity.copyFileIfExists(backupDir.getPath() + "/bookmark.txt",
                                       Navit.NAVIT_DATA_DIR + "/home/bookmark.txt");
            mActivity.copyFileIfExists(backupDir.getPath() + "/destination.txt",
                                       Navit.NAVIT_DATA_DIR + "/home/destination.txt");
            mActivity.copyFileIfExists(backupDir.getPath() + "/gui_internal.txt",
                                       Navit.NAVIT_DATA_DIR + "/home/gui_internal.txt");

            /* Restore Shared Preferences */
            preferenceOIS = new ObjectInputStream(
                new FileInputStream(backupDir.getPath() + "/preferences.bak"));
            Map<String, ?> entries = (Map<String, ?>) preferenceOIS.readObject();

            Editor prefEditor = mActivity
                                .getSharedPreferences(Navit.NAVIT_PREFS, Context.MODE_PRIVATE).edit();

            /* Remove all old Preferences */
            prefEditor.clear();

            /* Iterate through all Entries and add them to our Preferences */
            for (Entry<String, ?> entry : entries.entrySet()) {
                Object value = entry.getValue();
                String key = entry.getKey();

                if (value instanceof Boolean) {
                    prefEditor.putBoolean(key, ((Boolean) value).booleanValue());
                } else if (value instanceof Float) {
                    prefEditor.putFloat(key, ((Float) value).floatValue());
                } else if (value instanceof Integer) {
                    prefEditor.putInt(key, ((Integer) value).intValue());
                } else if (value instanceof Long) {
                    prefEditor.putLong(key, ((Long) value).longValue());
                } else if (value instanceof String) {
                    prefEditor.putString(key, (String) value);
                }
            }

            if (!prefEditor.commit()) {
                return mActivity.getTstring(R.string.failed_to_restore);
            }

        } catch (Exception e) {
            e.printStackTrace();
            return mActivity.getTstring(R.string.failed_to_restore);
        } finally {
            try {
                /* Close Stream to prevent Resource leak */
                if (preferenceOIS != null) {
                    preferenceOIS.close();
                }
            } catch (IOException e) {

            }
        }

        return null;
    }

    @Override
    protected void onPostExecute(String result) {
        super.onPostExecute(result);

        /* Dismiss the Wait Progress Dialog */
        mDialog.dismiss();

        /* If result is non null an Error occured */
        if (result != null) {
            Toast.makeText(mActivity, result, Toast.LENGTH_LONG).show();
            return;
        }

        /* Navit needs to be restarted. Currently the User has to restart it by himself */
        Toast.makeText(mActivity,
                       mActivity.getTstring(R.string.restore_successful_please_restart_navit),
                       Toast.LENGTH_LONG).show();
        NotificationManager nm = (NotificationManager) mActivity
                                 .getSystemService(Context.NOTIFICATION_SERVICE);
        nm.cancel(R.string.app_name);
        NavitVehicle.removeListener();
        mActivity.finish();
    }

    @Override
    protected void onCancelled() {
        super.onCancelled();
        Toast.makeText(mActivity, mActivity.getTstring(R.string.restore_failed), Toast.LENGTH_LONG)
        .show();
        mDialog.dismiss();
    }
}
