/*
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

import static org.navitproject.navit.NavitAppConfig.getTstring;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PointF;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.support.annotation.RequiresApi;
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
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.FrameLayout;
import android.widget.RelativeLayout;

import java.util.ArrayList;
import java.util.List;


class NavitGraphics {
    private static final String            TAG = "NavitGraphics";
    private static final long              TIME_FOR_LONG_PRESS = 300L;
    private final NavitGraphics            mParentGraphics;
    private final ArrayList<NavitGraphics> mOverlays;
    private int                            mBitmapWidth;
    private int                            mBitmapHeight;
    private int                            mPosX;
    private int                            mPosY;
    private int                            mPosWraparound;
    private int                            mOverlayDisabled;
    private int                            mBgColor;
    private float                          mTrackballX;
    private float                          mTrackballY;
    private int                            mPaddingLeft;
    private int                            mPaddingRight;
    private int                            mPaddingTop;
    private int                            mPaddingBottom;
    private NavitView                      mView;
    static final Handler                   sCallbackHandler = new CallBackHandler();
    private SystemBarTintView              mLeftTintView;
    private SystemBarTintView              mRightTintView;
    private SystemBarTintView              mTopTintView;
    private SystemBarTintView              mBottomTintView;
    private FrameLayout                    mFrameLayout;
    private RelativeLayout                 mRelativeLayout;
    private NavitCamera                    mCamera;
    private Navit                          mActivity;
    private static boolean                 sInMap;
    private boolean                        mTinting;


    void setBackgroundColor(int bgcolor) {
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
            mCamera = new NavitCamera(mActivity);
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
                ret.left += mParentGraphics.mBitmapWidth;
            }
            if (ret.top < 0) {
                ret.top += mParentGraphics.mBitmapHeight;
            }
        }
        ret.right = ret.left + mBitmapWidth;
        ret.bottom = ret.top + mBitmapHeight;
        if (mPosWraparound != 0) {
            if (mBitmapWidth < 0) {
                ret.right = ret.left + mBitmapWidth + mParentGraphics.mBitmapWidth;
            }
            if (mBitmapHeight < 0) {
                ret.bottom = ret.top + mBitmapHeight + mParentGraphics.mBitmapHeight;
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
        PointF mPressedPosition = null;

        NavitView(Context context) {
            super(context);
            // assumption usefull for the KitKat tinting only
            sInMap = true;
        }

        boolean isDrag() {
            return mTouchMode == DRAG;
        }

        public void onWindowFocusChanged(boolean hasWindowFocus) {
            Log.v(TAG,"onWindowFocusChanged = " + hasWindowFocus);
            if (Navit.sShowSoftKeyboardShowing && hasWindowFocus) {
                InputMethodManager imm  = (InputMethodManager) mActivity
                        .getSystemService(Context.INPUT_METHOD_SERVICE);
                imm.showSoftInput(this,InputMethodManager.SHOW_FORCED);
            }
            if (Navit.sShowSoftKeyboardShowing && !hasWindowFocus) {
                InputMethodManager imm  = (InputMethodManager) mActivity
                        .getSystemService(Context.INPUT_METHOD_SERVICE);
                imm.hideSoftInputFromWindow(this.getWindowToken(), 0);
            }
        }


        @TargetApi(21)
        public WindowInsets onApplyWindowInsets(WindowInsets insets) {
            Log.d(TAG,"onApplyWindowInsets");
            if (mTinting) {
                mPaddingLeft = insets.getSystemWindowInsetLeft();
                mPaddingRight = insets.getSystemWindowInsetRight();
                mPaddingBottom = insets.getSystemWindowInsetBottom();
                mPaddingTop = insets.getSystemWindowInsetTop();
                Log.v(TAG, String.format("Padding -1a- left=%d top=%d right=%d bottom=%d",
                        mPaddingLeft, mPaddingTop, mPaddingRight, mPaddingBottom));
                int width = this.getWidth();
                int height = this.getHeight();
                if (width > 0 && height > 0) {
                    adjustSystemBarsTintingViews();
                    sizeChangedCallback(mSizeChangedCallbackID, width, height);
                }
            }
            return insets;
        }

        private static final int MENU_DRIVE_HERE = 1;
        private static final int MENU_VIEW = 2;
        private static final int MENU_CANCEL = 3;

        @Override
        protected void onCreateContextMenu(ContextMenu menu) {
            super.onCreateContextMenu(menu);
            String clickCoord = getCoordForPoint((int)mPressedPosition.x, (int)mPressedPosition.y, false);
            menu.setHeaderTitle(NavitAppConfig.getTstring(R.string.position_popup_title) + " " + clickCoord);
            menu.add(1, MENU_DRIVE_HERE, NONE, NavitAppConfig.getTstring(R.string.position_popup_drive_here))
                    .setOnMenuItemClickListener(this);
            Uri intentUri = Uri.parse("geo:" + getCoordForPoint((int)mPressedPosition.x,
                    (int)mPressedPosition.y, true));
            Intent mContextMenuMapViewIntent = new Intent(Intent.ACTION_VIEW, intentUri);

            PackageManager packageManager = this.getContext().getPackageManager();
            List<ResolveInfo> activities = packageManager.queryIntentActivities(mContextMenuMapViewIntent,
                    PackageManager.MATCH_DEFAULT_ONLY);
            boolean isIntentSafe = (activities.size() > 0); // at least one candidate receiver
            if (isIntentSafe) { // add view with external app option
                menu.add(1, MENU_VIEW, NONE, NavitAppConfig.getTstring(R.string.position_popup_view))
                        .setOnMenuItemClickListener(this);
            } else {
                Log.w(TAG, "No application available to handle ACTION_VIEW intent, option not displayed");
            }
            menu.add(1, MENU_CANCEL, NONE, getTstring(R.string.cancel)).setOnMenuItemClickListener(this);
        }

        @Override
        public boolean onMenuItemClick(MenuItem item) {
            int itemId = item.getItemId();
            if (itemId == MENU_DRIVE_HERE) {
                Message msg = Message.obtain(sCallbackHandler, MsgType.CLB_SET_DISPLAY_DESTINATION.ordinal(),
                        (int) mPressedPosition.x, (int) mPressedPosition.y);
                msg.sendToTarget();
            } else if (itemId == MENU_VIEW) {
                Uri intentUri = Uri.parse("geo:" + getCoordForPoint((int) mPressedPosition.x,
                        (int) mPressedPosition.y, true));
                Intent mContextMenuMapViewIntent = new Intent(Intent.ACTION_VIEW, intentUri);
                mContextMenuMapViewIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
                if (mContextMenuMapViewIntent.resolveActivity(this.getContext().getPackageManager()) != null) {
                    this.getContext().startActivity(mContextMenuMapViewIntent);
                } else {
                    Log.w(TAG, "ACTION_VIEW intent is not handled by any application, discarding...");
                }
            }
            return true;
        }


        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            canvas.drawBitmap(mDrawBitmap, mPosX, mPosY, null);
            if (mOverlayDisabled == 0) {
                // assume we ARE in map view mode!
                sInMap = true;
                for (NavitGraphics overlay : mOverlays) {
                    if (overlay.mOverlayDisabled == 0) {
                        Rect r = overlay.get_rect();
                        canvas.drawBitmap(overlay.mDrawBitmap, r.left, r.top, null);
                    }
                }
            }
        }

        @Override
        protected void onSizeChanged(int w, int h, int oldw, int oldh) {
            Log.d(TAG, "onSizeChanged pixels x=" + w + " pixels y=" + h);
            Log.v(TAG, "onSizeChanged density=" + Navit.sMetrics.density);
            Log.v(TAG, "onSizeChanged scaledDensity=" + Navit.sMetrics.scaledDensity);
            super.onSizeChanged(w, h, oldw, oldh);
            mDrawBitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
            mDrawCanvas = new Canvas(mDrawBitmap);
            mBitmapWidth = w;
            mBitmapHeight = h;

            handleResize(w, h);
            sizeChangedCallback(mSizeChangedCallbackID, w, h);
        }

        void doLongpressAction() {
            Log.d(TAG, "doLongpressAction enter");
            mActivity.openContextMenu(this);
        }


        @SuppressLint("ClickableViewAccessibility")
        @Override
        public boolean onTouchEvent(MotionEvent event) {
            super.onTouchEvent(event);
            int x = (int) event.getX();
            int y = (int) event.getY();
            int switchValue = (event.getActionMasked());
            Log.d(TAG, "ACTION_ value =  " + switchValue);

            if (switchValue == MotionEvent.ACTION_DOWN) {
                mTouchMode = PRESSED;
                Log.d(TAG, "ACTION_DOWN mode PRESSED");
                if (!sInMap) {
                    buttonCallback(mButtonCallbackID, 1, 1, x, y); // down
                }
                mPressedPosition = new PointF(x, y);
                postDelayed(this, TIME_FOR_LONG_PRESS);

            } else if (switchValue == MotionEvent.ACTION_POINTER_DOWN) {
                mOldDist = spacing(event);
                if (mOldDist > 2f) {
                    mTouchMode = ZOOM;
                    Log.d(TAG, "ACTION_DOWN mode ZOOM started");
                }
            } else if (switchValue == MotionEvent.ACTION_UP) {
                Log.d(TAG, "ACTION_UP");
                switch (mTouchMode) {
                    case DRAG:
                        Log.d(TAG, "onTouch move");
                        motionCallback(mMotionCallbackID, x, y);
                        buttonCallback(mButtonCallbackID, 0, 1, x, y); // up
                        break;
                    case PRESSED:
                        if (sInMap) {
                            buttonCallback(mButtonCallbackID, 1, 1, x, y); // down
                        }
                        buttonCallback(mButtonCallbackID, 0, 1, x, y); // up
                        break;
                    default:
                        Log.i(TAG, "Unexpected touchmode: " + mTouchMode);
                }
                mTouchMode = NONE;
            } else if (switchValue == MotionEvent.ACTION_MOVE) {
                switch (mTouchMode) {
                    case DRAG:
                        motionCallback(mMotionCallbackID, x, y);
                        break;
                    case ZOOM:
                        doZoom(event);
                        break;
                    case PRESSED:
                        Log.d(TAG, "Start drag mode");
                        if (spacing(mPressedPosition, new PointF(event.getX(), event.getY())) > 20f) {
                            buttonCallback(mButtonCallbackID, 1, 1, x, y); // down
                            mTouchMode = DRAG;
                        }
                        break;
                    default:
                        Log.i(TAG, "Unexpected touchmode: " + mTouchMode);
                }
            }
            return true;
        }

        private void doZoom(MotionEvent event) {
            if (event.findPointerIndex(0) == -1 || event.findPointerIndex(1) == -1) {
                Log.e(TAG,"missing pointer");
                return;
            }
            float newDist = spacing(event);
            float scale;
            if (event.getActionMasked() == MotionEvent.ACTION_MOVE) {
                scale = newDist / mOldDist;
                Log.v(TAG, "New scale = " + scale);
                if (scale > 1.2) {
                    // zoom in
                    callbackMessageChannel(1, "");
                    mOldDist = newDist;
                } else if (scale < 0.8) {
                    mOldDist = newDist;
                    // zoom out
                    callbackMessageChannel(2, "");
                }
            }
        }

        private float spacing(MotionEvent event) {
            float x = event.getX(0) - event.getX(1);
            float y = event.getY(0) - event.getY(1);
            return (float) Math.sqrt(x * x + y * y);
        }

        private float spacing(PointF a, PointF b) {
            float x = a.x - b.x;
            float y = a.y - b.y;
            return (float)Math.sqrt(x * x + y * y);
        }

        @Override
        public boolean onKeyDown(int keyCode, KeyEvent event) {
            Log.d(TAG,"onkeydown = " + keyCode);
            String keyStr = null;
            switch (keyCode) {
                case KeyEvent.KEYCODE_ENTER:
                case KeyEvent.KEYCODE_DPAD_CENTER:
                    keyStr = String.valueOf((char) 13);
                    break;
                case KeyEvent.KEYCODE_DEL:
                    keyStr = String.valueOf((char) 8);
                    break;
                //case KeyEvent.KEYCODE_MENU:
                //    if (!sInMap) {
                //        this.postInvalidate();
                //        return true;
                //    }
                //    break;
                case KeyEvent.KEYCODE_SEARCH:
                    /* Handle event in Main Activity if map is shown */
                    if (!sInMap) {
                        keyStr = String.valueOf((char) 19);
                    }
                    break;
                case KeyEvent.KEYCODE_BACK:
                    keyStr = String.valueOf((char) 27);
                    break;
                case KeyEvent.KEYCODE_CALL:
                    keyStr = String.valueOf((char) 3);
                    break;
                case KeyEvent.KEYCODE_DPAD_DOWN:
                    keyStr = String.valueOf((char) 14);
                    break;
                case KeyEvent.KEYCODE_DPAD_LEFT:
                    keyStr = String.valueOf((char) 2);
                    break;
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                    keyStr = String.valueOf((char) 6);
                    break;
                case KeyEvent.KEYCODE_DPAD_UP:
                    keyStr = String.valueOf((char) 16);
                    break;
                default:
                    Log.v(TAG, "keycode: " + keyCode);
            }
            if (keyStr != null) {
                keypressCallback(mKeypressCallbackID, keyStr);
                return true;
            }
            return false;
        }

        @Override
        public boolean onKeyUp(int keyCode, KeyEvent event) {
            Log.d(TAG,"onkeyUp = " + keyCode);
            int i;
            String s = null;
            i = event.getUnicodeChar();

            if (i == 0) {
                switch (keyCode) {
                    case KeyEvent.KEYCODE_VOLUME_UP:
                    case KeyEvent.KEYCODE_VOLUME_DOWN:
                        return (!sInMap);
                    case KeyEvent.KEYCODE_SEARCH:
                        /* Handle event in Main Activity if map is shown */
                        if (sInMap) {
                            return false;
                        }
                        break;
                    case KeyEvent.KEYCODE_BACK:
                        if (Navit.sShowSoftKeyboardShowing) {
                            Navit.sShowSoftKeyboardShowing = false;
                        }
                        //s = java.lang.String.valueOf((char) 27);
                        return true;
                    case KeyEvent.KEYCODE_MENU:
                        if (!sInMap) {
                            if (!Navit.sShowSoftKeyboardShowing) {
                                // if in menu view:
                                // use as OK (Enter) key
                                s = String.valueOf((char) 13);
                            } // if soft keyboard showing on screen, dont use menu button as select key
                        } else {
                            return false;
                        }
                        break;
                    default:
                        Log.v(TAG, "keycode: " + keyCode);
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
            if (sInMap && mTouchMode == PRESSED) {
                doLongpressAction();
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

    NavitGraphics(final Activity navit, NavitGraphics parent, int x, int y, int w, int h,
                         int wraparound, int useCamera) {
        if (parent == null) {
            mOverlays = new ArrayList<>();
            if (useCamera != 0) {
                setCamera(useCamera);
            }
            setmActivity((Navit)navit);
        } else {
            mOverlays = null;
            mDrawBitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
            mBitmapWidth = w;
            mBitmapHeight = h;
            mPosX = x;
            mPosY = y;
            mPosWraparound = wraparound;
            mDrawCanvas = new Canvas(mDrawBitmap);
            parent.mOverlays.add(this);
        }
        mParentGraphics = parent;
    }

    /**
     * Sets up the main view.
     * @param navit The main activity.
     */
    private void setmActivity(final Navit navit) {
        this.mActivity = navit;
        mView = new NavitView(mActivity);
        mView.setClickable(false);
        mView.setFocusable(true);
        mView.setFocusableInTouchMode(true);
        mView.setKeepScreenOn(true);
        mRelativeLayout = new RelativeLayout(mActivity);
        mRelativeLayout.addView(mView);
        /* The navigational and status bar tinting code is meaningful only on API19+ */
        mTinting = Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT;
        if (mTinting) {
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

    enum MsgType {
        CLB_ZOOM_IN, CLB_ZOOM_OUT, CLB_REDRAW, CLB_MOVE, CLB_BUTTON_UP, CLB_BUTTON_DOWN, CLB_SET_DESTINATION,
        CLB_SET_DISPLAY_DESTINATION, CLB_CALL_CMD, CLB_COUNTRY_CHOOSER, CLB_LOAD_MAP, CLB_UNLOAD_MAP, CLB_DELETE_MAP
    }

    private static final MsgType[] msg_values = MsgType.values();

    private static class CallBackHandler extends Handler {
        public void handleMessage(Message msg) {
            switch (msg_values[msg.what]) {
                case CLB_ZOOM_IN:
                    callbackMessageChannel(1, "");
                    break;
                case CLB_ZOOM_OUT:
                    callbackMessageChannel(2, "");
                    break;
                case CLB_MOVE:
                    //motionCallback(mMotionCallbackID, msg.getData().getInt("x"), msg.getData().getInt("y"));
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
                    //buttonCallback(mButtonCallbackID, 0, 1, msg.getData().getInt("x"), msg.getData().getInt("y"));
                    break;
                case CLB_BUTTON_DOWN:
                    //buttonCallback(mButtonCallbackID, 1, 1, msg.getData().getInt("x"), msg.getData().getInt("y"));
                    break;
                case CLB_COUNTRY_CHOOSER:
                    break;
                case CLB_LOAD_MAP:
                    callbackMessageChannel(6, msg.getData().getString(("title")));
                    break;
                case CLB_DELETE_MAP:
                    //unload map before deleting it !!!
                    callbackMessageChannel(7, msg.getData().getString(("title")));
                    NavitUtils.removeFileIfExists(msg.getData().getString(("title")));
                    break;
                case CLB_UNLOAD_MAP:
                    callbackMessageChannel(7, msg.getData().getString(("title")));
                    break;
                case CLB_REDRAW:
                default:
                    Log.d(TAG, "Unhandled callback : " + msg_values[msg.what]);
            }
        }
    }


    private native void sizeChangedCallback(long id, int x, int y);

    private native void paddingChangedCallback(long id, int left, int top, int right, int bottom);

    private native void keypressCallback(long id, String s);

    private static native int callbackMessageChannel(int i, String s);

    private native void buttonCallback(long id, int pressed, int button, int x, int y);

    private native void motionCallback(long id, int x, int y);

    private native String getCoordForPoint(int x, int y, boolean absolutCoord);

    static native String[][] getAllCountries();

    private Canvas mDrawCanvas;
    private Bitmap mDrawBitmap;
    private long mSizeChangedCallbackID;
    private long mPaddingChangedCallbackID;
    private long mButtonCallbackID;
    private long mMotionCallbackID;
    private long mKeypressCallbackID;

    /**
     * Adjust views used to tint navigation and status bars.
     *
     * <p>This method is called from handleResize.
     *
     * It (re-)evaluates if and where the navigation bar is going to be shown, and calculates the
     * padding for objects which should not be obstructed.</p>
     *
     */
    private void adjustSystemBarsTintingViews() {

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
     * Handles resize events.
     *
     * <p>This method is called whenever the main View is resized in any way. This is the case when its
     * {@code onSizeChanged()} event handler fires.</p>
     */
    private void handleResize(int w, int h) {
        if (this.mParentGraphics == null) {
            Log.d(TAG, String.format("handleResize w=%d h=%d", w, h));
            if (mTinting) {
                if (Build.VERSION.SDK_INT == Build.VERSION_CODES.KITKAT) {
                    resizePaddingKitkat();
                }
                adjustSystemBarsTintingViews(); // is incl paddingchangedcallback
            }
        }
    }


    @RequiresApi(api = Build.VERSION_CODES.KITKAT)
    private void resizePaddingKitkat() {
        /*
         * API 19 does not support window insets.
         *
         * All system bar sizes are device defaults and do not change with rotation, but we have
         * to figure out which ones apply.
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
        mPaddingLeft = 0;
        if (!sInMap) {
            mPaddingTop = 0;
            mPaddingRight = 0;
            mPaddingBottom = 0;
            return;
        }
        Resources resources = NavitAppConfig.sResources;
        int shid = resources.getIdentifier("status_bar_height", "dimen", "android");
        int nhid = resources.getIdentifier("navigation_bar_height", "dimen", "android");
        int nhlid = resources.getIdentifier("navigation_bar_height_landscape", "dimen", "android");
        int nwid = resources.getIdentifier("navigation_bar_width", "dimen", "android");
        int statusBarHeight = (shid > 0) ? resources.getDimensionPixelSize(shid) : 0;
        int navigationBarHeight = (nhid > 0) ? resources.getDimensionPixelSize(nhid) : 0;
        int navigationBarHeightLandscape = (nhlid > 0) ? resources.getDimensionPixelSize(nhlid) : 0;
        int navigationBarWidth = (nwid > 0) ? resources.getDimensionPixelSize(nwid) : 0;
        Log.v(TAG, String.format("statusBarHeight=%d, navigationBarHeight=%d, "
                        + "navigationBarHeightLandscape=%d, navigationBarWidth=%d",
                        statusBarHeight, navigationBarHeight,
                        navigationBarHeightLandscape, navigationBarWidth));
        boolean isStatusShowing = !mActivity.mIsFullscreen;
        boolean isNavShowing = !ViewConfigurationCompat.hasPermanentMenuKey(ViewConfiguration.get(mActivity));
        Log.v(TAG, String.format("isStatusShowing=%b isNavShowing=%b", isStatusShowing, isNavShowing));
        boolean isLandscape = (mActivity.getResources().getConfiguration().orientation
                == Configuration.ORIENTATION_LANDSCAPE);
        boolean isNavAtBottom = (!isLandscape)
                || (mActivity.getResources().getConfiguration().smallestScreenWidthDp >= 600);
        Log.v(TAG, String.format("isNavAtBottom=%b (Config.smallestScreenWidthDp=%d, isLandscape=%b)",
                isNavAtBottom, mActivity.getResources().getConfiguration().smallestScreenWidthDp, isLandscape));

        mPaddingTop = isStatusShowing ? statusBarHeight : 0;
        mPaddingRight = (isNavShowing && !isNavAtBottom) ? navigationBarWidth : 0;
        mPaddingBottom = (!(isNavShowing && isNavAtBottom)) ? 0 : (
                isLandscape ? navigationBarHeightLandscape : navigationBarHeight);
    }


    /**
     * Returns whether the device has a hardware menu button.
     *
     * <p>Only Android versions starting with ICS (API version 14) support the API call to detect the presence of a
     * Menu button. On earlier Android versions, the following assumptions will be made: On API levels up to 10,
     * this method will always return {@code true}, as these Android versions relied on devices having a physical
     * Menu button. On API levels 11 through 13 (Honeycomb releases), this method will always return
     * {@code false}, as Honeycomb was a tablet-only release and did not require devices to have a Menu button.</p>
     *
     * <p>Note that this method is not aware of non-standard mechanisms on some customized builds of Android</p>
     */
    boolean hasMenuButton() {
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

    void setSizeChangedCallback(long id) {
        mSizeChangedCallbackID = id;
    }

    void setPaddingChangedCallback(long id) {
        mPaddingChangedCallbackID = id;
    }

    void setButtonCallback(long id) {
        Log.v(TAG,"set Buttononcallback");
        mButtonCallbackID = id;
    }

    void setMotionCallback(long id) {
        mMotionCallbackID = id;
        Log.v(TAG,"set Motioncallback");
    }

    void setKeypressCallback(long id) {
        Log.v(TAG,"set Keypresscallback");
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

    /* takes an image and draws it on the screen as a prerendered maptile.
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
    private static final int DRAW_MODE_BEGIN = 0;
    private static final int DRAW_MODE_END = 1;
    /* Used by the pedestrian plugin, draws without a mapbackground */
    private static final int DRAW_MODE_BEGIN_CLEAR = 2;

    protected void draw_mode(int mode) {
        if (mode == DRAW_MODE_END) {
            if (mParentGraphics == null) {
                mView.invalidate();
            } else {
                mParentGraphics.mView.invalidate(get_rect());
            }
        }
        if (mode == DRAW_MODE_BEGIN_CLEAR || (mode == DRAW_MODE_BEGIN && mParentGraphics != null)) {
            mDrawBitmap.eraseColor(0);
        }

    }

    protected void draw_drag(int x, int y) {
        mPosX = x;
        mPosY = y;
    }

    protected void overlay_disable(int disable) {
        Log.v(TAG,"overlay_disable: " + disable + ", Parent: " + (mParentGraphics != null));
        if (mParentGraphics == null) {
            sInMap = (disable == 0);
            workAroundForGuiInternal();
        }
        if (mOverlayDisabled != disable) {
            mOverlayDisabled = disable;
            if (mParentGraphics != null) {
                mParentGraphics.mView.invalidate(get_rect());
            }
        }
    }

    private void workAroundForGuiInternal() {
        if (!mTinting) {
            return;
        }
        Log.v(TAG,"workaround gui internal");
        if (!sInMap && !mView.isDrag() && Build.VERSION.SDK_INT == Build.VERSION_CODES.KITKAT) {
            mActivity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
            mActivity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION);
            return;
        }
        if (Build.VERSION.SDK_INT == Build.VERSION_CODES.KITKAT) {
            mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
            mActivity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_NAVIGATION);
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
}
