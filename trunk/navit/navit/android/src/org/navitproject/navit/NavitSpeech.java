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

import java.lang.Thread;
import android.app.Activity;
import android.widget.TextView;
import android.os.Bundle;
import android.os.Debug;
import android.os.Message;
import android.os.Handler;
import android.content.Context;
import android.util.Log;
import com.google.tts.TTS;


public class NavitSpeech implements Runnable {
	private TTS tts;
    	private TTS.InitListener ttsInitListener;
	private String what;
	private Thread thread;

	NavitSpeech(Navit navit) 
	{
	 	ttsInitListener = new TTS.InitListener() {
			public void onInit(int version) {
        		}
		};
		tts=new TTS(navit, ttsInitListener, true);
	}
	public void run()
	{
		Log.e("NavitSpeech","In "+what);
		tts.speak(what, 0, null);
	}
	public void say(String what)
	{
		this.what=what;
		thread = new Thread(this, "speech thread");
		thread.start();
	}
}

