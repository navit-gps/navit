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

import java.io.File;
import java.lang.reflect.Method;
import java.util.ArrayList;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PointF;
import android.graphics.Rect;
import android.os.Handler;
import android.os.Message;
import android.util.FloatMath;
import android.util.Log;
import android.view.ContextMenu;
import android.view.KeyEvent;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.RelativeLayout;


public class NavitGraphics
{
	private NavitGraphics            parent_graphics;
	private ArrayList<NavitGraphics> overlays          = new ArrayList<NavitGraphics>();
	int                              bitmap_w;
	int                              bitmap_h;
	int                              pos_x;
	int                              pos_y;
	int                              pos_wraparound;
	int                              overlay_disabled;
	float                            trackball_x, trackball_y;
	View                             view;
	RelativeLayout                   relativelayout;
	NavitCamera                      camera;
	Activity                         activity;

	public static Boolean            in_map            = false;

	// for menu key
	private static long              time_for_long_press               = 300L;
	private static long              interval_for_long_press           = 200L;

	private Handler timer_handler = new Handler();

	public void SetCamera(int use_camera)
	{
		if (use_camera != 0 && camera == null)
		{
			// activity.requestWindowFeature(Window.FEATURE_NO_TITLE);
			camera = new NavitCamera(activity);
			relativelayout.addView(camera);
			relativelayout.bringChildToFront(view);
		}
	}

	private class NavitView extends View implements Runnable, MenuItem.OnMenuItemClickListener{
		int               touch_mode = NONE;
		float             oldDist    = 0;
		static final int  NONE       = 0;
		static final int  DRAG       = 1;
		static final int  ZOOM       = 2;
		static final int  PRESSED    = 3;

		Method eventGetX = null;
		Method eventGetY = null;
		
		public PointF    mPressedPosition = null;
		
		public NavitView(Context context) {
			super(context);
			try
			{
				eventGetX = android.view.MotionEvent.class.getMethod("getX", int.class);
				eventGetY = android.view.MotionEvent.class.getMethod("getY", int.class);
			}
			catch (Exception e)
			{
				Log.e("NavitGraphics", "Multitouch zoom not supported");
			}
		}

		@Override
		protected void onCreateContextMenu(ContextMenu menu) {
			super.onCreateContextMenu(menu);
			
			menu.setHeaderTitle("Position...");
			menu.add(1, 1, NONE, Navit.get_text("drive here")).setOnMenuItemClickListener(this);
//			menu.add(1, 2, NONE, Navit.get_text("Add to contacts")).setOnMenuItemClickListener(this);
		}

		public boolean onMenuItemClick(MenuItem item) {
			switch(item.getItemId()) {
			case 1:
				Message msg = Message.obtain(callback_handler, msg_type.CLB_SET_DISPLAY_DESTINATION.ordinal()
				   , (int)mPressedPosition.x, (int)mPressedPosition.y);
				msg.sendToTarget();
				break;
			case 2:
				break;
			}
			return false;
		}
		
		@Override
		protected void onDraw(Canvas canvas)
		{
			super.onDraw(canvas);
			canvas.drawBitmap(draw_bitmap, pos_x, pos_y, null);
			if (overlay_disabled == 0)
			{
				//Log.e("NavitGraphics", "view -> onDraw 1");
				// assume we ARE in map view mode!
				in_map = true;

				Object overlays_array[];
				overlays_array = overlays.toArray();
				for (Object overlay : overlays_array)
				{
					//Log.e("NavitGraphics","view -> onDraw 2");

					NavitGraphics overlay_graphics = (NavitGraphics) overlay;
					if (overlay_graphics.overlay_disabled == 0)
					{
						//Log.e("NavitGraphics","view -> onDraw 3");

						int x = overlay_graphics.pos_x;
						int y = overlay_graphics.pos_y;
						if (overlay_graphics.pos_wraparound != 0 && x < 0) x += bitmap_w;
						if (overlay_graphics.pos_wraparound != 0 && y < 0) y += bitmap_h;
						canvas.drawBitmap(overlay_graphics.draw_bitmap, x, y, null);
					}
				}
			}
			else
			{
				if (Navit.show_soft_keyboard)
				{
					if (Navit.mgr != null)
					{
						//Log.e("NavitGraphics", "view -> SHOW SoftInput");
						//Log.e("NavitGraphics", "view mgr=" + String.valueOf(Navit.mgr));
						Navit.mgr.showSoftInput(this, InputMethodManager.SHOW_IMPLICIT);
						Navit.show_soft_keyboard_now_showing = true;
						// clear the variable now, keyboard will stay on screen until backbutton pressed
						Navit.show_soft_keyboard = false;
					}
				}
			}
		}

		@Override
		protected void onSizeChanged(int w, int h, int oldw, int oldh)
		{
			Log.e("Navit", "NavitGraphics -> onSizeChanged pixels x=" + w + " pixels y=" + h);
			Log.e("Navit", "NavitGraphics -> onSizeChanged density=" + Navit.metrics.density);
			Log.e("Navit", "NavitGraphics -> onSizeChanged scaledDensity="
					+ Navit.metrics.scaledDensity);
			super.onSizeChanged(w, h, oldw, oldh);
			draw_bitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
			draw_canvas = new Canvas(draw_bitmap);
			bitmap_w = w;
			bitmap_h = h;
			SizeChangedCallback(SizeChangedCallbackID, w, h);
		}

		public void do_longpress_action()
		{
			Log.e("NavitGraphics", "do_longpress_action enter");
			
			activity.openContextMenu(this);
		}

		private int getActionField(String fieldname, Object obj)
		{
			int ret_value = -999;
			try
			{
				java.lang.reflect.Field field = android.view.MotionEvent.class.getField(fieldname);
				try
				{
					ret_value = field.getInt(obj);
				}
				catch (Exception e)
				{
					e.printStackTrace();
				}
			}
			catch (NoSuchFieldException ex) {}

			return ret_value;
		}
		
		@Override
		public boolean onTouchEvent(MotionEvent event)
		{
			//Log.e("NavitGraphics", "onTouchEvent");
			super.onTouchEvent(event);
			int x = (int) event.getX();
			int y = (int) event.getY();

			int _ACTION_POINTER_UP_ = getActionField("ACTION_POINTER_UP", event);
			int _ACTION_POINTER_DOWN_ = getActionField("ACTION_POINTER_DOWN", event);
			int _ACTION_MASK_ = getActionField("ACTION_MASK", event);

			int switch_value = event.getAction();
			if (_ACTION_MASK_ != -999)
			{
				switch_value = (event.getAction() & _ACTION_MASK_);
			}

			if (switch_value == MotionEvent.ACTION_DOWN)
			{
				touch_mode = PRESSED;
				if (!in_map) ButtonCallback(ButtonCallbackID, 1, 1, x, y); // down
				mPressedPosition = new PointF(x, y);
				postDelayed(this, time_for_long_press);
			}
			else if ((switch_value == MotionEvent.ACTION_UP) || (switch_value == _ACTION_POINTER_UP_))
			{
				Log.e("NavitGraphics", "ACTION_UP");

				if ( touch_mode == DRAG )
				{
					Log.e("NavitGraphics", "onTouch move");

					MotionCallback(MotionCallbackID, x, y);
					ButtonCallback(ButtonCallbackID, 0, 1, x, y); // up
				}
				else if (touch_mode == ZOOM)
				{
					//Log.e("NavitGraphics", "onTouch zoom");

					float newDist = spacing(getFloatValue(event, 0), getFloatValue(event, 1));
					float scale = 0;
					if (newDist > 10f)
					{
						scale = newDist / oldDist;
					}

					if (scale > 1.3)
					{
						// zoom in
						CallbackMessageChannel(1, null);
						//Log.e("NavitGraphics", "onTouch zoom in");
					}
					else if (scale < 0.8)
					{
						// zoom out
						CallbackMessageChannel(2, null);
						//Log.e("NavitGraphics", "onTouch zoom out");
					}
				}
				else if (touch_mode == PRESSED)
				{
					if (in_map) ButtonCallback(ButtonCallbackID, 1, 1, x, y); // down
					ButtonCallback(ButtonCallbackID, 0, 1, x, y); // up
				}
				touch_mode = NONE;
			}
			else if (switch_value == MotionEvent.ACTION_MOVE)
			{
				//Log.e("NavitGraphics", "ACTION_MOVE");

				if (touch_mode == DRAG)
				{
					MotionCallback(MotionCallbackID, x, y);
				}
				else if (touch_mode == ZOOM)
				{
					float newDist = spacing(getFloatValue(event, 0), getFloatValue(event, 1));
					float scale = newDist / oldDist;
					Log.e("NavitGraphics", "New scale = " + scale);
					if (scale > 1.2)
					{
						// zoom in
						CallbackMessageChannel(1, "");
						oldDist = newDist;
						//Log.e("NavitGraphics", "onTouch zoom in");
					}
					else if (scale < 0.8)
					{
						oldDist = newDist;
						// zoom out
						CallbackMessageChannel(2, "");
						//Log.e("NavitGraphics", "onTouch zoom out");
					}
				}
				else if (touch_mode == PRESSED)
				{
					Log.e("NavitGraphics", "Start drag mode");
					if ( spacing(mPressedPosition, new PointF(event.getX(),  event.getY()))  > 20f) {
						ButtonCallback(ButtonCallbackID, 1, 1, x, y); // down
						touch_mode = DRAG;
					}
				}
			}
			else if (switch_value == _ACTION_POINTER_DOWN_)
			{
				//Log.e("NavitGraphics", "ACTION_POINTER_DOWN");
				oldDist = spacing(getFloatValue(event, 0), getFloatValue(event, 1));
				if (oldDist > 2f)
				{
					touch_mode = ZOOM;
					//Log.e("NavitGraphics", "--> zoom");
				}
			}
			return true;
		}

		private float spacing(PointF a, PointF b)
		{
			float x = a.x - b.x;
			float y = a.y - b.y;
			return FloatMath.sqrt(x * x + y * y);
		}

		private PointF getFloatValue(Object instance, Object argument)
		{
			PointF pos = new PointF(0,0); 
			
			if (eventGetX != null && eventGetY != null)
			{
				try
				{
					Float x = (java.lang.Float) eventGetX.invoke(instance, argument);
					Float y = (java.lang.Float) eventGetY.invoke(instance, argument);
					pos.set(x.floatValue(), y.floatValue());
					
				}
				catch (Exception e){}
			}
			return pos;
		}
		
		@Override
		public boolean onKeyDown(int keyCode, KeyEvent event)
		{
			int i;
			String s = null;
			boolean handled = true;
			i = event.getUnicodeChar();
			//Log.e("NavitGraphics", "onKeyDown " + keyCode + " " + i);
			// Log.e("NavitGraphics","Unicode "+event.getUnicodeChar());
			if (i == 0)
			{
				if (keyCode == android.view.KeyEvent.KEYCODE_DEL)
				{
					s = java.lang.String.valueOf((char) 8);
				}
				else if (keyCode == android.view.KeyEvent.KEYCODE_MENU)
				{
					if (!in_map)
					{
						// if last menukeypress is less than 0.2 seconds away then count longpress
						if ((System.currentTimeMillis() - Navit.last_pressed_menu_key) < interval_for_long_press)
						{
							Navit.time_pressed_menu_key = Navit.time_pressed_menu_key
									+ (System.currentTimeMillis() - Navit.last_pressed_menu_key);
							//Log.e("NavitGraphics", "press time=" + Navit.time_pressed_menu_key);

							// on long press let softkeyboard popup
							if (Navit.time_pressed_menu_key > time_for_long_press)
							{
								//Log.e("NavitGraphics", "long press menu key!!");
								Navit.show_soft_keyboard = true;
								Navit.time_pressed_menu_key = 0L;
								// need to draw to get the keyboard showing
								this.postInvalidate();
							}
						}
						else
						{
							Navit.time_pressed_menu_key = 0L;
						}
						Navit.last_pressed_menu_key = System.currentTimeMillis();
						// if in menu view:
						// use as OK (Enter) key
						s = java.lang.String.valueOf((char) 13);
						handled = true;
						// dont use menu key here (use it in onKeyUp)
						return handled;
					}
					else
					{
						// if on map view:
						// volume UP
						//s = java.lang.String.valueOf((char) 1);
						handled = false;
						return handled;
					}
				}
				else if (keyCode == android.view.KeyEvent.KEYCODE_SEARCH)
				{
					s = java.lang.String.valueOf((char) 19);
				}
				else if (keyCode == android.view.KeyEvent.KEYCODE_BACK)
				{
					//Log.e("NavitGraphics", "KEYCODE_BACK down");
					s = java.lang.String.valueOf((char) 27);
				}
				else if (keyCode == android.view.KeyEvent.KEYCODE_CALL)
				{
					s = java.lang.String.valueOf((char) 3);
				}
				else if (keyCode == android.view.KeyEvent.KEYCODE_VOLUME_UP)
				{
					if (!in_map)
					{
						// if in menu view:
						// use as UP key
						s = java.lang.String.valueOf((char) 16);
						handled = true;
					}
					else
					{
						// if on map view:
						// volume UP
						//s = java.lang.String.valueOf((char) 21);
						handled = false;
						return handled;
					}
				}
				else if (keyCode == android.view.KeyEvent.KEYCODE_VOLUME_DOWN)
				{
					if (!in_map)
					{
						// if in menu view:
						// use as DOWN key
						s = java.lang.String.valueOf((char) 14);
						handled = true;
					}
					else
					{
						// if on map view:
						// volume DOWN
						//s = java.lang.String.valueOf((char) 4);
						handled = false;
						return handled;
					}
				}
				else if (keyCode == android.view.KeyEvent.KEYCODE_DPAD_CENTER)
				{
					s = java.lang.String.valueOf((char) 13);
				}
				else if (keyCode == android.view.KeyEvent.KEYCODE_DPAD_DOWN)
				{
					s = java.lang.String.valueOf((char) 16);
				}
				else if (keyCode == android.view.KeyEvent.KEYCODE_DPAD_LEFT)
				{
					s = java.lang.String.valueOf((char) 2);
				}
				else if (keyCode == android.view.KeyEvent.KEYCODE_DPAD_RIGHT)
				{
					s = java.lang.String.valueOf((char) 6);
				}
				else if (keyCode == android.view.KeyEvent.KEYCODE_DPAD_UP)
				{
					s = java.lang.String.valueOf((char) 14);
				}
			}
			else if (i == 10)
			{
				s = java.lang.String.valueOf((char) 13);
			}
			else
			{
				s = java.lang.String.valueOf((char) i);
			}
			if (s != null)
			{
				KeypressCallback(KeypressCallbackID, s);
			}
			return handled;
		}

		@Override
		public boolean onKeyUp(int keyCode, KeyEvent event)
		{
			//Log.e("NavitGraphics", "onKeyUp " + keyCode);

			int i;
			String s = null;
			boolean handled = true;
			i = event.getUnicodeChar();

			if (i == 0)
			{
				if (keyCode == android.view.KeyEvent.KEYCODE_VOLUME_UP)
				{
					if (!in_map)
					{
						//s = java.lang.String.valueOf((char) 16);
						handled = true;
						return handled;
					}
					else
					{
						//s = java.lang.String.valueOf((char) 21);
						handled = false;
						return handled;
					}
				}
				else if (keyCode == android.view.KeyEvent.KEYCODE_VOLUME_DOWN)
				{
					if (!in_map)
					{
						//s = java.lang.String.valueOf((char) 14);
						handled = true;
						return handled;
					}
					else
					{
						//s = java.lang.String.valueOf((char) 4);
						handled = false;
						return handled;
					}
				}
				else if (keyCode == android.view.KeyEvent.KEYCODE_BACK)
				{
					if (Navit.show_soft_keyboard_now_showing)
					{
						Navit.show_soft_keyboard_now_showing = false;
					}
					//Log.e("NavitGraphics", "KEYCODE_BACK up");
					//s = java.lang.String.valueOf((char) 27);
					handled = true;
					return handled;
				}
				else if (keyCode == android.view.KeyEvent.KEYCODE_MENU)
				{
					if (!in_map)
					{
						if (Navit.show_soft_keyboard_now_showing)
						{
							// if soft keyboard showing on screen, dont use menu button as select key
						}
						else
						{
							// if in menu view:
							// use as OK (Enter) key
							s = java.lang.String.valueOf((char) 13);
							handled = true;
						}
					}
					else
					{
						// if on map view:
						// volume UP
						//s = java.lang.String.valueOf((char) 1);
						handled = false;
						return handled;
					}
				}
			}
			if (s != null)
			{
				KeypressCallback(KeypressCallbackID, s);
			}
			return handled;

		}

		@Override
		public boolean onTrackballEvent(MotionEvent event)
		{
			//Log.e("NavitGraphics", "onTrackball " + event.getAction() + " " + event.getX() + " "
			//		+ event.getY());
			String s = null;
			if (event.getAction() == android.view.MotionEvent.ACTION_DOWN)
			{
				s = java.lang.String.valueOf((char) 13);
			}
			if (event.getAction() == android.view.MotionEvent.ACTION_MOVE)
			{
				trackball_x += event.getX();
				trackball_y += event.getY();
				//Log.e("NavitGraphics", "trackball " + trackball_x + " " + trackball_y);
				if (trackball_x <= -1)
				{
					s = java.lang.String.valueOf((char) 2);
					trackball_x += 1;
				}
				if (trackball_x >= 1)
				{
					s = java.lang.String.valueOf((char) 6);
					trackball_x -= 1;
				}
				if (trackball_y <= -1)
				{
					s = java.lang.String.valueOf((char) 16);
					trackball_y += 1;
				}
				if (trackball_y >= 1)
				{
					s = java.lang.String.valueOf((char) 14);
					trackball_y -= 1;
				}
			}
			if (s != null)
			{
				KeypressCallback(KeypressCallbackID, s);
			}
			return true;
		}
		@Override
		protected void onFocusChanged(boolean gainFocus, int direction,
				Rect previouslyFocusedRect)
		{
			super.onFocusChanged(gainFocus, direction, previouslyFocusedRect);
			//Log.e("NavitGraphics", "FocusChange " + gainFocus);
		}

		public void run() {
			if (in_map && touch_mode == PRESSED)
			{
				do_longpress_action();
				touch_mode = NONE;
			}
		}
		
	}
	
	public NavitGraphics(final Activity activity, NavitGraphics parent, int x, int y, int w, int h,
			int alpha, int wraparound, int use_camera)
	{
		if (parent == null)
		{
			this.activity = activity;
			view = new NavitView(activity);
			//activity.registerForContextMenu(view);
			view.setClickable(false);
			view.setFocusable(true);
			view.setFocusableInTouchMode(true);
			view.setKeepScreenOn(true);
			relativelayout = new RelativeLayout(activity);
			if (use_camera != 0)
			{
				SetCamera(use_camera);
			}
			relativelayout.addView(view);

			activity.setContentView(relativelayout);
			view.requestFocus();
		}
		else
		{
			draw_bitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
			bitmap_w = w;
			bitmap_h = h;
			pos_x = x;
			pos_y = y;
			pos_wraparound = wraparound;
			draw_canvas = new Canvas(draw_bitmap);
			parent.overlays.add(this);
		}
		parent_graphics = parent;
	}

	static public enum msg_type {
		CLB_ZOOM_IN, CLB_ZOOM_OUT, CLB_REDRAW, CLB_MOVE, CLB_BUTTON_UP, CLB_BUTTON_DOWN, CLB_SET_DESTINATION
		, CLB_SET_DISPLAY_DESTINATION, CLB_CALL_CMD, CLB_COUNTRY_CHOOSER, CLB_LOAD_MAP, CLB_UNLOAD_MAP, CLB_DELETE_MAP
	};

	static public msg_type[] msg_values = msg_type.values();
	
	public Handler	callback_handler	= new Handler()
		{
			public void handleMessage(Message msg)
			{
				switch (msg_values[msg.what])
				{
				case CLB_ZOOM_IN:
					CallbackMessageChannel(1, "");
					break;
				case CLB_ZOOM_OUT:
					CallbackMessageChannel(2, "");
					break;
				case CLB_MOVE:
					MotionCallback(MotionCallbackID, msg.getData().getInt("x"), msg.getData().getInt("y"));
					break;
				case CLB_SET_DESTINATION:
					String lat = Float.toString(msg.getData().getFloat("lat"));
					String lon = Float.toString(msg.getData().getFloat("lon"));
					String q = msg.getData().getString("q");
					CallbackMessageChannel(3, lat + "#" + lon + "#" + q);
					break;
				case CLB_SET_DISPLAY_DESTINATION:
					int x = msg.arg1;
					int y = msg.arg2;
					CallbackMessageChannel(4, "" + x + "#" + y);
					break;
				case CLB_CALL_CMD:
					String cmd = msg.getData().getString("cmd");
					CallbackMessageChannel(5, cmd);
					break;
				case CLB_BUTTON_UP:
					ButtonCallback(ButtonCallbackID, 0, 1, msg.getData().getInt("x"), msg.getData().getInt("y")); // up
					break;
				case CLB_BUTTON_DOWN:
					ButtonCallback(ButtonCallbackID, 1, 1, msg.getData().getInt("x"), msg.getData().getInt("y")); // down
					break;
				case CLB_COUNTRY_CHOOSER:
					break;
				case CLB_LOAD_MAP:
					CallbackMessageChannel(6, msg.getData().getString("title"));
					break;
				case CLB_DELETE_MAP:
					File toDelete = new File( msg.getData().getString("title"));
					toDelete.delete();
				//fallthrough
				case CLB_UNLOAD_MAP:
					CallbackMessageChannel(7, msg.getData().getString("title"));
					break;
				}
			}
		};

	public native void SizeChangedCallback(int id, int x, int y);
	public native void KeypressCallback(int id, String s);
	public native int CallbackMessageChannel(int i, String s);
	public native void ButtonCallback(int id, int pressed, int button, int x, int y);
	public native void MotionCallback(int id, int x, int y);
	public native String GetDefaultCountry(int id, String s);
	public static native String[][] GetAllCountries();
	private Canvas	draw_canvas;
	private Bitmap	draw_bitmap;
	private int		SizeChangedCallbackID, ButtonCallbackID, MotionCallbackID, KeypressCallbackID;
	// private int count;

	public void setSizeChangedCallback(int id)
	{
		SizeChangedCallbackID = id;
	}
	public void setButtonCallback(int id)
	{
		ButtonCallbackID = id;
	}
	public void setMotionCallback(int id)
	{
		MotionCallbackID = id;
		Navit.setMotionCallback(id, this);
	}

	public void setKeypressCallback(int id)
	{
		KeypressCallbackID = id;
		// set callback id also in main intent (for menus)
		Navit.setKeypressCallback(id, this);
	}


	protected void draw_polyline(Paint paint, int c[])
	{
		//	Log.e("NavitGraphics","draw_polyline");
		paint.setStyle(Paint.Style.STROKE);
		//paint.setAntiAlias(true);
		//paint.setStrokeWidth(0);
		Path path = new Path();
		path.moveTo(c[0], c[1]);
		for (int i = 2; i < c.length; i += 2)
		{
			path.lineTo(c[i], c[i + 1]);
		}
		//global_path.close();
		draw_canvas.drawPath(path, paint);
	}

	protected void draw_polygon(Paint paint, int c[])
	{
		//Log.e("NavitGraphics","draw_polygon");
		paint.setStyle(Paint.Style.FILL);
		//paint.setAntiAlias(true);
		//paint.setStrokeWidth(0);
		Path path = new Path();
		path.moveTo(c[0], c[1]);
		for (int i = 2; i < c.length; i += 2)
		{
			path.lineTo(c[i], c[i + 1]);
		}
		//global_path.close();
		draw_canvas.drawPath(path, paint);
	}
	protected void draw_rectangle(Paint paint, int x, int y, int w, int h)
	{
		//Log.e("NavitGraphics","draw_rectangle");
		Rect r = new Rect(x, y, x + w, y + h);
		paint.setStyle(Paint.Style.FILL);
		paint.setAntiAlias(true);
		//paint.setStrokeWidth(0);
		draw_canvas.drawRect(r, paint);
	}
	protected void draw_circle(Paint paint, int x, int y, int r)
	{
		//Log.e("NavitGraphics","draw_circle");
		//		float fx = x;
		//		float fy = y;
		//		float fr = r / 2;
		paint.setStyle(Paint.Style.STROKE);
		draw_canvas.drawCircle(x, y, r / 2, paint);
	}
	protected void draw_text(Paint paint, int x, int y, String text, int size, int dx, int dy)
	{
		//		float fx = x;
		//		float fy = y;
		//Log.e("NavitGraphics","Text size "+size + " vs " + paint.getTextSize());
		paint.setTextSize(size / 15);
		paint.setStyle(Paint.Style.FILL);
		if (dx == 0x10000 && dy == 0)
		{
			draw_canvas.drawText(text, x, y, paint);
		}
		else
		{
			Path path = new Path();
			path.moveTo(x, y);
			path.rLineTo(dx, dy);
			paint.setTextAlign(android.graphics.Paint.Align.LEFT);
			draw_canvas.drawTextOnPath(text, path, 0, 0, paint);
		}
	}
	protected void draw_image(Paint paint, int x, int y, Bitmap bitmap)
	{
		//Log.e("NavitGraphics","draw_image");
		//		float fx = x;
		//		float fy = y;
		draw_canvas.drawBitmap(bitmap, x, y, paint);
	}
	protected void draw_mode(int mode)
	{
		//Log.e("NavitGraphics", "draw_mode mode=" + mode + " parent_graphics="
		//		+ String.valueOf(parent_graphics));

		if (mode == 2 && parent_graphics == null) view.invalidate();
		if (mode == 1 || (mode == 0 && parent_graphics != null)) draw_bitmap.eraseColor(0);

	}
	protected void draw_drag(int x, int y)
	{
		//Log.e("NavitGraphics","draw_drag");
		pos_x = x;
		pos_y = y;
	}
	protected void overlay_disable(int disable)
	{
		Log.e("NavitGraphics","overlay_disable: " + disable + "Parent: " + (parent_graphics != null));
		// assume we are NOT in map view mode!
		if (parent_graphics == null)
			in_map = (disable==0);
		if (overlay_disabled != disable) {
			overlay_disabled = disable;
			if (parent_graphics != null) {
				int x = pos_x;
				int y = pos_y;
				int width = bitmap_w;
				int height = bitmap_h;
				if (pos_wraparound != 0 && x < 0) x += parent_graphics.bitmap_w;
				if (pos_wraparound != 0 && y < 0) y += parent_graphics.bitmap_h;
				if (pos_wraparound != 0 && width < 0) width += parent_graphics.bitmap_w;
				if (pos_wraparound != 0 && height < 0) height += parent_graphics.bitmap_h;
				view.invalidate(x,y,x+width,y+height);
			}
		}
	}

	protected void overlay_resize(int x, int y, int w, int h, int alpha, int wraparond)
	{
		//Log.e("NavitGraphics","overlay_resize");
		pos_x = x;
		pos_y = y;
	}

	public static String getLocalizedString(String text)
	{
		String ret = CallbackLocalizedString(text);
		//Log.e("NavitGraphics", "callback_handler -> lozalized string=" + ret);
		return ret;
	}




	/**
	 * get localized string
	 */
	public static native String CallbackLocalizedString(String s);

}
