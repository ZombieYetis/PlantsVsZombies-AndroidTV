/*
 * Copyright (C) 2023-2026  PvZ TV Touch Team
 *
 * This file is part of PlantsVsZombies-AndroidTV.
 *
 * PlantsVsZombies-AndroidTV is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * PlantsVsZombies-AndroidTV is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * PlantsVsZombies-AndroidTV.  If not, see <https://www.gnu.org/licenses/>.
 */

package com.transmension.mobile;

import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.media.AudioManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.provider.Settings;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import android.widget.FrameLayout;

import java.io.File;
import java.util.Set;

public class NativeActivity extends Activity {
    public static final int MBB_CANCEL = 2;
    public static final int MBB_NO = 4;
    public static final int MBB_OK = 1;
    public static final int MBB_YES = 3;
    public static final int MBT_CANCEL = 1;
    public static final int MBT_NO = 4;
    public static final int MBT_OK = 0;
    public static final int MBT_OK_CANCEL = 2;
    public static final int MBT_YES = 3;
    public static final int MBT_YES_NO = 5;
    private static final String TAG = "NativeActivity";
    public static int ACTION_OPEN = 0;
    public static int ACTION_SEND = 1;
    private final Handler mHandler = new Handler();
    public long mNativeHandle;
    protected NativeView mNativeView;
    protected FrameLayout mLayout = null;
    protected FrameLayout container = null;
    private boolean mDestroyed;
    private View mStartupView;
    private long mStartupTime = System.nanoTime();
    private long mStartupMinDuration = 4000;
    private long mStartupMaxDuration = 15000;

    void setUI() {
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH)
            setTheme(android.R.style.Theme_DeviceDefault_NoActionBar_Fullscreen);
        //设置导航栏透明，UI会好看些
        Window window = getWindow();
        window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN)
            window.getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_LAYOUT_STABLE);


        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            if (getSharedPreferences("data", 0).getBoolean("immersive", true)) {
                window.getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY | View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
            }
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
            window.setNavigationBarContrastEnforced(false);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            window.setStatusBarColor(Color.TRANSPARENT);
            window.setNavigationBarColor(Color.TRANSPARENT);
        }
        if (Build.VERSION.SDK_INT >= 28) {
            window.getAttributes().layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        } else {
            window.setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            window.setDecorFitsSystemWindows(false);
            WindowInsetsController insetsController = getWindow().getInsetsController();
            if (insetsController != null) {
                insetsController.hide(WindowInsets.Type.statusBars() | WindowInsets.Type.displayCutout());
                insetsController.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            }
        }
    }

    @Override // android.app.Activity
    protected void onCreate(Bundle savedInstanceState) {


        setUI();

        mLayout = new FrameLayout(this);
        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(-1, -1);
        mNativeView = createNativeView();
        mNativeView.setActivity(this);
        mNativeView.requestFocus();
        mLayout.addView(mNativeView, lp);
        FrameLayout.LayoutParams layoutParams = new FrameLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        layoutParams.gravity = Gravity.CENTER;
        mLayout.setLayoutParams(layoutParams);
        // 添加容器ViewGroup并设置ContentView
        container = new FrameLayout(this);
        container.setBackgroundColor(Color.BLACK);
        container.addView(mLayout);
        setContentView(container);

        final String libPath = getApplicationInfo().nativeLibraryDir;
        final String sysLibPath = "/system/lib/" + getPackageName();
        final String vendorLibPath = "/vendor/lib/" + getPackageName();
        final String dataFilesPath = getInternalDataPath();
        final String[] libPaths = {libPath, sysLibPath, vendorLibPath, dataFilesPath};

        final String[] depends = new String[]{"fmodex"};
        final String module = "GameMain";
        final String libname = "main";
        final String funcname = "android_main";

        String path = null;
        int length = libPaths.length;
        int i = 0;
        while (true) {
            if (i >= length) {
                break;
            }
            String nativeLibraryDir = libPaths[i];
            File libraryFile = new File(nativeLibraryDir, libname);
            if (libraryFile.exists()) {
                path = libraryFile.getPath();
                break;
            }
            File libraryFile2 = new File(nativeLibraryDir, System.mapLibraryName(libname));
            if (libraryFile2.exists()) {
                path = libraryFile2.getPath();
                break;
            }
            File libraryFile3 = new File(nativeLibraryDir, module);
            if (libraryFile3.exists()) {
                path = libraryFile3.getPath();
                break;
            }
            File libraryFile4 = new File(nativeLibraryDir, System.mapLibraryName(module));
            if (!libraryFile4.exists()) {
                i++;
            } else {
                path = libraryFile4.getPath();
                break;
            }
        }
        if (path == null) {
            throw new IllegalArgumentException("Unable to find native library: " + libname + " or " + module);
        }
        NativeApp.load(libPaths, depends);

        mNativeHandle = NativeApp.loadNativeApp(path, funcname, this, mNativeView, getUserDataPath(), getExternalDataPath(), Build.VERSION.SDK_INT, getAssets());
        if (mNativeHandle == 0) {
            throw new IllegalArgumentException("Unable to load native library: " + path);
        }
        super.onCreate(savedInstanceState);
        mNativeView.requestFocus();


    }

    public String getUserDataPath() {
        SharedPreferences sharedPreferences = getSharedPreferences("data", 0);
        if (sharedPreferences.getBoolean("useExternalPath", Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)) {
            return getExternalDataPath();
        } else {
            return getInternalDataPath();
        }
    }

    @Override // android.app.Activity
    protected void onDestroy() {
        Log.i(TAG, "onDestroy()");
        mDestroyed = true;
        NativeApp.unloadNativeApp(mNativeHandle);
        super.onDestroy();
    }

    @Override // android.app.Activity
    protected void onPause() {
        Log.i(TAG, "onPause()");
        super.onPause();
        NativeApp.onPauseNative(mNativeHandle);

    }

    @Override // android.app.Activity
    protected void onResume() {
        Log.i(TAG, "onResume()");
        super.onResume();
        NativeApp.onResumeNative(mNativeHandle);
    }


    @Override // android.app.Activity
    protected void onStart() {
        Log.i(TAG, "onStart()");
        super.onStart();
        NativeApp.onStartNative(mNativeHandle);

    }

    @Override // android.app.Activity
    protected void onStop() {
        Log.i(TAG, "onStop()");
        super.onStop();
        NativeApp.onStopNative(mNativeHandle);

    }

    @Override // android.app.Activity, android.content.ComponentCallbacks
    public void onConfigurationChanged(Configuration newConfig) {
        Log.i(TAG, "onConfigurationchanged()");
        super.onConfigurationChanged(newConfig);
        if (!mDestroyed) {
            NativeApp.onConfigurationChangedNative(mNativeHandle);
        }
    }

    @Override // android.app.Activity, android.content.ComponentCallbacks
    public void onLowMemory() {
        Log.i(TAG, "onLowMemory()");
        super.onLowMemory();
        if (!mDestroyed) {
            NativeApp.onLowMemoryNative(mNativeHandle);
        }
    }

    public void createEventListeners() {
    }

//    public void addEventListener(ActivityEventListener listener) {
//
//    }
//
//    public void removeEventListener(ActivityEventListener listener) {
//
//    }

    protected void dispatchOnCreateEventToListener() {


    }

    protected void dispatchOnStartEventToListener() {

    }

    protected void dispatchOnPauseEventToListener() {

    }

    protected void dispatchOnResumeEventToListener() {

    }

    protected void dispatchOnStopEventToListener() {

    }

    protected void dispatchOnDestroyEventToListener() {

    }

    public boolean isAlive() {
        return !mDestroyed;
    }

    public FrameLayout getLayout() {
        return mLayout;
    }

    public void setWindowFlags(int flags, int mask) {
        getWindow().setFlags(flags, mask);
    }

    public void setWindowFormat(int format) {
        getWindow().setFormat(PixelFormat.RGBA_8888);
    }

    public void setStartupMinDuration(long duration) {
        mStartupMinDuration = duration;
    }

    public void setStartupMaxDuration(long duration) {
        mStartupMaxDuration = duration;
    }

    public void setStartupView(View view) {
        mStartupView = view;
    }

    public void showStartupView() {
        if (mStartupView != null && mStartupView.getParent() == null) {
            FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(-1, -1);
            mLayout.addView(mStartupView, lp);
            mStartupTime = System.nanoTime();
            mHandler.postDelayed(this::notifyStartupFinished, mStartupMaxDuration);
        }
    }

    public void hideStartupView() {
        mLayout.removeView(mStartupView);
    }

    public void notifyStartupFinished() {
        if (mStartupView != null) {
            long current = System.nanoTime();
            long elapsed = (current - mStartupTime) / 1000000;
            long delay = 0;
            if (elapsed < mStartupMinDuration) {
                delay = mStartupMinDuration - elapsed;
            }

            mHandler.postDelayed(() -> {
                hideStartupView();
                setStartupView(null);
            }, delay);
        }
    }

    public void installPackage(String path) {
    }

    public String getInternalDataPath() {
        File file = getFilesDir();
        return file.getAbsolutePath();
    }

    public String getExternalDataPath() {
        File file = getExternalFilesDir(null);
        return file == null ? "" : file.getAbsolutePath();
    }

    protected NativeView createNativeView() {
        return new NativeView(this);
    }

    public TextInputManager createTextInputManager(NativeView view) {
        return new AndroidTextInputManager(this, view);
    }

    public String getPackageSource() {
        ApplicationInfo appInfo = getApplicationInfo();
        return appInfo.sourceDir;
    }

    public String getVersionName() {
        try {
            PackageInfo info = getPackageManager().getPackageInfo(getPackageName(), 0);
            return info.versionName;
        } catch (PackageManager.NameNotFoundException e) {
            return "";
        }
    }

    public int getVersionCode() {
        try {
            PackageInfo info = getPackageManager().getPackageInfo(getPackageName(), 0);
            return info.versionCode;
        } catch (PackageManager.NameNotFoundException e) {
            return 0;
        }
    }

    public String getNativeLibraryDir() {
        ApplicationInfo appInfo = getApplicationInfo();
        return appInfo.nativeLibraryDir;
    }

    public int showMessageBox(String title, String message, int flags) {
        return 1048576;
    }

    public void closeMessageBox(int handle) {
    }

    public void closeAllMessageBoxes() {
    }

    public String getManufacturer() {
        return Build.MANUFACTURER;
    }

    public String getModel() {
        return Build.MODEL;
    }

    public String getProduct() {
        return Build.PRODUCT;
    }

    public String getAndroidId() {
        String androidId = Settings.Secure.getString(getContentResolver(), Settings.Secure.ANDROID_ID);
        return androidId != null ? androidId : "";
    }

    public int getDeviceId() {
        return 0;
    }

    public String getSerialNumber() {
        return "";
    }

    public String getMacAddress() {

        return "ABC";
    }

    public boolean isOnline() {
        return false;
    }

    public int getActiveNetworkType() {

        return 1;

    }

    public void processWorks() {
        NativeApp.processWorksNative(mNativeHandle);
    }

    public boolean isTablet() {
        boolean xlarge = (getResources().getConfiguration().screenLayout & 15) == 4;
        if (xlarge) {
            DisplayMetrics metrics = new DisplayMetrics();
            getWindowManager().getDefaultDisplay().getMetrics(metrics);
            return metrics.densityDpi == 240 || metrics.densityDpi == 160 || metrics.densityDpi == 213 || metrics.densityDpi == 320;
        }
        return false;
    }

    void openURL(String url, int flags) {

    }

    void openURL(String url) {
    }

    public boolean getHeadsetState() {
        AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        if (audioManager == null) {
            return false;
        }
        return audioManager.isWiredHeadsetOn() || audioManager.isBluetoothA2dpOn();
    }

    public void setAudioMode(int mode) {
        AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        if (audioManager != null) {
            audioManager.setMode(mode);
        }
    }

    public NativeView getNativeView() {
        return mNativeView;
    }

    public long getNativeHandle() {
        return mNativeHandle;
    }

    public String[] bundleToStringPairs(Bundle bundle) {
        if (bundle == null || bundle.size() == 0) {
            return null;
        }
        String[] result = new String[bundle.size() * 2];
        int i = 0;
        Set<String> keys = bundle.keySet();
        for (String key : keys) {
            Object value = bundle.get(key);
            String strValue = value.toString();
            int i2 = i + 1;
            result[i] = new String(key);
            i = i2 + 1;
            result[i2] = new String(strValue);
        }
        return result;
    }

    public String[] getIntentExtras() {
        Bundle extras = getIntent().getExtras();
        return bundleToStringPairs(extras);
    }

    public String getIntentURI() {
        Intent intent = getIntent();
        return intent.getDataString();
    }

    public Bundle getMetaDataBundle() {
        ActivityInfo ai = null;
        try {
            ai = getPackageManager().getActivityInfo(getComponentName(), 128);
        } catch (PackageManager.NameNotFoundException ignored) {
        }
        if (ai == null) {
            return null;
        }
        return ai.metaData;
    }

    public Bundle getAppMetaDataBundle() {
        ApplicationInfo appinfo = null;
        try {
            appinfo = getPackageManager().getApplicationInfo(getPackageName(), 128);
        } catch (PackageManager.NameNotFoundException ignored) {
        }
        if (appinfo == null) {
            return null;
        }
        return appinfo.metaData;
    }

    public String[] getAllMetaData() {
        Bundle metaData = getMetaDataBundle();
        return bundleToStringPairs(metaData);
    }

    public String getAppMetaData(String key) {
        Bundle metaData = getAppMetaDataBundle();
        if (metaData == null || !metaData.containsKey(key)) {
            return null;
        }
        return metaData.get(key).toString();
    }

    public String[] getAllAppMetaData() {
        Bundle metaData = getAppMetaDataBundle();
        return bundleToStringPairs(metaData);
    }

    public String getAppMetaData(String key, String defaultValue) {
        Bundle metaData = getAppMetaDataBundle();
        return (metaData == null || !metaData.containsKey(key)) ? defaultValue : metaData.get(key).toString();
    }

    public int getAppMetaData(String key, int defaultValue) {
        Bundle metaData = getAppMetaDataBundle();
        return (metaData == null || !metaData.containsKey(key)) ? defaultValue : metaData.getInt(key);
    }

    public boolean getAppMetaData(String key, boolean defaultValue) {
        Bundle metaData = getAppMetaDataBundle();
        return (metaData == null || !metaData.containsKey(key)) ? defaultValue : metaData.getBoolean(key);
    }

    public String getMetaData(String key) {
        Bundle metaData = getMetaDataBundle();
        if (metaData == null || !metaData.containsKey(key)) {
            return null;
        }
        return metaData.get(key).toString();
    }

    public String getMetaData(String key, String defaultValue) {
        Bundle metaData = getMetaDataBundle();
        return (metaData == null || !metaData.containsKey(key)) ? defaultValue : metaData.get(key).toString();
    }

    public int getMetaData(String key, int defaultValue) {
        Bundle metaData = getMetaDataBundle();
        return (metaData == null || !metaData.containsKey(key)) ? defaultValue : metaData.getInt(key);
    }

    public boolean getMetaData(String key, boolean defaultValue) {
        Bundle metaData = getMetaDataBundle();
        return (metaData == null || !metaData.containsKey(key)) ? defaultValue : metaData.getBoolean(key);
    }

    public void onNativeKeyEvent(KeyEvent event) {
        if (mNativeView != null) {
            InputManager.KeyInputEvent keyEvent = InputManager.KeyInputEvent.translate(event);
            NativeInputManager.onKeyInputEventNative(mNativeHandle, null, keyEvent);
        }
    }

    public void onNativeMotionEvent(MotionEvent event) {
        if (mNativeView != null) {
            if ((event.getSource() & 16) != 0) {
                NativeInputManager.onJoystickEventNative(mNativeHandle, null, InputManager.JoystickEvent.translate(event));
            } else if ((event.getSource() & 4) == 0) {
                NativeInputManager.onTouchEventNative(mNativeHandle, null, InputManager.PointerEvent.translate(event));
            }
        }
    }

    @Override // android.app.Activity
    public void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        NativeApp.onNewIntentNative(mNativeHandle);
    }

//    public void addActivityResultListener(ActivityResultListener listener) {
//
//    }
//
//    public void removeActivityResultListener(ActivityResultListener listener) {
//
//    }
//
//
//    public void addActivityNewIntentListener(ActivityNewIntentListener listener) {
//
//    }
//
//    public void removeActivityNewIntentListener(ActivityNewIntentListener listener) {
//
//    }

    public boolean startWebBrowser(String url, int options) {
        return false;
    }

    public boolean startWebBrowser(String url, int options, String orientation, String extras) {

        return false;

    }

    public void hideWebBrowser() {

    }

    void launchApp() {
    }

    public void restartApp() {
        finish();
    }

    public void setClipboard(String label, String text) {
    }

    public String getClipboard() {
        return "";

    }

    /* loaded from: classes.dex */
    class MessageBoxListener implements DialogInterface.OnClickListener {
        public int mId;
        public int mMessageBoxId;

        MessageBoxListener(int mbId, int id) {
            mMessageBoxId = mbId;
            mId = id;
        }

        @Override // android.content.DialogInterface.OnClickListener
        public void onClick(DialogInterface dialog, int which) {
            NativeApp.onMessageBoxButtonClickedNative(mNativeHandle, mMessageBoxId, mId);
            closeMessageBox(mMessageBoxId);
        }
    }
}
