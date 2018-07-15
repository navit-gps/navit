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

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PointF;
import android.graphics.Rect;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.support.v4.view.ViewConfigurationCompat;
import android.util.Log;
import android.view.ContextMenu;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup.LayoutParams;
import android.view.inputmethod.InputMethodManager;
import android.widget.FrameLayout;
import android.widget.RelativeLayout;
import java.io.File;
import java.lang.reflect.Method;
import java.util.ArrayList;


public class NavitGraphics {
    private static final String            TAG = "NavitGraphics";
    private final NavitGraphics            parent_graphics;
    private final ArrayList<NavitGraphics> overlays = new ArrayList<NavitGraphics>();
    private int                            bitmap_w;
    private int                            bitmap_h;
    private int                            pos_x;
    private int                            pos_y;
    private int                            pos_wraparound;
    private int                            overlay_disabled;
    private int                            bgcolor;
    private float                          trackball_x, trackball_y;
    private View                           view;
    private SystemBarTintView              navigationTintView;
    private SystemBarTintView              statusTintView;
    private FrameLayout                    frameLayout;
    private RelativeLayout                 relativelayout;
    private NavitCamera                    camera;
    private Navit                          activity;
    private static Boolean                 in_map = false;
    // for menu key
    private static final long              time_for_long_press = 300L;


    private Handler timer_handler = new Handler();

    public void setBackgroundColor(int bgcolor) {
        this.bgcolor = bgcolor;
        if (navigationTintView != null)
            navigationTintView.setBackgroundColor(bgcolor);
        if (statusTintView != null)
            statusTintView.setBackgroundColor(bgcolor);
    }

    private void SetCamera(int use_camera) {
        if (use_camera != 0 && camera == null) {
            // activity.requestWindowFeature(Window.FEATURE_NO_TITLE);
            camera = new NavitCamera(activity);
            relativelayout.addView(camera);
            relativelayout.bringChildToFront(view);
        }
    }

    private Rect get_rect() {
        Rect ret=new Rect();
        ret.left=pos_x;
        ret.top=pos_y;
        if (pos_wraparound != 0) {
            if (ret.left < 0) {
                ret.left+=parent_graphics.bitmap_w;
            }
            if (ret.top < 0) {
                ret.top+=parent_graphics.bitmap_h;
            }
        }
        ret.right=ret.left+bitmap_w;
        ret.bottom=ret.top+bitmap_h;
        if (pos_wraparound != 0) {
            if (bitmap_w < 0) {
                ret.right=ret.left+bitmap_w+parent_graphics.bitmap_w;
            }
            if (bitmap_h < 0) {
                ret.bottom=ret.top+bitmap_h+parent_graphics.bitmap_h;
            }
        }
        return ret;
    }

    private class NavitView extends View implements Runnable, MenuItem.OnMenuItemClickListener {
        int               touch_mode = NONE;
        float             oldDist    = 0;
        static final int  NONE       = 0;
        static final int  DRAG       = 1;
        static final int  ZOOM       = 2;
        static final int  PRESSED    = 3;

        Method eventGetX = null;
        Method eventGetY = null;

        PointF    mPressedPosition = null;

        public NavitView(Context context) {
            super(context);
            try {
                eventGetX = android.view.MotionEvent.class.getMethod("getX", int.class);
                eventGetY = android.view.MotionEvent.class.getMethod("getY", int.class);
            } catch (Exception e) {
                Log.e(TAG, "Multitouch zoom not supported");
            }
        }

        @Override
        protected void onCreateContextMenu(ContextMenu menu) {
            super.onCreateContextMenu(menu);

            menu.setHeaderTitle(activity.getTstring(R.string.position_popup_title)+"..");
            menu.add(1, 1, NONE, activity.getTstring(R.string.position_popup_drive_here)).setOnMenuItemClickListener(this);
            menu.add(1, 2, NONE, activity.getTstring(R.string.cancel)).setOnMenuItemClickListener(this);
        }

        @Override
        public boolean onMenuItemClick(MenuItem item) {
            switch(item.getItemId()) {
            case 1:
                Message msg = Message.obtain(callback_handler, msg_type.CLB_SET_DISPLAY_DESTINATION.ordinal()
                                             , (int)mPressedPosition.x, (int)mPressedPosition.y);
                msg.sendToTarget();
                break;
            }
            return false;
        }


        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            canvas.drawBitmap(draw_bitmap, pos_x, pos_y, null);
            if (overlay_disabled == 0) {
                // assume we ARE in map view mode!
                in_map = true;
                for (NavitGraphics overlay : overlays) {
                    if (overlay.overlay_disabled == 0) {
                        Rect r=overlay.get_rect();
                        canvas.drawBitmap(overlay.draw_bitmap, r.left, r.top, null);
                    }
                }
            } else {
                if (Navit.show_soft_keyboard) {
                    if (Navit.mgr != null) {
                        Navit.mgr.showSoftInput(this, InputMethodManager.SHOW_IMPLICIT);
                        Navit.show_soft_keyboard_now_showing = true;
                        // clear the variable now, keyboard will stay on screen until backbutton pressed
                        Navit.show_soft_keyboard = false;
                    }
                }
            }
        }

        @Override
        protected void onSizeChanged(int w, int h, int oldw, int oldh) {
            Log.d(TAG, "onSizeChanged pixels x=" + w + " pixels y=" + h);
            Log.d(TAG, "onSizeChanged density=" + Navit.metrics.density);
            Log.d(TAG, "onSizeChanged scaledDensity="
                  + Navit.metrics.scaledDensity);
            super.onSizeChanged(w, h, oldw, oldh);

            handleResize(w, h);
        }

        void do_longpress_action() {
            Log.d(TAG, "do_longpress_action enter");

            activity.openContextMenu(this);
        }

        private int getActionField(String fieldname, Object obj) {
            int ret_value = -999;
            try {
                java.lang.reflect.Field field = android.view.MotionEvent.class.getField(fieldname);
                try {
                    ret_value = field.getInt(obj);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            } catch (NoSuchFieldException ex) {
                ex.printStackTrace();
            }
            return ret_value;
        }

        @SuppressLint("ClickableViewAccessibility")
        @Override
        public boolean onTouchEvent(MotionEvent event) {
            super.onTouchEvent(event);
            int x = (int) event.getX();
            int y = (int) event.getY();

            int _ACTION_POINTER_UP_ = getActionField("ACTION_POINTER_UP", event);
            int _ACTION_POINTER_DOWN_ = getActionField("ACTION_POINTER_DOWN", event);
            int _ACTION_MASK_ = getActionField("ACTION_MASK", event);

            int switch_value = event.getAction();
            if (_ACTION_MASK_ != -999) {
                switch_value = (event.getAction() & _ACTION_MASK_);
            }

            if (switch_value == MotionEvent.ACTION_DOWN) {
                touch_mode = PRESSED;
                if (!in_map) ButtonCallback(ButtonCallbackID, 1, 1, x, y); // down
                mPressedPosition = new PointF(x, y);
                postDelayed(this, time_for_long_press);
            } else if ((switch_value == MotionEvent.ACTION_UP) || (switch_value == _ACTION_POINTER_UP_)) {
                Log.d(TAG, "ACTION_UP");

                switch (touch_mode) {
                case DRAG:
                    Log.d(TAG, "onTouch move");

                    MotionCallback(MotionCallbackID, x, y);
                    ButtonCallback(ButtonCallbackID, 0, 1, x, y); // up

                    break;
                case ZOOM:
                    float newDist = spacing(getFloatValue(event, 0), getFloatValue(event, 1));
                    float scale = 0;
                    if (newDist > 10f) {
                        scale = newDist / oldDist;
                    }

                    if (scale > 1.3) {
                        // zoom in
                        CallbackMessageChannel(1, null);
                    } else if (scale < 0.8) {
                        // zoom out
                        CallbackMessageChannel(2, null);
                    }
                    break;
                case PRESSED:
                    if (in_map) ButtonCallback(ButtonCallbackID, 1, 1, x, y); // down
                    ButtonCallback(ButtonCallbackID, 0, 1, x, y); // up

                    break;
                }
                touch_mode = NONE;
            } else if (switch_value == MotionEvent.ACTION_MOVE) {
                switch (touch_mode) {
                case DRAG:
                    MotionCallback(MotionCallbackID, x, y);
                    break;
                case ZOOM:
                    float newDist = spacing(getFloatValue(event, 0), getFloatValue(event, 1));
                    float scale = newDist / oldDist;
                    Log.d(TAG, "New scale = " + scale);
                    if (scale > 1.2) {
                        // zoom in
                        CallbackMessageChannel(1, "");
                        oldDist = newDist;
                    } else if (scale < 0.8) {
                        oldDist = newDist;
                        // zoom out
                        CallbackMessageChannel(2, "");
                    }
                    break;
                case PRESSED:
                    Log.d(TAG, "Start drag mode");
                    if (spacing(mPressedPosition, new PointF(event.getX(), event.getY())) > 20f) {
                        ButtonCallback(ButtonCallbackID, 1, 1, x, y); // down
                        touch_mode = DRAG;
                    }
                    break;
                }
            } else if (switch_value == _ACTION_POINTER_DOWN_) {
                oldDist = spacing(getFloatValue(event, 0), getFloatValue(event, 1));
                if (oldDist > 2f) {
                    touch_mode = ZOOM;
                }
            }
            return true;
        }

        private float spacing(PointF a, PointF b) {
            float x = a.x - b.x;
            float y = a.y - b.y;
            return (float)Math.sqrt(x * x + y * y);
        }

        private PointF getFloatValue(Object instance, Object argument) {
            PointF pos = new PointF(0,0);

            if (eventGetX != null && eventGetY != null) {
                try {
                    Float x = (java.lang.Float) eventGetX.invoke(instance, argument);
                    Float y = (java.lang.Float) eventGetY.invoke(instance, argument);
                    pos.set(x, y);

                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
            return pos;
        }

        @Override
        public boolean onKeyDown(int keyCode, KeyEvent event) {
            int i;
            String s = null;
            long interval_for_long_press = 200L;
            i = event.getUnicodeChar();
            if (i == 0) {
                switch (keyCode) {
                case KeyEvent.KEYCODE_DEL:
                    s = String.valueOf((char) 8);
                    break;
                case KeyEvent.KEYCODE_MENU:
                    if (!in_map) {
                        // if last menukeypress is less than 0.2 seconds away then count longpress
                        if ((System.currentTimeMillis() - Navit.last_pressed_menu_key) < interval_for_long_press) {
                            Navit.time_pressed_menu_key = Navit.time_pressed_menu_key
                                                          + (System.currentTimeMillis() - Navit.last_pressed_menu_key);
                            // on long press let softkeyboard popup
                            if (Navit.time_pressed_menu_key > time_for_long_press) {
                                Navit.show_soft_keyboard = true;
                                Navit.time_pressed_menu_key = 0L;
                                // need to draw to get the keyboard showing
                                this.postInvalidate();
                            }
                        } else {
                            Navit.time_pressed_menu_key = 0L;
                        }
                        Navit.last_pressed_menu_key = System.currentTimeMillis();
                        // if in menu view:
                        // use as OK (Enter) key
                        // dont use menu key here (use it in onKeyUp)
                        return true;
                    } else {
                        // if on map view:
                        // volume UP
                        //s = java.lang.String.valueOf((char) 1);
                        return true;
                    }
                case KeyEvent.KEYCODE_SEARCH:
                    /* Handle event in Main Activity if map is shown */
                    if (in_map)
                        return false;

                    s = String.valueOf((char) 19);
                    break;
                case KeyEvent.KEYCODE_BACK:
                    s = String.valueOf((char) 27);
                    break;
                case KeyEvent.KEYCODE_CALL:
                    s = String.valueOf((char) 3);
                    break;
                case KeyEvent.KEYCODE_VOLUME_UP:
                    if (!in_map) {
                        // if in menu view:
                        // use as UP key
                        s = String.valueOf((char) 16);
                    } else {
                        // if on map view:
                        // volume UP
                        //s = java.lang.String.valueOf((char) 21);
                        return false;
                    }
                    break;
                case KeyEvent.KEYCODE_VOLUME_DOWN:
                    if (!in_map) {
                        // if in menu view:
                        // use as DOWN key
                        s = String.valueOf((char) 14);
                    } else {
                        // if on map view:
                        // volume DOWN
                        //s = java.lang.String.valueOf((char) 4);
                        return false;
                    }
                    break;
                case KeyEvent.KEYCODE_DPAD_CENTER:
                    s = String.valueOf((char) 13);
                    break;
                case KeyEvent.KEYCODE_DPAD_DOWN:
                    s = String.valueOf((char) 14);
                    break;
                case KeyEvent.KEYCODE_DPAD_LEFT:
                    s = String.valueOf((char) 2);
                    break;
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                    s = String.valueOf((char) 6);
                    break;
                case KeyEvent.KEYCODE_DPAD_UP:
                    s = String.valueOf((char) 16);
                    break;
                }
            } else if (i == 10) {
                s = java.lang.String.valueOf((char) 13);
            }

            if (s != null) {
                KeypressCallback(KeypressCallbackID, s);
            }
            return true;
        }

        @Override
        public boolean onKeyUp(int keyCode, KeyEvent event) {
            int i;
            String s = null;
            i = event.getUnicodeChar();

            if (i == 0) {
                switch (keyCode) {
                case KeyEvent.KEYCODE_VOLUME_UP:
                    return (!in_map);
                case KeyEvent.KEYCODE_VOLUME_DOWN:
                    return (!in_map);
                case KeyEvent.KEYCODE_SEARCH:
                    /* Handle event in Main Activity if map is shown */
                    if (in_map)
                        return false;
                    break;
                case KeyEvent.KEYCODE_BACK:
                    if (Navit.show_soft_keyboard_now_showing) {
                        Navit.show_soft_keyboard_now_showing = false;
                    }
                    //s = java.lang.String.valueOf((char) 27);
                    return true;
                case KeyEvent.KEYCODE_MENU:
                    if (!in_map) {
                        if (Navit.show_soft_keyboard_now_showing) {
                            // if soft keyboard showing on screen, dont use menu button as select key
                        } else {
                            // if in menu view:
                            // use as OK (Enter) key
                            s = String.valueOf((char) 13);
                        }
                    } else {
                        // if on map view:
                        // volume UP
                        //s = java.lang.String.valueOf((char) 1);
                        return false;
                    }
                    break;
                }
            } else if(i!=10) {
                s = java.lang.String.valueOf((char) i);
            }

            if (s != null) {
                KeypressCallback(KeypressCallbackID, s);
            }
            return true;
        }

        @Override
        public boolean onKeyMultiple (int keyCode, int count, KeyEvent event) {
            String s;
            if(keyCode == KeyEvent.KEYCODE_UNKNOWN) {
                s=event.getCharacters();
                KeypressCallback(KeypressCallbackID, s);
                return true;
            }
            return super.onKeyMultiple(keyCode, count, event);
        }

        @Override
        public boolean onTrackballEvent(MotionEvent event) {
            String s = null;
            if (event.getAction() == android.view.MotionEvent.ACTION_DOWN) {
                s = java.lang.String.valueOf((char) 13);
            }
            if (event.getAction() == android.view.MotionEvent.ACTION_MOVE) {
                trackball_x += event.getX();
                trackball_y += event.getY();
                if (trackball_x <= -1) {
                    s = java.lang.String.valueOf((char) 2);
                    trackball_x += 1;
                }
                if (trackball_x >= 1) {
                    s = java.lang.String.valueOf((char) 6);
                    trackball_x -= 1;
                }
                if (trackball_y <= -1) {
                    s = java.lang.String.valueOf((char) 16);
                    trackball_y += 1;
                }
                if (trackball_y >= 1) {
                    s = java.lang.String.valueOf((char) 14);
                    trackball_y -= 1;
                }
            }
            if (s != null) {
                KeypressCallback(KeypressCallbackID, s);
            }
            return true;
        }

        public void run() {
            if (in_map && touch_mode == PRESSED) {
                do_longpress_action();
                touch_mode = NONE;
            }
        }

    }

    private class SystemBarTintView extends View {

        public SystemBarTintView(Context context) {
            super(context);
            this.setBackgroundColor(bgcolor);
        }

    }

    public NavitGraphics(final Activity activity, NavitGraphics parent, int x, int y, int w, int h,
                         int wraparound, int use_camera) {
        if (parent == null) {
            this.activity = (Navit) activity;
            view = new NavitView(activity);
            //activity.registerForContextMenu(view);
            view.setClickable(false);
            view.setFocusable(true);
            view.setFocusableInTouchMode(true);
            view.setKeepScreenOn(true);
            relativelayout = new RelativeLayout(activity);
            if (use_camera != 0) {
                SetCamera(use_camera);
            }
            relativelayout.addView(view);

            /* The navigational and status bar tinting code is meaningful only on API19+ */
            if(Build.VERSION.SDK_INT >= 19) {
                frameLayout = new FrameLayout(activity);
                frameLayout.addView(relativelayout);
                navigationTintView = new SystemBarTintView(activity);
                statusTintView = new SystemBarTintView(activity);
                frameLayout.addView(navigationTintView);
                frameLayout.addView(statusTintView);
                activity.setContentView(frameLayout);
            } else {
                activity.setContentView(relativelayout);
            }

            view.requestFocus();
        } else {
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

    public enum msg_type {
        CLB_ZOOM_IN, CLB_ZOOM_OUT, CLB_REDRAW, CLB_MOVE, CLB_BUTTON_UP, CLB_BUTTON_DOWN, CLB_SET_DESTINATION
        , CLB_SET_DISPLAY_DESTINATION, CLB_CALL_CMD, CLB_COUNTRY_CHOOSER, CLB_LOAD_MAP, CLB_UNLOAD_MAP, CLB_DELETE_MAP
    }

    static private final msg_type[] msg_values = msg_type.values();

    public final Handler    callback_handler    = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg_values[msg.what]) {
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
                String q = msg.getData().getString(("q"));
                CallbackMessageChannel(3, lat + "#" + lon + "#" + q);
                break;
            case CLB_SET_DISPLAY_DESTINATION:
                int x = msg.arg1;
                int y = msg.arg2;
                CallbackMessageChannel(4, "" + x + "#" + y);
                break;
            case CLB_CALL_CMD:
                String cmd = msg.getData().getString(("cmd"));
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
                CallbackMessageChannel(6, msg.getData().getString(("title")));
                break;
            case CLB_DELETE_MAP:
                File toDelete = new File( msg.getData().getString(("title")));
                toDelete.delete();
            //fallthrough
            case CLB_UNLOAD_MAP:
                CallbackMessageChannel(7, msg.getData().getString(("title")));
                break;
            }
        }
    };

    public native void SizeChangedCallback(int id, int x, int y);
    public native void PaddingChangedCallback(int id, int left, int right, int top, int bottom);
    public native void KeypressCallback(int id, String s);
    public native int CallbackMessageChannel(int i, String s);
    public native void ButtonCallback(int id, int pressed, int button, int x, int y);
    public native void MotionCallback(int id, int x, int y);
    public native String GetDefaultCountry(int id, String s);
    public static native String[][] GetAllCountries();
    private Canvas  draw_canvas;
    private Bitmap  draw_bitmap;
    private int     SizeChangedCallbackID, PaddingChangedCallbackID, ButtonCallbackID, MotionCallbackID, KeypressCallbackID;

    /**
     * @brief Adjust views used to tint navigation and status bars.
     *
     * This method is called from handleResize.
     *
     * It (re-)evaluates if and where the navigation bar is going to be shown, and calculates the
     * padding for objects which should not be obstructed.
     *
     */
    private void adjustSystemBarsTintingViews() {

        /* frameLayout is only created on platforms supporting navigation and status bar tinting */
        if (frameLayout == null)
            return;

        if (activity == null) {
            Log.w(TAG, "Main Activity is not a Navit instance, cannot update padding");
            return;
        }

        Navit navit = activity;

        /*
         * Determine visibility of status bar.
         * The status bar is always visible unless we are in fullscreen mode.
         */
        final Boolean isStatusShowing = !navit.isFullscreen;

        /*
         * Determine visibility of navigation bar.
         * This logic is based on the presence of a hardware menu button and is known to work on
         * devices which allow switching between hw and sw buttons (OnePlus One running CyanogenMod).
         */
        final Boolean isNavShowing = !ViewConfigurationCompat.hasPermanentMenuKey(ViewConfiguration.get(navit));
        Log.d(TAG, String.format("isStatusShowing=%b isNavShowing=%b", isStatusShowing, isNavShowing));

        /*
         * Determine where the navigation bar would be displayed.
         * Logic is taken from AOSP RenderSessionImpl.findNavigationBar()
         * (platform/frameworks/base/tools/layoutlib/bridge/src/com/android/layoutlib/bridge/impl/RenderSessionImpl.java)
         */
        final Boolean isLandscape = (navit.getResources().getConfiguration().orientation ==
                                     Configuration.ORIENTATION_LANDSCAPE);
        final Boolean isNavAtBottom = (!isLandscape) || (navit.getResources().getConfiguration().smallestScreenWidthDp >= 600);
        Log.d(TAG, String.format("isNavAtBottom=%b (Configuration.smallestScreenWidthDp=%d, isLandscape=%b)",
                                 isNavAtBottom, navit.getResources().getConfiguration().smallestScreenWidthDp, isLandscape));

        int left = 0;
        int top = isStatusShowing ? Navit.status_bar_height : 0;
        int right = (isNavShowing && !isNavAtBottom) ? Navit.navigation_bar_width : 0;
        final int bottom = (!(isNavShowing
                              && isNavAtBottom)) ? 0 : isLandscape ? Navit.navigation_bar_height_landscape : Navit.navigation_bar_height;

        /* hide tint bars during update to prevent ugly effects */
        statusTintView.setVisibility(View.GONE);
        navigationTintView.setVisibility(View.GONE);

        frameLayout.post(new Runnable() {
            @Override
            public void run() {
                statusTintView.setVisibility(isStatusShowing ? View.VISIBLE : View.GONE);
                FrameLayout.LayoutParams statusLayoutParams = new FrameLayout.LayoutParams(LayoutParams.MATCH_PARENT,
                        Navit.status_bar_height, Gravity.TOP);

                /* Prevent tint views from overlapping when navigation is on the right */
                statusLayoutParams.setMargins(0, 0, (isNavShowing && !isNavAtBottom) ? Navit.navigation_bar_width : 0, 0);
                statusTintView.setLayoutParams(statusLayoutParams);
                Log.d(TAG, String.format("statusTintView: width=%d height=%d",
                                         statusTintView.getWidth(), statusTintView.getHeight()));
                navigationTintView.setVisibility(isNavShowing ? View.VISIBLE : View.GONE);
                LayoutParams navigationLayoutParams = new FrameLayout.LayoutParams(
                    isNavAtBottom ? LayoutParams.MATCH_PARENT : Navit.navigation_bar_width,  // X
                    isNavAtBottom ? bottom : LayoutParams.MATCH_PARENT, // Y
                    Gravity.BOTTOM | Gravity.RIGHT);
                navigationTintView.setLayoutParams(navigationLayoutParams);
                Log.d(TAG, String.format("navigationTintView: width=%d height=%d",
                                         navigationTintView.getWidth(), navigationTintView.getHeight()));
            }
        });

        Log.d(TAG, String.format("Padding left=%d top=%d right=%d bottom=%d", left, top, right, bottom));

        PaddingChangedCallback(PaddingChangedCallbackID, left, top, right, bottom);
    }

    /**
     * @brief Handles resize events.
     *
     * This method is called whenever the main View is resized in any way. This is the case when its
     * {@code onSizeChanged()} event handler fires or when toggling Fullscreen mode.
     *
     */
    public void handleResize(int w, int h) {
        if (this.parent_graphics != null)
            this.parent_graphics.handleResize(w, h);
        else {
            Log.d(TAG, String.format("handleResize w=%d h=%d", w, h));

            adjustSystemBarsTintingViews();

            draw_bitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
            draw_canvas = new Canvas(draw_bitmap);
            bitmap_w = w;
            bitmap_h = h;
            SizeChangedCallback(SizeChangedCallbackID, w, h);
        }
    }

    /**
     * @brief Returns whether the device has a hardware menu button.
     *
     * Only Android versions starting with ICS (API version 14) support the API call to detect the presence of a
     * Menu button. On earlier Android versions, the following assumptions will be made: On API levels up to 10,
     * this method will always return {@code true}, as these Android versions relied on devices having a physical
     * Menu button. On API levels 11 through 13 (Honeycomb releases), this method will always return
     * {@code false}, as Honeycomb was a tablet-only release and did not require devices to have a Menu button.
     *
     * Note that this method is not aware of non-standard mechanisms on some customized builds of Android. For
     * example, CyanogenMod has an option to add a menu button to the navigation bar. Even with that option,
     * this method will still return `false`.
     */
    public boolean hasMenuButton() {
        if (Build.VERSION.SDK_INT <= 10)
            return true;
        else if (Build.VERSION.SDK_INT <= 13)
            return false;
        else
            return ViewConfiguration.get(activity.getApplication()).hasPermanentMenuKey();
    }

    public void setSizeChangedCallback(int id) {
        SizeChangedCallbackID = id;
    }
    public void setPaddingChangedCallback(int id) {
        PaddingChangedCallbackID = id;
    }
    public void setButtonCallback(int id) {
        ButtonCallbackID = id;
    }
    public void setMotionCallback(int id) {
        MotionCallbackID = id;
        if(activity != null) {
            activity.setMotionCallback(id, this);
        }
    }

    public void setKeypressCallback(int id) {
        KeypressCallbackID = id;
        // set callback id also in main intent (for menus)
        if(activity != null) {
            activity.setKeypressCallback(id, this);
        }
    }


    protected void draw_polyline(Paint paint, int[] c) {
        int i, ndashes;
        float [] intervals;
        paint.setStrokeWidth(c[0]);
        paint.setARGB(c[1],c[2],c[3],c[4]);
        paint.setStyle(Paint.Style.STROKE);
        //paint.setAntiAlias(true);
        //paint.setStrokeWidth(0);
        ndashes=c[5];
        intervals=new float[ndashes+(ndashes%2)];
        for (i = 0; i < ndashes; i++)
            intervals[i]=c[6+i];

        if((ndashes%2)==1)
            intervals[ndashes]=intervals[ndashes-1];

        if(ndashes>0)
            paint.setPathEffect(new android.graphics.DashPathEffect(intervals,0.0f));

        Path path = new Path();
        path.moveTo(c[6+ndashes], c[7+ndashes]);
        for (i = 8+ndashes; i < c.length; i += 2) {
            path.lineTo(c[i], c[i + 1]);
        }
        //global_path.close();
        draw_canvas.drawPath(path, paint);
        paint.setPathEffect(null);
    }

    protected void draw_polygon(Paint paint, int[] c) {
        paint.setStrokeWidth(c[0]);
        paint.setARGB(c[1],c[2],c[3],c[4]);
        paint.setStyle(Paint.Style.FILL);
        //paint.setAntiAlias(true);
        //paint.setStrokeWidth(0);
        Path path = new Path();
        path.moveTo(c[5], c[6]);
        for (int i = 7; i < c.length; i += 2) {
            path.lineTo(c[i], c[i + 1]);
        }
        //global_path.close();
        draw_canvas.drawPath(path, paint);
    }

    protected void draw_rectangle(Paint paint, int x, int y, int w, int h) {
        Rect r = new Rect(x, y, x + w, y + h);
        paint.setStyle(Paint.Style.FILL);
        paint.setAntiAlias(true);
        //paint.setStrokeWidth(0);
        draw_canvas.drawRect(r, paint);
    }

    protected void draw_circle(Paint paint, int x, int y, int r) {
        paint.setStyle(Paint.Style.STROKE);
        draw_canvas.drawCircle(x, y, r / 2, paint);
    }

    protected void draw_text(Paint paint, int x, int y, String text, int size, int dx, int dy, int bgcolor) {
        int oldcolor=paint.getColor();
        Path path=null;

        paint.setTextSize(size / 15);
        paint.setStyle(Paint.Style.FILL);

        if (dx != 0x10000 || dy != 0) {
            path = new Path();
            path.moveTo(x, y);
            path.rLineTo(dx, dy);
            paint.setTextAlign(android.graphics.Paint.Align.LEFT);
        }

        if(bgcolor!=0) {
            paint.setStrokeWidth(3);
            paint.setColor(bgcolor);
            paint.setStyle(Paint.Style.STROKE);
            if(path==null) {
                draw_canvas.drawText(text, x, y, paint);
            } else {
                draw_canvas.drawTextOnPath(text, path, 0, 0, paint);
            }
            paint.setStyle(Paint.Style.FILL);
            paint.setColor(oldcolor);
        }

        if(path==null) {
            draw_canvas.drawText(text, x, y, paint);
        } else {
            draw_canvas.drawTextOnPath(text, path, 0, 0, paint);
        }
        paint.clearShadowLayer();
    }
    protected void draw_image(Paint paint, int x, int y, Bitmap bitmap) {
        draw_canvas.drawBitmap(bitmap, x, y, null);
    }

    /* takes an image and draws it on the screen as a prerendered maptile
     *
     *
     *
     * @param paint     Paint object used to draw the image
     * @param count     the number of points specified
     * @param p0x and p0y   specifying the top left point
     * @param p1x and p1y   specifying the top right point
     * @param p2x and p2y   specifying the bottom left point, not yet used but kept
     *                      for compatibility with the linux port
     * @param bitmap    Bitmap object holding the image to draw
     *
     * TODO make it work with 4 points specified to make it work for 3D mapview, so it can be used
     *      for small but very detailed maps as well as for large maps with very little detail but large
     *      coverage.
     * TODO make it work with rectangular tiles as well ?
     */
    protected void draw_image_warp(Paint paint, int count, int p0x, int p0y, int p1x, int p1y, int p2x, int p2y,
                                   Bitmap bitmap) {

        float width;
        float scale;
        float deltaY;
        float deltaX;
        float angle;
        Matrix matrix;

        if (count == 3) {
            matrix = new Matrix();
            deltaX = p1x - p0x;
            deltaY = p1y - p0y;
            width = (float) (Math.sqrt((deltaX * deltaX) + (deltaY * deltaY)));
            angle = (float) (Math.atan2(deltaY, deltaX) * 180d / Math.PI);
            scale = width / bitmap.getWidth();
            matrix.preScale(scale, scale);
            matrix.postTranslate(p0x, p0y);
            matrix.postRotate(angle, p0x, p0y);
            draw_canvas.drawBitmap(bitmap, matrix, paint);
        }
    }

    /* These constants must be synchronized with enum draw_mode_num in graphics.h. */
    private static final int draw_mode_begin = 0;
    private static final int draw_mode_end = 1;

    protected void draw_mode(int mode) {
        if (mode == draw_mode_end) {
            if (parent_graphics == null) {
                view.invalidate();
            } else {
                parent_graphics.view.invalidate(get_rect());
            }
        }
        if (mode == draw_mode_begin && parent_graphics != null) {
            draw_bitmap.eraseColor(0);
        }

    }
    protected void draw_drag(int x, int y) {
        pos_x = x;
        pos_y = y;
    }
    protected void overlay_disable(int disable) {
        Log.d(TAG,"overlay_disable: " + disable + "Parent: " + (parent_graphics != null));
        // assume we are NOT in map view mode!
        if (parent_graphics == null)
            in_map = (disable==0);
        if (overlay_disabled != disable) {
            overlay_disabled = disable;
            if (parent_graphics != null) {
                parent_graphics.view.invalidate(get_rect());
            }
        }
    }

    protected void overlay_resize(int x, int y, int w, int h, int wraparound) {
        draw_bitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
        bitmap_w = w;
        bitmap_h = h;
        pos_x = x;
        pos_y = y;
        pos_wraparound = wraparound;
        draw_canvas.setBitmap(draw_bitmap);
    }

    public static native String CallbackLocalizedString(String s);
}
