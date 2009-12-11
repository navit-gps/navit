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

import android.app.Activity;
import android.content.Context;
import android.graphics.*;
import android.os.Bundle;
import android.os.Debug;
import android.view.*;
import android.view.Window;
import android.util.Log;
import android.widget.RelativeLayout;
import java.util.ArrayList;
import java.lang.String;
import android.app.Activity;
import android.content.Context;
import android.hardware.Camera;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.Window;
import java.io.IOException;


public class NavitGraphics {
	private NavitGraphics parent_graphics;
	private ArrayList overlays=new ArrayList();
	int bitmap_w;
	int bitmap_h;
	int pos_x;
	int pos_y;
	int pos_wraparound;
	int overlay_disabled;
	float trackball_x,trackball_y;
	View view;
	RelativeLayout relativelayout;
	NavitCamera camera;
	Activity activity;

	public void
	SetCamera(int use_camera)
	{
		if (use_camera != 0 && camera == null) {
			// activity.requestWindowFeature(Window.FEATURE_NO_TITLE);
			camera=new NavitCamera(activity);
			relativelayout.addView(camera);
			relativelayout.bringChildToFront(view);
		}
	}
	public NavitGraphics(Activity activity, NavitGraphics parent, int x, int y, int w, int h, int alpha, int wraparound, int use_camera) {
		if (parent == null) {
			this.activity=activity;
			view=new View(activity) {
	@Override protected void onDraw(Canvas canvas)
	{
		super.onDraw(canvas);
		canvas.drawBitmap(draw_bitmap, pos_x, pos_y, null);
		if (overlay_disabled == 0) {
			Object overlays_array[];
			overlays_array=overlays.toArray();
			for (Object overlay : overlays_array) {
				NavitGraphics overlay_graphics=(NavitGraphics)overlay;
				if (overlay_graphics.overlay_disabled == 0) {
					int x=overlay_graphics.pos_x;
					int y=overlay_graphics.pos_y;
					if (overlay_graphics.pos_wraparound != 0 && x < 0)
						x+=bitmap_w;
					if (overlay_graphics.pos_wraparound != 0 && y < 0)
						y+=bitmap_h;
					canvas.drawBitmap(overlay_graphics.draw_bitmap, x, y, null);
				}
			}
		}
	}
	@Override protected void onSizeChanged(int w, int h, int oldw, int oldh)
	{
		super.onSizeChanged(w, h, oldw, oldh);
		draw_bitmap=Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
		draw_canvas=new Canvas(draw_bitmap);
		bitmap_w=w;
		bitmap_h=h;
		SizeChangedCallback(SizeChangedCallbackID, w, h);
	}
        @Override public boolean onTouchEvent(MotionEvent event)
	{
		super.onTouchEvent(event);
		int action = event.getAction();
		int x=(int)event.getX();
		int y=(int)event.getY();
		if (action == MotionEvent.ACTION_DOWN) {
			// Log.e("NavitGraphics", "onTouch down");
			ButtonCallback(ButtonCallbackID, 1, 1, x, y);
		}
		if (action == MotionEvent.ACTION_UP) {
			// Log.e("NavitGraphics", "onTouch up");
			ButtonCallback(ButtonCallbackID, 0, 1, x, y);
			// if (++count == 3)
		        //	Debug.stopMethodTracing();
		}
		if (action == MotionEvent.ACTION_MOVE) {
			// Log.e("NavitGraphics", "onTouch move");
			MotionCallback(MotionCallbackID, x, y);
		}
		return true;
	} 
	@Override public boolean onKeyDown(int keyCode, KeyEvent event)
	{
		int i;
		String s=null;
		i=event.getUnicodeChar();
		Log.e("NavitGraphics","onKeyDown "+keyCode+" "+i);
		// Log.e("NavitGraphics","Unicode "+event.getUnicodeChar());
		if (i == 0) {
			if (keyCode == android.view.KeyEvent.KEYCODE_DEL) {
				s=java.lang.String.valueOf((char)8);
			} else if (keyCode == android.view.KeyEvent.KEYCODE_MENU) {
				s=java.lang.String.valueOf((char)1);
			} else if (keyCode == android.view.KeyEvent.KEYCODE_SEARCH) {
				s=java.lang.String.valueOf((char)19);
			} else if (keyCode == android.view.KeyEvent.KEYCODE_BACK) {
				s=java.lang.String.valueOf((char)27);
			} else if (keyCode == android.view.KeyEvent.KEYCODE_CALL) {
				s=java.lang.String.valueOf((char)3);
			} else if (keyCode == android.view.KeyEvent.KEYCODE_VOLUME_UP) {
				s=java.lang.String.valueOf((char)21);
			} else if (keyCode == android.view.KeyEvent.KEYCODE_VOLUME_DOWN) {
				s=java.lang.String.valueOf((char)4);
			} else if (keyCode == android.view.KeyEvent.KEYCODE_DPAD_CENTER) {
				s=java.lang.String.valueOf((char)13);
			} else if (keyCode == android.view.KeyEvent.KEYCODE_DPAD_DOWN) {
				s=java.lang.String.valueOf((char)16);
			} else if (keyCode == android.view.KeyEvent.KEYCODE_DPAD_LEFT) {
				s=java.lang.String.valueOf((char)2);
			} else if (keyCode == android.view.KeyEvent.KEYCODE_DPAD_RIGHT) {
				s=java.lang.String.valueOf((char)6);
			} else if (keyCode == android.view.KeyEvent.KEYCODE_DPAD_UP) {
				s=java.lang.String.valueOf((char)14);
			}
		} else if (i == 10) {
			s=java.lang.String.valueOf((char)13);
		} else {
			s=java.lang.String.valueOf((char)i);
		}
		if (s != null) {
			KeypressCallback(KeypressCallbackID, s);
		}
		return true;
	}
	@Override public boolean onKeyUp(int keyCode, KeyEvent event)
	{
		Log.e("NavitGraphics","onKeyUp "+keyCode);
		return true;
	}
	@Override public boolean onTrackballEvent(MotionEvent event)
	{
		Log.e("NavitGraphics","onTrackball "+event.getAction() + " " +event.getX()+" "+event.getY());
		String s=null;
		if (event.getAction() == android.view.MotionEvent.ACTION_DOWN) {
			s=java.lang.String.valueOf((char)13);
		}
		if (event.getAction() == android.view.MotionEvent.ACTION_MOVE) {
			trackball_x+=event.getX();
			trackball_y+=event.getY();
			Log.e("NavitGraphics","trackball "+trackball_x+" "+trackball_y);
			if (trackball_x <= -1) {
				s=java.lang.String.valueOf((char)2);
				trackball_x+=1;
			}
			if (trackball_x >= 1) {
				s=java.lang.String.valueOf((char)6);
				trackball_x-=1;
			}
			if (trackball_y <= -1) {
				s=java.lang.String.valueOf((char)16);
				trackball_y+=1;
			}
			if (trackball_y >= 1) {
				s=java.lang.String.valueOf((char)14);
				trackball_y-=1;
			}
		}
		if (s != null) {
			KeypressCallback(KeypressCallbackID, s);
		}
		return true;
	}
	@Override protected void onFocusChanged(boolean gainFocus, int direction, Rect previouslyFocusedRect)
	{
		super.onFocusChanged(gainFocus, direction, previouslyFocusedRect);
		Log.e("NavitGraphics","FocusChange "+gainFocus);
	}
			};
			view.setFocusable(true);
			view.setFocusableInTouchMode(true);
			relativelayout=new RelativeLayout(activity);
			if (use_camera != 0) {
				SetCamera(use_camera);
			}
			relativelayout.addView(view);
			activity.setContentView(relativelayout);
			view.requestFocus();
		} else {
			draw_bitmap=Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
			bitmap_w=w;
			bitmap_h=h;
			pos_x=x;
			pos_y=y;
			pos_wraparound=wraparound;
			draw_canvas=new Canvas(draw_bitmap);
			parent.overlays.add(this);		
		}
		parent_graphics=parent;
	}
	public native void SizeChangedCallback(int id, int x, int y);
	public native void ButtonCallback(int id, int pressed, int button, int x, int y);
	public native void MotionCallback(int id, int x, int y);
	public native void KeypressCallback(int id, String s);
	private Canvas draw_canvas;
	private Bitmap draw_bitmap;
	private int SizeChangedCallbackID,ButtonCallbackID,MotionCallbackID,KeypressCallbackID;
	// private int count;

	public void setSizeChangedCallback(int id)
	{
		SizeChangedCallbackID=id;
	}
	public void setButtonCallback(int id)
	{
		ButtonCallbackID=id;
	}
	public void setMotionCallback(int id)
	{
		MotionCallbackID=id;
	}
	public void setKeypressCallback(int id)
	{
		KeypressCallbackID=id;
	}

	protected void draw_polyline(Paint paint,int c[])
	{
		paint.setStyle(Paint.Style.STROKE);
		Path path = new Path();
		path.moveTo(c[0], c[1]);
		for (int i = 2 ; i < c.length ; i+=2) {
			path.lineTo(c[i], c[i+1]);
		}
		draw_canvas.drawPath(path, paint);
	}

	protected void draw_polygon(Paint paint,int c[])
	{
		paint.setStyle(Paint.Style.FILL);
		Path path = new Path();
		path.moveTo(c[0], c[1]);
		for (int i = 2 ; i < c.length ; i+=2) {
			path.lineTo(c[i], c[i+1]);
		}
		draw_canvas.drawPath(path, paint);
	}
	protected void draw_rectangle(Paint paint,int x, int y, int w, int h)
	{
		Rect r = new Rect(x, y, x+w, y+h);
		paint.setStyle(Paint.Style.FILL);
		draw_canvas.drawRect(r, paint);		
	}
	protected void draw_circle(Paint paint,int x, int y, int r)
	{
		float fx=x;
		float fy=y;
		float fr=r/2;
		paint.setStyle(Paint.Style.STROKE);
		draw_canvas.drawCircle(fx, fy, fr, paint);
	}
	protected void draw_text(Paint paint,int x, int y, String text, int size, int dx, int dy)
	{
		float fx=x;
		float fy=y;
		// Log.e("NavitGraphics","Text size "+size + " vs " + paint.getTextSize());
		paint.setTextSize((float)size/15);
		if (dx == 0x10000 && dy == 0) {
			draw_canvas.drawText(text, fx, fy, paint);
		} else {
			Path path = new Path();
			path.moveTo(x, y);
			path.rLineTo(dx, dy);
			paint.setTextAlign(android.graphics.Paint.Align.LEFT);
			draw_canvas.drawTextOnPath(text, path, 0, 0, paint);
		}
	}
	protected void draw_image(Paint paint, int x, int y, Bitmap bitmap)
	{
		float fx=x;
		float fy=y;
		draw_canvas.drawBitmap(bitmap, fx, fy, paint);
	}
	protected void draw_mode(int mode)
	{
		if (mode == 2 && parent_graphics == null)
			view.invalidate();
		if (mode == 1 || (mode == 0 && parent_graphics != null)) 
			draw_bitmap.eraseColor(0);
			
	}
	protected void draw_drag(int x, int y)
	{
		pos_x=x;
		pos_y=y;
	}
	protected void overlay_disable(int disable)
	{
		overlay_disabled=disable;
	}
	protected void overlay_resize(int x, int y, int w, int h, int alpha, int wraparond)
	{
		pos_x=x;
		pos_y=y;
	}
}
