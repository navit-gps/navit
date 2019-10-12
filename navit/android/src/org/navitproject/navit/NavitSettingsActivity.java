package org.navitproject.navit;

import android.os.Build;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.util.Log;
import java.io.File;



public class NavitSettingsActivity extends PreferenceActivity {

    private static final String TAG = "Settings";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.e(TAG,"onCreate");
        PreferenceManager prefMgr = getPreferenceManager();
        prefMgr.setSharedPreferencesName("NavitPrefs");
        prefMgr.setSharedPreferencesMode(MODE_PRIVATE);
        setPreferenceScreen(createPreferenceHierarchy());
    }

    private PreferenceScreen createPreferenceHierarchy() {
        Log.e(TAG,"onCreateHierarchy");
        PreferenceScreen root = getPreferenceManager().createPreferenceScreen(this);

        if (android.os.Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            ListPreference listPref = new ListPreference(this);
            File[] candidateDirs = getExternalFilesDirs(null);
            CharSequence[] entries = new CharSequence[candidateDirs.length];
            CharSequence[] entryValues = new CharSequence[candidateDirs.length];
            for (int i = 0; i < candidateDirs.length; i++) {
                File candidateDir = candidateDirs[i];
                entries[i] = candidateDir.toString(); // entries is the human readable form
                entryValues[i] = entries[i];
                Log.e(TAG,"candidate Dir ");
            }
            listPref.setEntries(entries);
            listPref.setEntryValues(entryValues);
            listPref.setDialogTitle("map location");
            listPref.setKey("filenamePath");
            listPref.setTitle("map location");
            listPref.setSummary("choose where the maps will be stored");
            root.addPreference(listPref);
        }
        return root;
    }
}
