package org.navitproject.navit;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;

public class NavitMessages {

	static final int DIALOG_REMOVE_PROGRESS_BAR = 0;
	static final int DIALOG_PROGRESS_BAR = 1;
	static final int DIALOG_TOAST = 2;
		
	static public void sendDialogMessage(Handler msgHandler, int what, String title, String text, int dialog_num, int value1, int value2)
	{
		Message msg = msgHandler.obtainMessage(what);
		Bundle data = new Bundle();
		
		data.putString("title", title);
		data.putString("text", text); 
		data.putInt("value1", value1); 
		data.putInt("value2", value2);
		data.putInt("dialog_num", dialog_num);
		msg.setData(data);
		
		msgHandler.sendMessage(msg);
	}
	
}
