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



public class NavitTimeout implements Runnable {
	private static Handler handler =new Handler() {
			public void handleMessage(Message m) {
				Log.e("Navit","Handler received message");
			}
		};
	private boolean event_multi;
	private int event_callbackid;
	private int event_timeout;
	public native void TimeoutCallback(int del, int id);

	NavitTimeout(int timeout, boolean multi, int callbackid) 
	{
		event_timeout=timeout;	
		event_multi=multi;
		event_callbackid=callbackid;
		handler.postDelayed(this, event_timeout);
	}
	public void run() {
		// Log.e("Navit","Handle Event");
		if (event_multi) {
			handler.postDelayed(this, event_timeout);
			TimeoutCallback(0, event_callbackid);
		} else
			TimeoutCallback(1, event_callbackid);
	}
	public void remove()
	{
		handler.removeCallbacks(this);
	}
}

