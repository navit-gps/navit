/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.navitproject.navit;

import android.app.Activity;
import android.widget.TextView;
import android.os.Bundle;
import android.os.Debug;
import android.os.Message;
import android.os.Handler;
import android.content.Context;
import android.util.Log;
import com.google.tts.TTS;


public class NavitSpeech {
	private TTS tts;
    	private TTS.InitListener ttsInitListener;
	NavitSpeech(Context context) 
	{
	 	ttsInitListener = new TTS.InitListener() {
			public void onInit(int version) {
        		}
		};
		tts=new TTS(context, ttsInitListener, true);
	}
	public void say(String what)
	{
		Log.e("NavitSpeech","In "+what);
		tts.speak(what, 0, null);
	}
}

