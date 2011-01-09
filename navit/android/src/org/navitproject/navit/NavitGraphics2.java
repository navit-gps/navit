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
import android.opengl.GLSurfaceView;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.Window;
import java.io.IOException;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import android.opengl.GLU;
import java.nio.ByteBuffer;
import java.nio.FloatBuffer;
import java.nio.ByteOrder;



class ClearRenderer implements GLSurfaceView.Renderer {
    public FloatBuffer flb[];
    public int flb_len;
    boolean busy;
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        // Do nothing special.
    }

    public void onSurfaceChanged(GL10 gl, int w, int h) {
        gl.glViewport(0, 0, w, h);
    }
   protected static FloatBuffer makeFloatBuffer(float[] arr) {
                ByteBuffer bb = ByteBuffer.allocateDirect(arr.length*4);
                bb.order(ByteOrder.nativeOrder());
                FloatBuffer fb = bb.asFloatBuffer();
                fb.put(arr);
                fb.position(0);
                return fb;
        }



    public void onDrawFrame(GL10 gl) {
	if (busy) {
		return;
	}
	 float[] square = new float[] {  -0.25f, -0.25f, 0.0f,
                        0.25f, -0.25f, 0.0f,
                        -0.25f, 0.25f, 0.0f,
                        0.25f, 0.25f, 0.0f };

gl.glClearColor(1.0f, 1.0f, 0.2f, 0.0f);
                gl.glMatrixMode(GL10.GL_PROJECTION);
                gl.glLoadIdentity();
                //GLU.gluOrtho2D(gl, 0.0f,1.3f,0.0f,1.0f);
                GLU.gluOrtho2D(gl, 0.0f,320.0f,480.0f,0.0f);


        gl.glClear(GL10.GL_COLOR_BUFFER_BIT | GL10.GL_DEPTH_BUFFER_BIT);
gl.glShadeModel(GL10.GL_SMOOTH);
                
                gl.glMatrixMode(GL10.GL_MODELVIEW);
                gl.glLoadIdentity();


                gl.glColor4f(0.25f, 0.25f, 0.75f, 1.0f);
                //gl.glTranslatef(0f, 1f, 0.0f);
                //gl.glScalef(1.0f/320, -1.0f/480, 0.001f);
                //gl.glRotatef(0, 0, 1, 0);
	
			
		Log.e("navit", "flb_len "+flb_len);
		for (int i = 0 ; i < flb_len ; i++) {
			if (flb[i] != null) {
				gl.glVertexPointer(3, GL10.GL_FLOAT, 0, flb[i]);
				gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
				gl.glDrawArrays(GL10.GL_LINE_STRIP, 0, flb[i].capacity()/3);
			}
		}
//		FloatBuffer buf=makeFloatBuffer(square);
//		gl.glVertexPointer(3, GL10.GL_FLOAT, 0, buf);
//		gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
//		Log.e("navit", "capacity "+buf.capacity());
//		gl.glDrawArrays(GL10.GL_LINE_STRIP, 0, buf.capacity()/3);
    }
}

public class NavitGraphics2 {
	private NavitGraphics parent_graphics;
	private ArrayList overlays=new ArrayList();
	int bitmap_w;
	int bitmap_h;
	int pos_x;
	int pos_y;
	int pos_wraparound;
	int overlay_disabled;
	float trackball_x,trackball_y;
	GLSurfaceView view;
	FloatBuffer[] flb;
	
	RelativeLayout relativelayout;
	NavitCamera camera;
	Activity activity;
	ClearRenderer renderer;

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
			flb=new FloatBuffer[10000];
			view=new GLSurfaceView(activity) {
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
		boolean handled=true;
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
				handled=false;
			} else if (keyCode == android.view.KeyEvent.KEYCODE_VOLUME_DOWN) {
				s=java.lang.String.valueOf((char)4);
				handled=false;
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
		return handled;
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
			renderer=new ClearRenderer();	
			renderer.flb=new FloatBuffer[1000];
			renderer.flb_len=0;
			view.setRenderer(renderer);
			view.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
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
	}

	protected void draw_polygon(Paint paint,int c[])
	{
	 //float[] square = new float[] {  -0.25f, -0.25f, 0.0f,
          //              0.25f, -0.25f, 0.0f,
            //            -0.25f, 0.25f, 0.0f,
             //           0.25f, 0.25f, 0.0f };
		int len=c.length/2;
	 	float[] square = new float[3*len];
		for (int i = 0 ; i < len ; i++) {
			square[i*3]=c[i*2];
			square[i*3+1]=c[i*2+1];
			square[i*3+2]=0;
		}
//		if (renderer.flb_len < 100) {
//			renderer.flb[renderer.flb_len++]=ClearRenderer.makeFloatBuffer(square);
//		}
		if (renderer!=null && renderer.flb_len < 1000) {
			renderer.flb[renderer.flb_len++]=ClearRenderer.makeFloatBuffer(square);
			//renderer.flb_len=1;
		}

	}
	protected void draw_rectangle(Paint paint,int x, int y, int w, int h)
	{
	}
	protected void draw_circle(Paint paint,int x, int y, int r)
	{
	}
	protected void draw_text(Paint paint,int x, int y, String text, int size, int dx, int dy)
	{
	}
	protected void draw_image(Paint paint, int x, int y, Bitmap bitmap)
	{
	}
	protected void draw_mode(int mode)
	{
		Log.e("navit", "draw_mode "+mode);
		if (mode == 2 && parent_graphics == null) {
			view.draw(draw_canvas);
			view.invalidate();
			view.requestRender();
		}
		if (mode == 1 || (mode == 0 && parent_graphics != null)) {
			if (renderer!=null) {
				renderer.flb_len=0;
			}
			draw_bitmap.eraseColor(0);
		}
		if (mode == 0 && renderer != null) {
			renderer.flb_len=0;
		}
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
