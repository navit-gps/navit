package org.navitproject.navit;

import static org.navitproject.navit.NavitAppConfig.getTstring;

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

    private final Navit mActivity;
    private ProgressDialog mDialog;
    private final String mTimestamp;

    NavitRestoreTask(Navit context, String timestamp) {
        mActivity = context;
        mTimestamp = timestamp;
    }

    @Override
    protected void onPreExecute() {
        super.onPreExecute();

        /* Create a Wait Progress Dialog to inform the User that we are working */
        mDialog = new ProgressDialog(mActivity);
        mDialog.setIndeterminate(true);
        mDialog.setMessage(getTstring(R.string.restoring));
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
            return getTstring(R.string.backup_not_found);
        }

        ObjectInputStream preferenceOis = null;
        try {
            /* Delete all old Files in Home */
            NavitUtils.removeFileIfExists(Navit.sMapFilenamePath + "/home/bookmark.txt");
            NavitUtils.removeFileIfExists(Navit.sMapFilenamePath + "/home/destination.txt");
            NavitUtils.removeFileIfExists(Navit.sMapFilenamePath + "/home/gui_internal.txt");


            /* Restore Files in home */
            NavitUtils.copyFileIfExists(backupDir.getPath() + "/bookmark.txt",
                    Navit.sMapFilenamePath + "/home/bookmark.txt");
            NavitUtils.copyFileIfExists(backupDir.getPath() + "/destination.txt",
                    Navit.sMapFilenamePath + "/home/destination.txt");
            NavitUtils.copyFileIfExists(backupDir.getPath() + "/gui_internal.txt",
                    Navit.sMapFilenamePath + "/home/gui_internal.txt");

            /* Restore Shared Preferences */
            preferenceOis = new ObjectInputStream(
                    new FileInputStream(backupDir.getPath() + "/preferences.bak"));
            Map<String, ?> entries = (Map<String, ?>) preferenceOis.readObject();

            Editor prefEditor = mActivity.getSharedPreferences(NavitAppConfig.NAVIT_PREFS, Context.MODE_PRIVATE).edit();

            /* Remove all old Preferences */
            prefEditor.clear();

            /* Iterate through all Entries and add them to our Preferences */
            for (Entry<String, ?> entry : entries.entrySet()) {
                Object value = entry.getValue();
                String key = entry.getKey();

                if (value instanceof Boolean) {
                    prefEditor.putBoolean(key, (Boolean) value);
                } else if (value instanceof Float) {
                    prefEditor.putFloat(key, (Float) value);
                } else if (value instanceof Integer) {
                    prefEditor.putInt(key, (Integer) value);
                } else if (value instanceof Long) {
                    prefEditor.putLong(key, (Long) value);
                } else if (value instanceof String) {
                    prefEditor.putString(key, (String) value);
                }
            }

            if (!prefEditor.commit()) {
                return getTstring(R.string.failed_to_restore);
            }

        } catch (Exception e) {
            e.printStackTrace();
            return getTstring(R.string.failed_to_restore);
        } finally {
            try {
                /* Close Stream to prevent Resource leak */
                if (preferenceOis != null) {
                    preferenceOis.close();
                }
            } catch (IOException e) {
                // Catching but ignoring that exception when closing the stream
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
                getTstring(R.string.restore_successful_please_restart_navit),
                Toast.LENGTH_LONG).show();
        NotificationManager nm = (NotificationManager) mActivity.getSystemService(Context.NOTIFICATION_SERVICE);
        nm.cancel(R.string.app_name);
        mActivity.finish();
    }

    @Override
    protected void onCancelled() {
        super.onCancelled();
        Toast.makeText(mActivity, getTstring(R.string.restore_failed), Toast.LENGTH_LONG)
            .show();
        mDialog.dismiss();
    }
}
