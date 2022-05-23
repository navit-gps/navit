package org.navitproject.navit;

import static org.navitproject.navit.NavitAppConfig.getTstring;

import android.app.ProgressDialog;
import android.content.Context;
import android.os.AsyncTask;
import android.os.Environment;
import android.text.format.Time;
import android.widget.Toast;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;


public class NavitBackupTask extends AsyncTask<Void, Void, String> {

    private final Navit mActivity;

    private ProgressDialog mDialog;

    NavitBackupTask(Navit context) {
        mActivity = context;
    }

    @Override
    protected void onPreExecute() {
        super.onPreExecute();

        /* Create a Wait Progress Dialog to inform the User that we are working */
        mDialog = new ProgressDialog(mActivity);
        mDialog.setIndeterminate(true);
        mDialog.setMessage(getTstring(R.string.backing_up));
        mDialog.show();
    }

    @Override
    protected String doInBackground(Void... v) {
        Time now = new Time();
        now.setToNow();

        /* This is the Directory where all Subdirectories are stored by date */
        File mainBackupDir = new File(
                Environment.getExternalStorageDirectory().getPath() + "/navit/backup/");

        /* Create the Main Backup Directory if it doesn't exist */
        if (!mainBackupDir.isDirectory()) {
            if (!mainBackupDir.mkdirs()) {
                return getTstring(R.string.failed_to_create_backup_directory);
            }
        }

        /* Create a Timestamp in the format YYYY-MM-DD-Index */
        String timestamp = now.year + "-" + String.format("%02d", now.month + 1) + "-" + String
                .format("%02d", now.monthDay);
        /* Get the next free index */
        int index = 1;
        for (String s : mainBackupDir.list()) {
            if (s.contains(timestamp)) {
                int newIndex = Integer.parseInt(s.substring(11));
                if (newIndex >= index) {
                    index = newIndex + 1;
                }
            }
        }
        timestamp += "-" + index;

        /* This is the Directory in which the Files are copied into */
        File backupDir = new File(
                Environment.getExternalStorageDirectory().getPath() + "/navit/backup/" + timestamp);

        /* Create the Backup Directory if it doesn't exist */
        if (!backupDir.isDirectory()) {
            if (!backupDir.mkdirs()) {
                return getTstring(R.string.failed_to_create_backup_directory);
            }
        }

        ObjectOutputStream preferencesOOs = null;
        try {
            /* Backup Files in home */
            NavitUtils.copyFileIfExists(Navit.sMapFilenamePath + "/home/bookmark.txt",
                    backupDir.getPath() + "/bookmark.txt");
            NavitUtils.copyFileIfExists(Navit.sMapFilenamePath + "/home/destination.txt",
                    backupDir.getPath() + "/destination.txt");
            NavitUtils.copyFileIfExists(Navit.sMapFilenamePath + "/home/gui_internal.txt",
                    backupDir.getPath() + "/gui_internal.txt");

            /* Backup Shared Preferences */
            preferencesOOs = new ObjectOutputStream(
                    new FileOutputStream(backupDir.getPath() + "/preferences.bak"));
            preferencesOOs.writeObject(
                    mActivity.getSharedPreferences(NavitAppConfig.NAVIT_PREFS, Context.MODE_PRIVATE)
                    .getAll());
        } catch (IOException e) {
            e.printStackTrace();
            return getTstring(R.string.backup_failed);
        } finally {
            /* Close Stream to prevent Resource Leaks */
            try {
                if (preferencesOOs != null) {
                    preferencesOOs.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
                return getTstring(R.string.backup_failed);
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

        Toast.makeText(mActivity, getTstring(R.string.backup_successful),
                Toast.LENGTH_LONG).show();
    }

    @Override
    protected void onCancelled() {
        super.onCancelled();
        Toast.makeText(mActivity, getTstring(R.string.backup_failed), Toast.LENGTH_LONG)
            .show();
        mDialog.dismiss();
    }
}
