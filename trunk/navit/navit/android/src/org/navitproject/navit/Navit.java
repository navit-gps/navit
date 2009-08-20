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
import android.os.PowerManager;
import android.content.Context;
import android.content.res.Resources;
import android.util.Log;
import java.util.Locale;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;


public class Navit extends Activity implements Handler.Callback
{
    public Handler handler;
    private PowerManager.WakeLock wl;
    private boolean extractRes(String resname, String result)
    {
	int slash=-1;
	boolean needs_update=false;
	File resultfile;
	Resources res=getResources();
	Log.e("Navit","Res Name " + resname);
	Log.e("Navit","result " + result);
	int id=res.getIdentifier(resname,"raw","org.navitproject.navit");
	Log.e("Navit","Res ID " + id);
	if (id == 0)
		return false;
	while ((slash=result.indexOf("/",slash+1)) != -1) {
		if (slash != 0) {
			Log.e("Navit","Checking "+result.substring(0,slash));
			resultfile=new File(result.substring(0,slash));
			if (!resultfile.exists()) {
				Log.e("Navit","Creating dir");
				if (!resultfile.mkdir())
					return false;
				needs_update=true;
			}
		}
	}
	resultfile=new File(result);
	if (!resultfile.exists())
		needs_update=true;
	if (!needs_update) {
		try {
			InputStream resourcestream=res.openRawResource(id);
			FileInputStream resultfilestream=new FileInputStream(resultfile);
			byte[] resourcebuf = new byte[1024];
			byte[] resultbuf = new byte[1024];
			int i = 0;
			while ((i = resourcestream.read(resourcebuf)) != -1) {
				if (resultfilestream.read(resultbuf) != i) {
					Log.e("Navit","Result is too short");
					needs_update=true;
					break;
				}
				for (int j = 0 ; j < i ; j++) {
					if (resourcebuf[j] != resultbuf[j]) {
						Log.e("Navit","Result is different");
						needs_update=true;
						break;
					}
				}
				if (needs_update)
					break;
			}
			if (!needs_update && resultfilestream.read(resultbuf) != -1) {
				Log.e("Navit","Result is too long");
				needs_update=true;
			}
		} 
		catch (Exception e) {
			Log.e("Navit","Exception "+e.getMessage());
			return false;
		}
	}
	if (needs_update) {
		Log.e("Navit","Extracting resource");
		
		try {
			InputStream resourcestream=res.openRawResource(id);
			FileOutputStream resultfilestream=new FileOutputStream(resultfile);
			byte[] buf = new byte[1024];
			int i = 0;
			while ((i = resourcestream.read(buf)) != -1) {
				resultfilestream.write(buf, 0, i);
			}
		} 
		catch (Exception e) {
			Log.e("Navit","Exception "+e.getMessage());
			return false;
		}
	}
	return true;
    }
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
	PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);  
	wl = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK|PowerManager.ON_AFTER_RELEASE, "NavitDoNotDimScreen"); 
	Locale locale=java.util.Locale.getDefault();
	String lang=locale.getLanguage();
	String langu=lang;
	String langc=lang;
	int pos=langu.indexOf('_');
	if (pos != -1) {
		langc=langu.substring(0, pos);
		langu=langc + langu.substring(pos).toUpperCase(locale);
	} else {
		String country=locale.getCountry();
		Log.e("Navit","Country "+country);
		langu=langc + "_" + country.toUpperCase(locale);
	}
	Log.e("Navit","Language " + lang);

	if (!extractRes(langc,"/data/data/org.navitproject.navit/locale/"+langc+"/LC_MESSAGES/navit.mo")) {
		Log.e("Navit","Failed to extract language resource "+langc);
	}
	if (!extractRes("navit","/data/data/org.navitproject.navit/share/navit.xml")) {
		Log.e("Navit","Failed to extract navit.xml");
	}
	// Debug.startMethodTracing("calc");
        NavitMain(this, langu);
    }
    @Override public void onStart()
    {
        super.onStart();
	Log.e("Navit","OnStart");
    }
    @Override public void onRestart()
    {
        super.onRestart();
	Log.e("Navit","OnRestart");
    }
    @Override public void onResume()
    {
        super.onResume();
	Log.e("Navit","OnResume");
    }
    @Override public void onPause()
    {
	super.onPause();
	Log.e("Navit","OnPause");
    }
    @Override public void onStop()
    {
	super.onStop();
	Log.e("Navit","OnStop");
    }
    @Override public void onDestroy()
    {
	super.onDestroy();
	Log.e("Navit","OnDestroy");
    }

    public void disableSuspend()
    {
	wl.acquire();
	wl.release();
    }

    public void exit()
    {
	finish();
    }

    public boolean handleMessage(Message m) {
	Log.e("Navit","Handler received message");
	return true;
    }

    /* A native method that is implemented by the
     * 'hello-jni' native library, which is packaged
     * with this application.
     */
    public native void NavitMain(Navit x, String lang);

    /* this is used to load the 'hello-jni' library on application
     * startup. The library has already been unpacked into
     * /data/data/com.example.Navit/lib/libhello-jni.so at
     * installation time by the package manager.
     */
    static {
        System.loadLibrary("navit");
    }
}

