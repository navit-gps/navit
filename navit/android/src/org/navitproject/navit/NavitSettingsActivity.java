package org.navitproject.navit;

import android.os.Build;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.EditTextPreference;
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
        final PreferenceScreen root = getPreferenceManager().createPreferenceScreen(this);

        final CheckBoxPreference checkboxPref = new CheckBoxPreference(this);
        checkboxPref.setTitle("Unlock developer options");
        checkboxPref.setEnabled(false);
        checkboxPref.setSummary("placeholder for an upcoming setting");
        root.addPreference(checkboxPref);

        final CheckBoxPreference checkboxPref2 = new CheckBoxPreference(this);
        checkboxPref2.setTitle("Unlock large map download");
        checkboxPref2.setEnabled(false);
        checkboxPref2.setSummary("placeholder for an upcoming setting");
        root.addPreference(checkboxPref2);

        final EditTextPreference customMapserver = new EditTextPreference(this);
        customMapserver.setEnabled(false);
        customMapserver.setTitle("custom mapserver");
        customMapserver.setSummary("upcoming option to select a local share as mapserver");
        root.addPreference(customMapserver);

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
