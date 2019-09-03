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
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
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
import android.view.WindowInsets;
import android.view.inputmethod.InputMethodManager;
import android.widget.FrameLayout;
import android.widget.RelativeLayout;
import java.io.File;
import java.lang.reflect.Method;
import java.util.ArrayList;


public class NavitGraphics {
    private static final String            TAG = "NavitGraphics";
    private final NavitGraphics            mNavitGraphics;
    private final ArrayList<NavitGraphics> mOverlays = new ArrayList<>();
    private int                            mBitmapWidth;
    private int                            mBitmapHeight;
    private int                            mPosX;
    private int                            mPosY;
    private int                            mPosWraparound;
    private int                            mOverlayDisabled;
    private int                            mBgColor;
    private float                          mTrackballX;
    private float                          mTrackballY;
    private int                            mPaddingLeft = 0;
    private int                            mPaddingRight = 0;
    private int                            mPaddingTop = 0;
    private int                            mPaddingBottom = 0;
    private View                           mView;
    private SystemBarTintView              mLeftTintView;
    private SystemBarTintView              mRightTintView;
    private SystemBarTintView              mTopTintView;
    private SystemBarTintView              mBottomTintView;
    private FrameLayout                    mFrameLayout;
    private RelativeLayout                 mRelativeLayout;
    private NavitCamera                    mCamera = null;
    private Navit                          mActivity;
    private static Boolean                 mInMap = false;
    // for menu key
    private static final long              mTimeForLongPress = 300L;


    public void setBackgroundColor(int bgcolor) {
        this.mBgColor = bgcolor;
        if (mLeftTintView != null) {
            mLeftTintView.setBackgroundColor(bgcolor);
        }
        if (mRightTintView != null) {
            mRightTintView.setBackgroundColor(bgcolor);
        }
        if (mTopTintView != null) {
            mTopTintView.setBackgroundColor(bgcolor);
        }
        if (mBottomTintView != null) {
            mBottomTintView.setBackgroundColor(bgcolor);
        }
    }

    private void setCamera(int useCamera) {
        if (useCamera != 0 && mCamera == null) {
            // mActivity.requestWindowFeature(Window.FEATURE_NO_TITLE);
            addCamera();
            addCameraView();
        }
    }

    /**
     * @brief Adds a camera.
     *
     * This method does not create the view for the camera. This must be done separately by calling
     * {@link #addCameraView()}.
     */
    private void addCamera() {
        mCamera = new NavitCamera(mActivity);
    }

    /**
     * @brief Adds a view for the camera.
     *
     * If {@link #mCamera} is null, this method is a no-op.
     */
    private void addCameraView() {
        if (mCamera != null) {
            mRelativeLayout.addView(mCamera);
            mRelativeLayout.bringChildToFront(mView);
        }
    }

    private Rect get_rect() {
        Rect ret = new Rect();
        ret.left = mPosX;
        ret.top = mPosY;
        if (mPosWraparound != 0) {
            if (ret.left < 0) {
                ret.left += mNavitGraphics.mBitmapWidth;
            }
            if (ret.top < 0) {
                ret.top += mNavitGraphics.mBitmapHeight;
            }
        }
        ret.right = ret.left + mBitmapWidth;
        ret.bottom = ret.top + mBitmapHeight;
        if (mPosWraparound != 0) {
            if (mBitmapWidth < 0) {
                ret.right = ret.left + mBitmapWidth + mNavitGraphics.mBitmapWidth;
            }
            if (mBitmapHeight < 0) {
                ret.bottom = ret.top + mBitmapHeight + mNavitGraphics.mBitmapHeight;
            }
        }
        return ret;
    }

    private class NavitView extends View implements Runnable, MenuItem.OnMenuItemClickListener {
        int               mTouchMode = NONE;
        float             mOldDist = 0;
        static final int  NONE       = 0;
        static final int  DRAG       = 1;
        static final int  ZOOM       = 2;
        static final int  PRESSED    = 3;

        Method eventGetX = null;
        Method eventGetY = null;

        PointF mPressedPosition = null;

        public NavitView(Context context) {
            super(context);
            try {
                eventGetX = android.view.MotionEvent.class.getMethod("getX", int.class);
                eventGetY = android.view.MotionEvent.class.getMethod("getY", int.class);
            } catch (Exception e) {
                Log.d(TAG, "Multitouch zoom not supported");
            }
        }

        @Override
        @TargetApi(20)
        public WindowInsets onApplyWindowInsets(WindowInsets insets) {
            /*
             * We're skipping the top inset here because it appears to have a bug on most Android versions tested,
             * causing it to report between 24 and 64 dip more than what is actually occupied by the system UI.
             * The top inset is retrieved in handleResize(), with logic depending on the platform version.
             */
            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.KITKAT_WATCH
            //        && android.os.Build.VERSION.SDK_INT < Build.VERSION_CODES.O
            ) {
                mPaddingLeft = insets.getSystemWindowInsetLeft();
                mPaddingRight = insets.getSystemWindowInsetRight();
                mPaddingBottom = insets.getSystemWindowInsetBottom();
            }
            return super.onApplyWindowInsets(insets);
        }

        @Override
        protected void onCreateContextMenu(ContextMenu menu) {
            super.onCreateContextMenu(menu);

            menu.setHeaderTitle(mActivity.getTstring(R.string.position_popup_title) + "..");
            menu.add(1, 1, NONE, mActivity.getTstring(R.string.position_popup_drive_here))
                    .setOnMenuItemClickListener(this);
            menu.add(1, 2, NONE, mActivity.getTstring(R.string.cancel)).setOnMenuItemClickListener(this);
        }

        @Override
        public boolean onMenuItemClick(MenuItem item) {
            switch (item.getItemId()) {
                case 1:
                    Message msg = Message.obtain(mCallbackHandler, msgType.CLB_SET_DISPLAY_DESTINATION.ordinal(),
                            (int)mPressedPosition.x, (int)mPressedPosition.y);
                    msg.sendToTarget();
                    break;
            }
            return false;
        }


        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            canvas.drawBitmap(mDrawBitmap, mPosX, mPosY, null);
            if (mOverlayDisabled == 0) {
                // assume we ARE in map view mode!
                mInMap = true;
                for (NavitGraphics overlay : mOverlays) {
                    if (overlay.mOverlayDisabled == 0) {
                        Rect r = overlay.get_rect();
                        canvas.drawBitmap(overlay.mDrawBitmap, r.left, r.top, null);
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
            Log.d(TAG, "onSizeChanged scaledDensity=" + Navit.metrics.scaledDensity);
            super.onSizeChanged(w, h, oldw, oldh);

            handleResize(w, h);
        }

        void do_longpress_action() {
            Log.d(TAG, "do_longpress_action enter");

            mActivity.openContextMenu(this);
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

            final int ACTION_POINTER_UP = getActionField("ACTION_POINTER_UP", event);
            final int ACTION_POINTER_DOWN = getActionField("ACTION_POINTER_DOWN", event);
            final int ACTION_MASK = getActionField("ACTION_MASK", event);

            int switch_value = event.getAction();
            if (ACTION_MASK != -999) {
                switch_value = (event.getAction() & ACTION_MASK);
            }

            if (switch_value == MotionEvent.ACTION_DOWN) {
                mTouchMode = PRESSED;
                if (!mInMap) {
                    buttonCallback(mButtonCallbackID, 1, 1, x, y); // down
                }
                mPressedPosition = new PointF(x, y);
                postDelayed(this, mTimeForLongPress);
            } else if ((switch_value == MotionEvent.ACTION_UP) || (switch_value == ACTION_POINTER_UP)) {
                Log.d(TAG, "ACTION_UP");

                switch (mTouchMode) {
                    case DRAG:
                        Log.d(TAG, "onTouch move");

                        motionCallback(mMotionCallbackID, x, y);
                        buttonCallback(mButtonCallbackID, 0, 1, x, y); // up

                        break;
                    case ZOOM:
                        float newDist = spacing(getFloatValue(event, 0), getFloatValue(event, 1));
                        float scale = 0;
                        if (newDist > 10f) {
                            scale = newDist / mOldDist;
                        }

                        if (scale > 1.3) {
                            // zoom in
                            callbackMessageChannel(1, null);
                        } else if (scale < 0.8) {
                            // zoom out
                            callbackMessageChannel(2, null);
                        }
                        break;
                    case PRESSED:
                        if (mInMap) {
                            buttonCallback(mButtonCallbackID, 1, 1, x, y); // down
                        }
                        buttonCallback(mButtonCallbackID, 0, 1, x, y); // up

                        break;
                }
                mTouchMode = NONE;
            } else if (switch_value == MotionEvent.ACTION_MOVE) {
                switch (mTouchMode) {
                    case DRAG:
                        motionCallback(mMotionCallbackID, x, y);
                        break;
                    case ZOOM:
                        float newDist = spacing(getFloatValue(event, 0), getFloatValue(event, 1));
                        float scale = newDist / mOldDist;
                        Log.d(TAG, "New scale = " + scale);
                        if (scale > 1.2) {
                            // zoom in
                            callbackMessageChannel(1, "");
                            mOldDist = newDist;
                        } else if (scale < 0.8) {
                            mOldDist = newDist;
                            // zoom out
                            callbackMessageChannel(2, "");
                        }
                        break;
                    case PRESSED:
                        Log.d(TAG, "Start drag mode");
                        if (spacing(mPressedPosition, new PointF(event.getX(), event.getY())) > 20f) {
                            buttonCallback(mButtonCallbackID, 1, 1, x, y); // down
                            mTouchMode = DRAG;
                        }
                        break;
                }
            } else if (switch_value == ACTION_POINTER_DOWN) {
                mOldDist = spacing(getFloatValue(event, 0), getFloatValue(event, 1));
                if (mOldDist > 2f) {
                    mTouchMode = ZOOM;
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
                        if (!mInMap) {
                            // if last menukeypress is less than 0.2 seconds away then count longpress
                            if ((System.currentTimeMillis() - Navit.last_pressed_menu_key) < interval_for_long_press) {
                                Navit.time_pressed_menu_key = Navit.time_pressed_menu_key
                                    + (System.currentTimeMillis() - Navit.last_pressed_menu_key);
                                // on long press let softkeyboard popup
                                if (Navit.time_pressed_menu_key > mTimeForLongPress) {
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
                        if (mInMap) {
                            return false;
                        }

                        s = String.valueOf((char) 19);
                        break;
                    case KeyEvent.KEYCODE_BACK:
                        s = String.valueOf((char) 27);
                        break;
                    case KeyEvent.KEYCODE_CALL:
                        s = String.valueOf((char) 3);
                        break;
                    case KeyEvent.KEYCODE_VOLUME_UP:
                        if (!mInMap) {
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
                        if (!mInMap) {
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
                keypressCallback(mKeypressCallbackID, s);
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
                        return (!mInMap);
                    case KeyEvent.KEYCODE_VOLUME_DOWN:
                        return (!mInMap);
                    case KeyEvent.KEYCODE_SEARCH:
                        /* Handle event in Main Activity if map is shown */
                        if (mInMap) {
                            return false;
                        }
                        break;
                    case KeyEvent.KEYCODE_BACK:
                        if (Navit.show_soft_keyboard_now_showing) {
                            Navit.show_soft_keyboard_now_showing = false;
                        }
                        //s = java.lang.String.valueOf((char) 27);
                        return true;
                    case KeyEvent.KEYCODE_MENU:
                        if (!mInMap) {
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
            } else if (i != 10) {
                s = java.lang.String.valueOf((char) i);
            }

            if (s != null) {
                keypressCallback(mKeypressCallbackID, s);
            }
            return true;
        }

        @Override
        public boolean onKeyMultiple(int keyCode, int count, KeyEvent event) {
            String s;
            if (keyCode == KeyEvent.KEYCODE_UNKNOWN) {
                s = event.getCharacters();
                keypressCallback(mKeypressCallbackID, s);
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
                mTrackballX += event.getX();
                mTrackballY += event.getY();
                if (mTrackballX <= -1) {
                    s = java.lang.String.valueOf((char) 2);
                    mTrackballX += 1;
                }
                if (mTrackballX >= 1) {
                    s = java.lang.String.valueOf((char) 6);
                    mTrackballX -= 1;
                }
                if (mTrackballY <= -1) {
                    s = java.lang.String.valueOf((char) 16);
                    mTrackballY += 1;
                }
                if (mTrackballY >= 1) {
                    s = java.lang.String.valueOf((char) 14);
                    mTrackballY -= 1;
                }
            }
            if (s != null) {
                keypressCallback(mKeypressCallbackID, s);
            }
            return true;
        }

        public void run() {
            if (mInMap && mTouchMode == PRESSED) {
                do_longpress_action();
                mTouchMode = NONE;
            }
        }

    }

    private class SystemBarTintView extends View {

        public SystemBarTintView(Context context) {
            super(context);
            this.setBackgroundColor(mBgColor);
        }

    }

    public NavitGraphics(final Activity activity, NavitGraphics parent, int x, int y, int w, int h,
                         int wraparound, int useCamera) {
        if (parent == null) {
            if (useCamera != 0) {
                addCamera();
            }
            setmActivity(activity);
        } else {
            mDrawBitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
            mBitmapWidth = w;
            mBitmapHeight = h;
            mPosX = x;
            mPosY = y;
            mPosWraparound = wraparound;
            mDrawCanvas = new Canvas(mDrawBitmap);
            parent.mOverlays.add(this);
        }
        mNavitGraphics = parent;
    }

    /**
     * @brief Sets up the main view.
     *
     * @param activity The main activity.
     */
    protected void setmActivity(final Activity activity) {
        if (Navit.graphics == null) {
            Navit.graphics = this;
        }
        this.mActivity = (Navit) activity;
        mView = new NavitView(mActivity);
        mView.setClickable(false);
        mView.setFocusable(true);
        mView.setFocusableInTouchMode(true);
        mView.setKeepScreenOn(true);
        mRelativeLayout = new RelativeLayout(mActivity);
        addCameraView();
        mRelativeLayout.addView(mView);

        /* The navigational and status bar tinting code is meaningful only on API19+ */
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT
 //               && android.os.Build.VERSION.SDK_INT < Build.VERSION_CODES.O
                       ) {
            mFrameLayout = new FrameLayout(mActivity);
            mFrameLayout.addView(mRelativeLayout);
            mLeftTintView = new SystemBarTintView(mActivity);
            mRightTintView = new SystemBarTintView(mActivity);
            mTopTintView = new SystemBarTintView(mActivity);
            mBottomTintView = new SystemBarTintView(mActivity);
            mFrameLayout.addView(mLeftTintView);
            mFrameLayout.addView(mRightTintView);
            mFrameLayout.addView(mTopTintView);
            mFrameLayout.addView(mBottomTintView);
            mActivity.setContentView(mFrameLayout);
        } else {
            mActivity.setContentView(mRelativeLayout);
        }

        mView.requestFocus();
    }

    enum msgType {
        CLB_ZOOM_IN, CLB_ZOOM_OUT, CLB_REDRAW, CLB_MOVE, CLB_BUTTON_UP, CLB_BUTTON_DOWN, CLB_SET_DESTINATION,
        CLB_SET_DISPLAY_DESTINATION, CLB_CALL_CMD, CLB_COUNTRY_CHOOSER, CLB_LOAD_MAP, CLB_UNLOAD_MAP, CLB_DELETE_MAP
    }

    private static final msgType[] msg_values = msgType.values();

    public final Handler mCallbackHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg_values[msg.what]) {
                case CLB_ZOOM_IN:
                    callbackMessageChannel(1, "");
                    break;
                case CLB_ZOOM_OUT:
                    callbackMessageChannel(2, "");
                    break;
                case CLB_MOVE:
                    motionCallback(mMotionCallbackID, msg.getData().getInt("x"), msg.getData().getInt("y"));
                    break;
                case CLB_SET_DESTINATION:
                    String lat = Float.toString(msg.getData().getFloat("lat"));
                    String lon = Float.toString(msg.getData().getFloat("lon"));
                    String q = msg.getData().getString(("q"));
                    callbackMessageChannel(3, lat + "#" + lon + "#" + q);
                    break;
                case CLB_SET_DISPLAY_DESTINATION:
                    int x = msg.arg1;
                    int y = msg.arg2;
                    callbackMessageChannel(4, "" + x + "#" + y);
                    break;
                case CLB_CALL_CMD:
                    String cmd = msg.getData().getString(("cmd"));
                    callbackMessageChannel(5, cmd);
                    break;
                case CLB_BUTTON_UP:
                    buttonCallback(mButtonCallbackID, 0, 1, msg.getData().getInt("x"), msg.getData().getInt("y")); // up
                    break;
                case CLB_BUTTON_DOWN:
                    // down
                    buttonCallback(mButtonCallbackID, 1, 1, msg.getData().getInt("x"), msg.getData().getInt("y"));
                    break;
                case CLB_COUNTRY_CHOOSER:
                    break;
                case CLB_LOAD_MAP:
                    callbackMessageChannel(6, msg.getData().getString(("title")));
                    break;
                case CLB_DELETE_MAP:
                    File toDelete = new File(msg.getData().getString(("title")));
                    toDelete.delete();
                    //fallthrough
                case CLB_UNLOAD_MAP:
                    callbackMessageChannel(7, msg.getData().getString(("title")));
                    break;
            }
        }
    };

    public native void sizeChangedCallback(long id, int x, int y);

    public native void paddingChangedCallback(long id, int left, int right, int top, int bottom);

    public native void keypressCallback(long id, String s);

    public native int callbackMessageChannel(int i, String s);

    public native void buttonCallback(long id, int pressed, int button, int x, int y);

    public native void motionCallback(long id, int x, int y);

    public native String getDefaultCountry(int id, String s);

    public static native String[][] getAllCountries();

    private Canvas mDrawCanvas;
    private Bitmap mDrawBitmap;
    private long mSizeChangedCallbackID;
    private long mPaddingChangedCallbackID;
    private long mButtonCallbackID;
    private long mMotionCallbackID;
    private long mKeypressCallbackID;

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

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) {
            return;
        }

        /* hide tint bars during update to prevent ugly effects */
        mLeftTintView.setVisibility(View.GONE);
        mRightTintView.setVisibility(View.GONE);
        mTopTintView.setVisibility(View.GONE);
        mBottomTintView.setVisibility(View.GONE);

        mFrameLayout.post(new Runnable() {
            @Override
            public void run() {
                FrameLayout.LayoutParams leftLayoutParams = new FrameLayout.LayoutParams(mPaddingLeft,
                        LayoutParams.MATCH_PARENT, Gravity.BOTTOM | Gravity.LEFT);
                mLeftTintView.setLayoutParams(leftLayoutParams);

                FrameLayout.LayoutParams rightLayoutParams = new FrameLayout.LayoutParams(mPaddingRight,
                        LayoutParams.MATCH_PARENT, Gravity.BOTTOM | Gravity.RIGHT);
                mRightTintView.setLayoutParams(rightLayoutParams);

                FrameLayout.LayoutParams topLayoutParams = new FrameLayout.LayoutParams(LayoutParams.MATCH_PARENT,
                        mPaddingTop, Gravity.TOP);
                /* Prevent horizontal and vertical tint views from overlapping */
                topLayoutParams.setMargins(mPaddingLeft, 0, mPaddingRight, 0);
                mTopTintView.setLayoutParams(topLayoutParams);

                FrameLayout.LayoutParams bottomLayoutParams = new FrameLayout.LayoutParams(LayoutParams.MATCH_PARENT,
                        mPaddingBottom, Gravity.BOTTOM);
                /* Prevent horizontal and vertical tint views from overlapping */
                bottomLayoutParams.setMargins(mPaddingLeft, 0, mPaddingRight, 0);
                mBottomTintView.setLayoutParams(bottomLayoutParams);

                /* show tint bars again */
                mLeftTintView.setVisibility(View.VISIBLE);
                mRightTintView.setVisibility(View.VISIBLE);
                mTopTintView.setVisibility(View.VISIBLE);
                mBottomTintView.setVisibility(View.VISIBLE);
            }
        });

        paddingChangedCallback(mPaddingChangedCallbackID, mPaddingLeft, mPaddingTop, mPaddingRight, mPaddingBottom);
    }

    /**
     * @brief Handles resize events.
     *
     * This method is called whenever the main View is resized in any way. This is the case when its
     * {@code onSizeChanged()} event handler fires or when toggling Fullscreen mode.
     *
     */
    @TargetApi(23)
    void handleResize(int w, int h) {
        if (this.mNavitGraphics != null) {
            this.mNavitGraphics.handleResize(w, h);
        } else {
            Log.d(TAG, String.format("handleResize w=%d h=%d", w, h));

            if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
                /*
                 * On API 23+ we can query window insets to determine the area which is obscured by the system bars.
                 * This appears to have a bug, though, causing an inset to be reported for the navigation bar even
                 * when it is not obstructing the window. Therefore, we are relying on the values previously obtained
                 * by NavitView#onApplyWindowInsets(), though this is affected by a different bug. Luckily, the two
                 * bugs appear to be complementary, allowing us to mix and match results.
                 */
                if (mView == null) {
                    Log.w(TAG, "view is null, cannot update padding");
                } else {
                    Log.d(TAG, String.format("view w=%d h=%d x=%.0f y=%.0f",
                            mView.getWidth(), mView.getHeight(), mView.getX(), mView.getY()));
                    if (mView.getRootWindowInsets() == null) {
                        Log.w(TAG, "No root window insets, cannot update padding");
                    } else {
                        Log.d(TAG, String.format("RootWindowInsets left=%d right=%d top=%d bottom=%d",
                                mView.getRootWindowInsets().getSystemWindowInsetLeft(),
                                mView.getRootWindowInsets().getSystemWindowInsetRight(),
                                mView.getRootWindowInsets().getSystemWindowInsetTop(),
                                mView.getRootWindowInsets().getSystemWindowInsetBottom()));
                        mPaddingTop = mView.getRootWindowInsets().getSystemWindowInsetTop();
                    }
                }
            } else if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.KITKAT_WATCH) {
                /*
                 * API 20-22 do not support root window insets, forcing us to make an educated guess about the
                 * navigation bar height:
                 *
                 * The size is a platform default and does not change with rotation, but we have to figure out if it
                 * applies, i.e. if the status bar is visible.
                 *
                 * The status bar is always visible unless we are in fullscreen mode. (Fortunately, none of the
                 * versions affected by this support split screen mode, which would have further complicated things.)
                 */
                if (mActivity.mIsFullscreen) {
                    mPaddingTop = 0;
                } else {
                    Resources resources = mView.getResources();
                    int shid = resources.getIdentifier("status_bar_height", "dimen", "android");
                    mPaddingTop = (shid > 0) ? resources.getDimensionPixelSize(shid) : 0;
                }
            } else if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.KITKAT
                    //&& android.os.Build.VERSION.SDK_INT < Build.VERSION_CODES.O
            ) {
                /*
                 * API 19 does not support window insets at all, forcing us to do even more guessing than on API 20-22:
                 *
                 * All system bar sizes are platform defaults and do not change with rotation, but we have
                 * to figure out which ones apply.
                 *
                 * Status bar visibility is as on API 20-22.
                 *
                 * The navigation bar is shown on devices that report they have no physical menu button. This seems to
                 * work even on devices that allow disabling the physical buttons (and use the navigation bar, in which
                 * case they report no physical menu button is available; tested with a OnePlus One running CyanogenMod)
                 *
                 * If shown, the navigation bar may appear on the side or at the bottom. The logic to determine this is
                 * taken from AOSP RenderSessionImpl.findNavigationBar()
                 * platform/frameworks/base/tools/layoutlib/bridge/src/com/android/
                 * layoutlib/bridge/impl/RenderSessionImpl.java
                 */
                Resources resources = mView.getResources();
                int shid = resources.getIdentifier("statusBarHeight", "dimen", "android");
                int adhid = resources.getIdentifier("actionBarDefaultHeight", "dimen", "android");
                int nhid = resources.getIdentifier("navigationBarHeight", "dimen", "android");
                int nhlid = resources.getIdentifier("navigationBarHeightLandscape", "dimen", "android");
                int nwid = resources.getIdentifier("navigationBarWidth", "dimen", "android");
                int statusBarHeight = (shid > 0) ? resources.getDimensionPixelSize(shid) : 0;
                int actionBarDefaultHeight = (adhid > 0) ? resources.getDimensionPixelSize(adhid) : 0;
                int navigationBarHeight = (nhid > 0) ? resources.getDimensionPixelSize(nhid) : 0;
                int navigationBarHeightLandscape = (nhlid > 0) ? resources.getDimensionPixelSize(nhlid) : 0;
                int navigationBarWidth = (nwid > 0) ? resources.getDimensionPixelSize(nwid) : 0;
                Log.d(TAG, String.format(
                        "statusBarHeight=%d, actionBarDefaultHeight=%d, navigationBarHeight=%d, "
                                + "navigationBarHeightLandscape=%d, navigationBarWidth=%d",
                                statusBarHeight, actionBarDefaultHeight, navigationBarHeight,
                                navigationBarHeightLandscape, navigationBarWidth));

                if (mActivity == null) {
                    Log.w(TAG, "Main Activity is not a Navit instance, cannot update padding");
                } else if (mFrameLayout != null) {
                    /* mFrameLayout is only created on platforms supporting navigation and status bar tinting */

                    Navit navit = mActivity;
                    boolean isStatusShowing = !navit.mIsFullscreen;
                    boolean isNavShowing = !ViewConfigurationCompat.hasPermanentMenuKey(ViewConfiguration.get(navit));
                    Log.d(TAG, String.format("isStatusShowing=%b isNavShowing=%b", isStatusShowing, isNavShowing));

                    boolean isLandscape = (navit.getResources().getConfiguration().orientation
                            == Configuration.ORIENTATION_LANDSCAPE);
                    boolean isNavAtBottom = (!isLandscape)
                            || (navit.getResources().getConfiguration().smallestScreenWidthDp >= 600);
                    Log.d(TAG, String.format("isNavAtBottom=%b (Configuration.smallestScreenWidthDp=%d, isLandscape=%b)",
                            isNavAtBottom, navit.getResources().getConfiguration().smallestScreenWidthDp, isLandscape));

                    mPaddingLeft = 0;
                    mPaddingTop = isStatusShowing ? statusBarHeight : 0;
                    mPaddingRight = (isNavShowing && !isNavAtBottom) ? navigationBarWidth : 0;
                    mPaddingBottom = (!(isNavShowing && isNavAtBottom)) ? 0 : (
                            isLandscape ? navigationBarHeightLandscape : navigationBarHeight);
                }
            } else {
                /* API 18 and below does not support drawing under the system bars, padding is 0 all around */
                mPaddingLeft = 0;
                mPaddingRight = 0;
                mPaddingTop = 0;
                mPaddingBottom = 0;
            }

            Log.d(TAG, String.format("Padding left=%d top=%d right=%d bottom=%d",
                    mPaddingLeft, mPaddingTop, mPaddingRight, mPaddingBottom));

            adjustSystemBarsTintingViews();

            mDrawBitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
            mDrawCanvas = new Canvas(mDrawBitmap);
            mBitmapWidth = w;
            mBitmapHeight = h;
            sizeChangedCallback(mSizeChangedCallbackID, w, h);
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
        if (Build.VERSION.SDK_INT <= 10) {
            return true;
        } else {
            if (Build.VERSION.SDK_INT <= 13) {
                return false;
            } else {
                return ViewConfiguration.get(mActivity.getApplication()).hasPermanentMenuKey();
            }
        }
    }

    public void setSizeChangedCallback(long id) {
        mSizeChangedCallbackID = id;
    }

    public void setPaddingChangedCallback(long id) {
        mPaddingChangedCallbackID = id;
    }

    public void setButtonCallback(long id) {
        mButtonCallbackID = id;
    }

    public void setMotionCallback(long id) {
        mMotionCallbackID = id;
        if (mActivity != null) {
            mActivity.setGraphics(this);
        }
    }

    public void setKeypressCallback(long id) {
        mKeypressCallbackID = id;
    }


    protected void draw_polyline(Paint paint, int[] c) {
        paint.setStrokeWidth(c[0]);
        paint.setARGB(c[1],c[2],c[3],c[4]);
        paint.setStyle(Paint.Style.STROKE);
        //paint.setAntiAlias(true);
        //paint.setStrokeWidth(0);
        int ndashes = c[5];
        float[] intervals = new float[ndashes + (ndashes % 2)];
        for (int i = 0; i < ndashes; i++) {
            intervals[i] = c[6 + i];
        }

        if ((ndashes % 2) == 1) {
            intervals[ndashes] = intervals[ndashes - 1];
        }

        if (ndashes > 0) {
            paint.setPathEffect(new android.graphics.DashPathEffect(intervals,0.0f));
        }

        Path path = new Path();
        path.moveTo(c[6 + ndashes], c[7 + ndashes]);
        for (int i = 8 + ndashes; i < c.length; i += 2) {
            path.lineTo(c[i], c[i + 1]);
        }
        //global_path.close();
        mDrawCanvas.drawPath(path, paint);
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
        mDrawCanvas.drawPath(path, paint);
    }

    protected void draw_rectangle(Paint paint, int x, int y, int w, int h) {
        Rect r = new Rect(x, y, x + w, y + h);
        paint.setStyle(Paint.Style.FILL);
        paint.setAntiAlias(true);
        //paint.setStrokeWidth(0);
        mDrawCanvas.drawRect(r, paint);
    }

    protected void draw_circle(Paint paint, int x, int y, int r) {
        paint.setStyle(Paint.Style.STROKE);
        mDrawCanvas.drawCircle(x, y, r / 2, paint);
    }

    protected void draw_text(Paint paint, int x, int y, String text, int size, int dx, int dy, int bgcolor) {
        int oldcolor = paint.getColor();
        Path path = null;

        paint.setTextSize(size / 15);
        paint.setStyle(Paint.Style.FILL);

        if (dx != 0x10000 || dy != 0) {
            path = new Path();
            path.moveTo(x, y);
            path.rLineTo(dx, dy);
            paint.setTextAlign(android.graphics.Paint.Align.LEFT);
        }

        if (bgcolor != 0) {
            paint.setStrokeWidth(3);
            paint.setColor(bgcolor);
            paint.setStyle(Paint.Style.STROKE);
            if (path == null) {
                mDrawCanvas.drawText(text, x, y, paint);
            } else {
                mDrawCanvas.drawTextOnPath(text, path, 0, 0, paint);
            }
            paint.setStyle(Paint.Style.FILL);
            paint.setColor(oldcolor);
        }

        if (path == null) {
            mDrawCanvas.drawText(text, x, y, paint);
        } else {
            mDrawCanvas.drawTextOnPath(text, path, 0, 0, paint);
        }
        paint.clearShadowLayer();
    }

    protected void draw_image(Paint paint, int x, int y, Bitmap bitmap) {
        mDrawCanvas.drawBitmap(bitmap, x, y, null);
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
            mDrawCanvas.drawBitmap(bitmap, matrix, paint);
        }
    }

    /* These constants must be synchronized with enum draw_mode_num in graphics.h. */
    private static final int draw_mode_begin = 0;
    private static final int draw_mode_end = 1;

    protected void draw_mode(int mode) {
        if (mode == draw_mode_end) {
            if (mNavitGraphics == null) {
                mView.invalidate();
            } else {
                mNavitGraphics.mView.invalidate(get_rect());
            }
        }
        if (mode == draw_mode_begin && mNavitGraphics != null) {
            mDrawBitmap.eraseColor(0);
        }

    }

    protected void draw_drag(int x, int y) {
        mPosX = x;
        mPosY = y;
    }

    protected void overlay_disable(int disable) {
        Log.d(TAG,"overlay_disable: " + disable + "Parent: " + (mNavitGraphics != null));
        // assume we are NOT in map view mode!
        if (mNavitGraphics == null) {
            mInMap = (disable == 0);
        }
        if (mOverlayDisabled != disable) {
            mOverlayDisabled = disable;
            if (mNavitGraphics != null) {
                mNavitGraphics.mView.invalidate(get_rect());
            }
        }
    }

    protected void overlay_resize(int x, int y, int w, int h, int wraparound) {
        mDrawBitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
        mBitmapWidth = w;
        mBitmapHeight = h;
        mPosX = x;
        mPosY = y;
        mPosWraparound = wraparound;
        mDrawCanvas.setBitmap(mDrawBitmap);
    }

    public static native String callbackLocalizedString(String s);
}
