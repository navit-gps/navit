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

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;

import android.app.Activity;
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
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.RelativeLayout;


public class NavitGraphics
{
	private NavitGraphics			parent_graphics;
	private ArrayList					overlays	= new ArrayList();
	int									bitmap_w;
	int									bitmap_h;
	int									pos_x;
	int									pos_y;
	int									pos_wraparound;
	int									overlay_disabled;
	float									trackball_x, trackball_y;
	View									view;
	RelativeLayout						relativelayout;
	NavitCamera							camera;
	Activity								activity;

	public static Boolean			in_map						= false;
	private static long				time_for_long_press		= 300L;
	private static long				interval_for_long_press	= 200L;
	
	// Overlay View for Android
	//
	// here you can draw all the nice things you want
	// and get touch events for it (without touching C-code)
	private NavitAndroidOverlay NavitAOverlay;
	

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
	public NavitGraphics(Activity activity, NavitGraphics parent, int x, int y, int w, int h,
			int alpha, int wraparound, int use_camera)
	{
		if (parent == null)
		{
			this.activity = activity;
			view = new View(activity)
			{
				int					touch_mode	= NONE;
				float					oldDist		= 0;
				PointF				touch_now	= new PointF(0, 0);
				PointF				touch_start	= new PointF(0, 0);
				PointF				touch_prev	= new PointF(0, 0);
				static final int	NONE			= 0;
				static final int	DRAG			= 1;
				static final int	ZOOM			= 2;
				static final int	PRESS			= 3;
			
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
								Log.e("NavitGraphics", "view -> SHOW SoftInput");
								//Log.e("NavitGraphics", "view mgr=" + String.valueOf(Navit.mgr));
								Navit.mgr.showSoftInput(this, InputMethodManager.SHOW_IMPLICIT);
								Navit.show_soft_keyboard_now_showing=true;
								// clear the variable now, keyboard will stay on screen until backbutton pressed
								Navit.show_soft_keyboard=false;
							}
						}
					}
				}

				@Override
				protected void onSizeChanged(int w, int h, int oldw, int oldh)
				{
					//Log.e("Navit","NavitGraphics -> onSizeChanged");
					super.onSizeChanged(w, h, oldw, oldh);
					draw_bitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
					draw_canvas = new Canvas(draw_bitmap);
					bitmap_w = w;
					bitmap_h = h;
					SizeChangedCallback(SizeChangedCallbackID, w, h);
				}

				@Override
				public boolean onTouchEvent(MotionEvent event)
				{
					PointF touch_now2 = null;
					PointF touch_start2 = null;
					PointF touch_prev2 = null;
					PointF touch_last_load_tiles2 = null;

					super.onTouchEvent(event);
					int action = event.getAction();
					int x = (int) event.getX();
					int y = (int) event.getY();

					int _ACTION_POINTER_UP_ = -999;
					int _ACTION_POINTER_DOWN_ = -999;
					int _ACTION_MASK_ = -999;
					try
					{
						java.lang.reflect.Field field = android.view.MotionEvent.class
								.getField("ACTION_POINTER_UP");
						try
						{
							_ACTION_POINTER_UP_ = field.getInt(event);
						}
						catch (IllegalArgumentException e)
						{
							e.printStackTrace();
						}
						catch (IllegalAccessException e)
						{
							e.printStackTrace();
						}
					}
					catch (NoSuchFieldException ex)
					{

					}
					try
					{
						java.lang.reflect.Field field = android.view.MotionEvent.class
								.getField("ACTION_POINTER_DOWN");
						try
						{
							_ACTION_POINTER_DOWN_ = field.getInt(event);
						}
						catch (IllegalArgumentException e)
						{
							e.printStackTrace();
						}
						catch (IllegalAccessException e)
						{
							e.printStackTrace();
						}
					}
					catch (NoSuchFieldException ex)
					{

					}
					try
					{
						java.lang.reflect.Field field = android.view.MotionEvent.class
								.getField("ACTION_MASK");
						try
						{
							_ACTION_MASK_ = field.getInt(event);
						}
						catch (IllegalArgumentException e)
						{
							e.printStackTrace();
						}
						catch (IllegalAccessException e)
						{
							e.printStackTrace();
						}
					}
					catch (NoSuchFieldException ex)
					{

					}

					// calculate value
					int switch_value = (event.getAction() & _ACTION_MASK_);
					// calculate value

					if (switch_value == MotionEvent.ACTION_DOWN)
					{
						this.touch_now.set(event.getX(), event.getY());
						this.touch_start.set(event.getX(), event.getY());
						this.touch_prev.set(event.getX(), event.getY());
						ButtonCallback(ButtonCallbackID, 1, 1, x, y); // down
						touch_mode = DRAG;

					}
					else if ((switch_value == MotionEvent.ACTION_UP)
							|| (switch_value == _ACTION_POINTER_UP_))
					{
						this.touch_now.set(event.getX(), event.getY());
						touch_now2 = touch_now;
						touch_start2 = touch_start;

						if ((touch_mode == DRAG) && (spacing(touch_start2, touch_now2) < 8f))
						{
							// just a single press down
							touch_mode = PRESS;

							//Log.e("NavitGraphics", "onTouch up");
							//ButtonCallback(ButtonCallbackID, 1, 1, x, y); // down
							ButtonCallback(ButtonCallbackID, 0, 1, x, y); // up

							touch_mode = NONE;
						}
						else
						{
							if (touch_mode == DRAG)
							{
								touch_now2 = touch_now;
								touch_start2 = touch_start;

								//Log.e("NavitGraphics", "onTouch move");
								MotionCallback(MotionCallbackID, x, y);
								ButtonCallback(ButtonCallbackID, 0, 1, x, y); // up

								touch_mode = NONE;
							}
							else
							{
								if (touch_mode == ZOOM)
								{
									// end of "pinch zoom" move
									float newDist = spacing(event);
									float scale = 0;
									if (newDist > 10f)
									{
										scale = newDist / oldDist;
									}

									if (scale > 1.3)
									{
										// zoom in
										String s = java.lang.String.valueOf((char) 17);
										KeypressCallback(KeypressCallbackID, s);
										//ButtonCallback(ButtonCallbackID, 0, 1, x, y); // up
										Log.e("NavitGraphics", "onTouch zoom in");
									}
									else if (scale < 0.8)
									{
										// zoom out    
										String s = java.lang.String.valueOf((char) 15);
										KeypressCallback(KeypressCallbackID, s);
										//ButtonCallback(ButtonCallbackID, 0, 1, x, y); // up
										Log.e("NavitGraphics", "onTouch zoom out");
									}

									touch_mode = NONE;
								}
								else
								{
									//Log.d(TAG, "touch_mode=NONE (END of ZOOM part 2)");
									touch_mode = NONE;
								}
							}
						}
					}
					else if (switch_value == MotionEvent.ACTION_MOVE)
					{

						if (touch_mode == DRAG)
						{
							this.touch_now.set(event.getX(), event.getY());
							touch_now2 = touch_now;
							touch_start2 = touch_start;
							touch_prev2 = touch_prev;
							this.touch_prev.set(event.getX(), event.getY());

							//Log.e("NavitGraphics", "onTouch move2");
							MotionCallback(MotionCallbackID, x, y);
						}
						else if (touch_mode == ZOOM)
						{
							this.touch_now.set(event.getX(), event.getY());
							this.touch_prev.set(event.getX(), event.getY());
						}
					}
					else if (switch_value == _ACTION_POINTER_DOWN_)
					{
						oldDist = spacing(event);
						if (oldDist > 10f)
						{
							touch_mode = ZOOM;
						}
						//break;
					}
					return true;
				}

				private float spacing(PointF a, PointF b)
				{
					float x = a.x - b.x;
					float y = a.y - b.y;
					return FloatMath.sqrt(x * x + y * y);
				}

				public float spacing(MotionEvent event)
				{
					float x;
					float y;
					java.lang.Object instance = event;
					java.lang.Object arg0 = (int) 0;
					java.lang.Object arg1 = (int) 1;
					try
					{
						Method m_getX = android.view.MotionEvent.class.getMethod("getX", int.class);
						Method m_getY = android.view.MotionEvent.class.getMethod("getX", int.class);
						float y0 = 0;
						try
						{
							Float xxxx = (java.lang.Float) m_getY.invoke(instance, arg0);
							y0 = xxxx.floatValue();
						}
						catch (IllegalArgumentException e)
						{
							e.printStackTrace();
						}
						catch (IllegalAccessException e)
						{
							e.printStackTrace();
						}
						catch (InvocationTargetException e)
						{
							e.printStackTrace();
						}

						float y1 = 0;
						try
						{
							Float xxxx = (java.lang.Float) m_getY.invoke(instance, arg1);
							y1 = xxxx.floatValue();
						}
						catch (IllegalArgumentException e)
						{
							e.printStackTrace();
						}
						catch (IllegalAccessException e)
						{
							e.printStackTrace();
						}
						catch (InvocationTargetException e)
						{
							e.printStackTrace();
						}

						float x0 = 0;
						try
						{
							Float xxxx = (java.lang.Float) m_getX.invoke(instance, arg0);
							x0 = xxxx.floatValue();
						}
						catch (IllegalArgumentException e)
						{
							e.printStackTrace();
						}
						catch (IllegalAccessException e)
						{
							e.printStackTrace();
						}
						catch (InvocationTargetException e)
						{
							e.printStackTrace();
						}
						float x1 = 0;
						try
						{
							Float xxxx = (java.lang.Float) m_getY.invoke(instance, arg1);
							x1 = xxxx.floatValue();
						}
						catch (IllegalArgumentException e)
						{
							e.printStackTrace();
						}
						catch (IllegalAccessException e)
						{
							e.printStackTrace();
						}
						catch (InvocationTargetException e)
						{
							e.printStackTrace();
						}
						//.invoke(instance, arg) - m_getX.invoke(instance, arg);
						x = x0 - x1;
						y = y0 - y1;
					}
					catch (NoSuchMethodException ex)
					{
						x = 0;
						y = 0;
					}
					return FloatMath.sqrt(x * x + y * y);
				}

				public boolean onTouchEventxxxxx(MotionEvent event)
				{
					super.onTouchEvent(event);
					int action = event.getAction();
					int x = (int) event.getX();
					int y = (int) event.getY();
					if (action == MotionEvent.ACTION_DOWN)
					{
						// Log.e("NavitGraphics", "onTouch down");
						ButtonCallback(ButtonCallbackID, 1, 1, x, y);
					}
					if (action == MotionEvent.ACTION_UP)
					{
						// Log.e("NavitGraphics", "onTouch up");
						ButtonCallback(ButtonCallbackID, 0, 1, x, y);
						// if (++count == 3)
						//	Debug.stopMethodTracing();
					}
					if (action == MotionEvent.ACTION_MOVE)
					{
						// Log.e("NavitGraphics", "onTouch move");
						MotionCallback(MotionCallbackID, x, y);
					}
					return true;
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
									Log.e("NavitGraphics", "press time=" + Navit.time_pressed_menu_key);
									
									// on long press let softkeyboard popup
									if (Navit.time_pressed_menu_key > time_for_long_press)
									{
										Log.e("NavitGraphics", "long press menu key!!");
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
							Log.e("NavitGraphics", "KEYCODE_BACK down");
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
								handled=true;
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
								handled=true;
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
							Log.e("NavitGraphics", "KEYCODE_BACK up");
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
					Log.e("NavitGraphics", "onTrackball " + event.getAction() + " " + event.getX() + " "
							+ event.getY());
					String s = null;
					if (event.getAction() == android.view.MotionEvent.ACTION_DOWN)
					{
						s = java.lang.String.valueOf((char) 13);
					}
					if (event.getAction() == android.view.MotionEvent.ACTION_MOVE)
					{
						trackball_x += event.getX();
						trackball_y += event.getY();
						Log.e("NavitGraphics", "trackball " + trackball_x + " " + trackball_y);
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
					Log.e("NavitGraphics", "FocusChange " + gainFocus);
				}
			};
			view.setFocusable(true);
			view.setFocusableInTouchMode(true);
			view.setKeepScreenOn(true);
			relativelayout = new RelativeLayout(activity);
			if (use_camera != 0)
			{
				SetCamera(use_camera);
			}
			relativelayout.addView(view);
			
			// android overlay
			Log.e("Navit","create android overlay");
			NavitAOverlay = new NavitAndroidOverlay(relativelayout.getContext());
			RelativeLayout.LayoutParams NavitAOverlay_lp = new RelativeLayout.LayoutParams(
					RelativeLayout.LayoutParams.FILL_PARENT, RelativeLayout.LayoutParams.FILL_PARENT);
			relativelayout.addView(NavitAOverlay, NavitAOverlay_lp);
			NavitAOverlay.bringToFront();
			NavitAOverlay.invalidate();
			// android overlay

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

	public Handler	callback_handler	= new Handler()
													{
														public void handleMessage(Message msg)
														{
															if (msg.getData().getInt("Callback") == 1)
															{
																//Log.e("NavitGraphics","callback_handler -> handleMessage 1");
																KeypressCallback(KeypressCallbackID, msg.getData()
																		.getString("s"));
															}
															else if (msg.getData().getInt("Callback") == 2)
															{
																//Log.e("NavitGraphics","callback_handler -> handleMessage 2");
																ButtonCallback(ButtonCallbackID, 1, 1, 0, 0); // down
															}
															else if (msg.getData().getInt("Callback") == 3)
															{
																//Log.e("NavitGraphics","callback_handler -> handleMessage 3");
																ButtonCallback(ButtonCallbackID, 0, 1, 0, 0); // up
															}
														}
													};
	
	public native void SizeChangedCallback(int id, int x, int y);
	public native void ButtonCallback(int id, int pressed, int button, int x, int y);
	public native void MotionCallback(int id, int x, int y);
	public native void KeypressCallback(int id, String s);
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
		Navit.setMotionCallback(id,this);
	}

	public void setKeypressCallback(int id)
	{
		KeypressCallbackID = id;
		// set callback id also in main intent (for menus)
		Navit.setKeypressCallback(id,this);
	}
	

	protected void draw_polyline(Paint paint, int c[])
	{
		paint.setStyle(Paint.Style.STROKE);
		Path path = new Path();
		path.moveTo(c[0], c[1]);
		for (int i = 2; i < c.length; i += 2)
		{
			path.lineTo(c[i], c[i + 1]);
		}
		draw_canvas.drawPath(path, paint);
	}

	protected void draw_polygon(Paint paint, int c[])
	{
		paint.setStyle(Paint.Style.FILL);
		Path path = new Path();
		path.moveTo(c[0], c[1]);
		for (int i = 2; i < c.length; i += 2)
		{
			path.lineTo(c[i], c[i + 1]);
		}
		draw_canvas.drawPath(path, paint);
	}
	protected void draw_rectangle(Paint paint, int x, int y, int w, int h)
	{
		Rect r = new Rect(x, y, x + w, y + h);
		paint.setStyle(Paint.Style.FILL);
		draw_canvas.drawRect(r, paint);
	}
	protected void draw_circle(Paint paint, int x, int y, int r)
	{
		float fx = x;
		float fy = y;
		float fr = r / 2;
		paint.setStyle(Paint.Style.STROKE);
		draw_canvas.drawCircle(fx, fy, fr, paint);
	}
	protected void draw_text(Paint paint, int x, int y, String text, int size, int dx, int dy)
	{
		float fx = x;
		float fy = y;
		// Log.e("NavitGraphics","Text size "+size + " vs " + paint.getTextSize());
		paint.setTextSize((float) size / 15);
		paint.setStyle(Paint.Style.FILL);
		if (dx == 0x10000 && dy == 0)
		{
			draw_canvas.drawText(text, fx, fy, paint);
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
		// Log.e("NavitGraphics","draw_image");

		float fx = x;
		float fy = y;
		draw_canvas.drawBitmap(bitmap, fx, fy, paint);
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
		//Log.e("NavitGraphics","overlay_disable");
		// assume we are NOT in map view mode!
		in_map=false;

		overlay_disabled = disable;
	}
	protected void overlay_resize(int x, int y, int w, int h, int alpha, int wraparond)
	{
		//Log.e("NavitGraphics","overlay_resize");
		pos_x = x;
		pos_y = y;
	}
}
