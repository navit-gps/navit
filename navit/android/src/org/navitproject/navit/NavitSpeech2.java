/*
 * Navit, a modular navigation system. Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if
 * not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

package org.navitproject.navit;

import static org.navitproject.navit.NavitAppConfig.getTstring;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.speech.tts.TextToSpeech;
import android.util.Log;




@SuppressWarnings("unused")
class NavitSpeech2 implements TextToSpeech.OnInitListener, NavitActivityResult {

    private final Navit mNavit;
    private static final int MY_DATA_CHECK_CODE = 1;
    private static final String TAG = "NavitSpeech2";
    private TextToSpeech mTts;


    NavitSpeech2(Navit navit) {
        this.mNavit = navit;
        navit.setActivityResult(1, this);
        Log.d(TAG, "Create");
        Intent checkIntent = new Intent();
        checkIntent.setAction(TextToSpeech.Engine.ACTION_CHECK_TTS_DATA);
        if (navit.getPackageManager()
                .resolveActivity(checkIntent, PackageManager.MATCH_DEFAULT_ONLY) != null) {
            Log.d(TAG, "ACTION_CHECK_TTS_DATA available");
            navit.startActivityForResult(checkIntent, MY_DATA_CHECK_CODE);
        } else {
            Log.e(TAG, "ACTION_CHECK_TTS_DATA not available, assume tts is working");
            mTts = new TextToSpeech(navit.getApplication(), this);
        }
    }

    public void onInit(int status) {
        Log.d(TAG, "Status " + status);
    }

    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        Log.d(TAG, "onActivityResult " + requestCode + " " + resultCode);
        if (requestCode == MY_DATA_CHECK_CODE) {
            if (resultCode == TextToSpeech.Engine.CHECK_VOICE_DATA_PASS) {
                // success, create the TTS instance
                mTts = new TextToSpeech(mNavit.getApplication(), this);
            } else {
                // missing data, ask to install it
                AlertDialog.Builder builder = new AlertDialog.Builder(mNavit);
                builder
                    .setTitle(getTstring(R.string.TTS_title_data_missing))
                    .setMessage(getTstring(R.string.TTS_qery_install_data))
                    .setPositiveButton(getTstring(R.string.yes),
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    Intent installIntent = new Intent();
                                    installIntent.setAction(
                                            TextToSpeech.Engine.ACTION_INSTALL_TTS_DATA);
                                    mNavit.startActivity(installIntent);
                                }
                            })
                .setNegativeButton(getTstring(R.string.no), null)
                    .show();
            }
        }
    }

    public void say(String what) {
        if (mTts != null) {
            mTts.speak(what, TextToSpeech.QUEUE_FLUSH, null);
        }
    }
}

