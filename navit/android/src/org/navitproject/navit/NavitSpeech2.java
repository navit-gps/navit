/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

package org.navitproject.navit;

import android.content.Intent;
import android.speech.tts.TextToSpeech;
import android.util.Log;


public class NavitSpeech2 implements TextToSpeech.OnInitListener, NavitActivityResult {
	private TextToSpeech mTts;
	private Navit navit;
	int MY_DATA_CHECK_CODE=1;


	public void onInit(int status)
	{
		Log.e("NavitSpeech2","Status "+status);
	}

	public void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		Log.e("NavitSpeech2","onActivityResult "+requestCode+" "+resultCode);
		if (requestCode == MY_DATA_CHECK_CODE) {
			if (resultCode == TextToSpeech.Engine.CHECK_VOICE_DATA_PASS) {
				// success, create the TTS instance
				mTts = new TextToSpeech(navit, this);
			} else {
				// missing data, install it
				Intent installIntent = new Intent();
				installIntent.setAction(TextToSpeech.Engine.ACTION_INSTALL_TTS_DATA);
				navit.startActivity(installIntent);
			}
		}
	}

	NavitSpeech2(Navit navit)
	{
		this.navit=navit;
		navit.setActivityResult(1, this);
		Log.e("NavitSpeech2","Create");
		Intent checkIntent = new Intent();
		checkIntent.setAction(TextToSpeech.Engine.ACTION_CHECK_TTS_DATA);
		navit.startActivityForResult(checkIntent, MY_DATA_CHECK_CODE);
	}
	public void say(String what)
	{
		if (mTts != null) {
			mTts.speak(what, TextToSpeech.QUEUE_FLUSH, null);
		}
	}
}

