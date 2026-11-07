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

import android.animation.LayoutTransition;
import android.animation.ObjectAnimator;
import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.ShapeDrawable;
import android.graphics.drawable.shapes.RoundRectShape;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.provider.MediaStore;
import android.provider.OpenableColumns;
import android.text.Editable;
import android.text.InputFilter;
import android.text.InputType;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.Pair;
import android.util.TypedValue;
import android.util.Xml;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.NumberPicker;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import com.trans.pvztv.BuildConfig;
import com.trans.pvztv.R;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Locale;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

public class SetActivity extends Activity {
    private static final int REQUEST_CODE_IMPORT_SAVEDATA = 0;
    private static final int REQUEST_CODE_EXPORT_SAVEDATA = 1;
    private static final int REQUEST_CODE_IMPORT_PAK = 2;
    private static final int REQUEST_CODE_IMPORT_LEVEL_CONFIG = 3;
    private static final int REQUEST_CODE_EXPORT_LOG = 4;

    private static final String EXTERNAL_PAK_DIR = "imported_paks";
    private static final String EXTERNAL_PAK_PREFIX = "external_pak:";
    private SharedPreferences dataSharedPreferences;
    private LinearLayout pakListLayout;
    private LinearLayout.LayoutParams pakItemLayoutParams;

    public static File getUserDataFile(Context context) {
        SharedPreferences sharedPreferences = context.getSharedPreferences("data", 0);
        if (sharedPreferences.getBoolean("useExternalPath", Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)) {
            return context.getExternalFilesDir(null);
        } else {
            return context.getFilesDir();
        }
    }

    public static void copyDir(File srcDir, File destDir) throws IOException {
        if (srcDir.isDirectory()) {
            if (!destDir.exists()) {
                destDir.mkdirs();
            }

            String[] children = srcDir.list();
            if (children == null) return;
            for (String child : children) {
                File srcFile = new File(srcDir, child);
                File destFile = new File(destDir, child);
                copyDir(srcFile, destFile);
            }
        } else {
            FileInputStream inputStream = new FileInputStream(srcDir);
            FileOutputStream outputStream = new FileOutputStream(destDir);

            byte[] buffer = new byte[1024];
            int bytesRead;
            while ((bytesRead = inputStream.read(buffer)) > 0) {
                outputStream.write(buffer, 0, bytesRead);
            }

            inputStream.close();
            outputStream.close();
        }
    }

    public static void deleteRecursive(File file) {
        if (file.isFile()) {
            file.delete();
        } else {
            // 如果是文件夹，递归删除
            File[] files = file.listFiles();
            if (files != null && files.length > 0) {
                for (File subFile : files) {
                    deleteRecursive(subFile);
                }
            }
            file.delete();
        }
    }

    public static List<Pair<String, Long>> findNumDirs(File dir) {
        List<Pair<String, Long>> list = new ArrayList<>();
        if (dir.exists() && dir.isDirectory()) {
            File[] files = dir.listFiles((dir1, name) -> {
                // 判断只要是数字开头的文件夹就符合要求
                return new File(dir1, name).isDirectory() && Character.isDigit(name.charAt(0));
            });
            if (files != null && files.length > 0) {
                // 将文件夹的名称和修改时间存入Pair中，再将Pair存入list中
                for (File file : files) {
                    String dirName = file.getName();
                    long modifiedTime = file.lastModified();
                    Pair<String, Long> pair = new Pair<>(dirName, modifiedTime);
                    list.add(pair);
                }
                // 按照修改时间排序
                Collections.sort(list, (o1, o2) -> o2.second.compareTo(o1.second));
            }
        }
        return list;
    }

    public static void addFolderToZip(File folder, ZipOutputStream zipOutputStream) throws IOException {
        File[] files = folder.listFiles();
        if (files == null) return;
        for (File file : files) {
            if (file.isDirectory()) {
                addFolderToZip(file, zipOutputStream);
            } else {
                byte[] buffer = new byte[1024];
                FileInputStream fileInputStream = new FileInputStream(file);
                zipOutputStream.putNextEntry(new ZipEntry(file.getName()));
                int length;
                while ((length = fileInputStream.read(buffer)) > 0) {
                    zipOutputStream.write(buffer, 0, length);
                }
                fileInputStream.close();
            }
        }
    }

    public void loadPreferencesFromAssetsFile(SharedPreferences preferences, SharedPreferences sharedPreferences) {
        //如果是初次启动，则载入assets文件夹中的data.xml
        if (preferences.getBoolean("firstLaunch", true)) {
            try {
                InputStream inputStream = getAssets().open("defaultSetting.xml");
                XmlPullParser parser = Xml.newPullParser();
                parser.setInput(inputStream, "utf-8");

                int eventType = parser.getEventType();
                SharedPreferences.Editor editor = sharedPreferences.edit();
                while (eventType != XmlPullParser.END_DOCUMENT) {
                    if (eventType == XmlPullParser.START_TAG) {
                        switch (parser.getName()) {
                            case "boolean": {
                                String name = parser.getAttributeValue(null, "name");
                                boolean value = Boolean.parseBoolean(parser.getAttributeValue(null, "value"));
                                editor.putBoolean(name, value);
                                break;
                            }
                            case "int": {
                                String name = parser.getAttributeValue(null, "name");
                                int value = Integer.parseInt(parser.getAttributeValue(null, "value"));
                                editor.putInt(name, value);
                                break;
                            }
                        }
                    }
                    eventType = parser.next();
                }
                editor.apply();
                inputStream.close();
            } catch (IOException | XmlPullParserException e) {
                e.printStackTrace();
            }
            preferences.edit().putBoolean("firstLaunch", false).apply();
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);


        //设置主题、导航栏与操作栏透明，UI会好看些

        boolean isNight = (getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_YES) == Configuration.UI_MODE_NIGHT_YES;

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH)
            setTheme(isNight ? android.R.style.Theme_DeviceDefault : android.R.style.Theme_DeviceDefault_Light);
        Window window = getWindow();
        if (!isNight) {
            window.getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR | View.SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR);
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
            window.setNavigationBarContrastEnforced(false);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            window.setStatusBarColor(Color.TRANSPARENT);
            window.setNavigationBarColor(Color.TRANSPARENT);
        }
        ActionBar actionBar = getActionBar();
        if (actionBar != null) {
            actionBar.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        }


        //如果是初次启动，则载入assets文件夹中的data.xml
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        SharedPreferences sharedPreferences = getSharedPreferences("data", 0);
        dataSharedPreferences = sharedPreferences;

        loadPreferencesFromAssetsFile(preferences, sharedPreferences);


        setTitle(getString(R.string.addon_title));
        ScrollView scrollView = new ScrollView(this);
        LinearLayout linearLayout = new LinearLayout(this);
        linearLayout.setOrientation(LinearLayout.VERTICAL);
        LayoutTransition transition = new LayoutTransition();
        transition.setDuration(200L);
        ObjectAnimator animator = ObjectAnimator.ofFloat(null, "scaleX", 0.0f, 1.0f);
        transition.setAnimator(2, animator);
        linearLayout.setLayoutTransition(transition);


        LinearLayout.LayoutParams wrapWrapParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        wrapWrapParams.gravity = Gravity.CENTER;
        LinearLayout.LayoutParams matchWrapParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        matchWrapParams.gravity = Gravity.CENTER;
        pakItemLayoutParams = matchWrapParams;
        LinearLayout.LayoutParams weightParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        weightParams.weight = 1;
        LinearLayout.LayoutParams marginParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        marginParams.setMargins(0, 20, 0, 20);

        float density = getResources().getDisplayMetrics().density;
        float radius = 40 * density;
        ShapeDrawable oval = new ShapeDrawable(new RoundRectShape(new float[]{radius, radius, radius, radius, radius, radius, radius, radius}, null, null));
        oval.getPaint().setColor(isNight ? Color.DKGRAY : Color.LTGRAY);


        String aspectName = getString(R.string.addon_aspect_name);
        Button aspectExpand = new Button(this);
        LinearLayout aspectLayout = new LinearLayout(this);

        aspectExpand.setText(String.format("▽ %s ▽", aspectName));
        aspectExpand.setTypeface(Typeface.DEFAULT_BOLD);
        aspectExpand.setTextSize(15f);
        aspectExpand.setBackgroundDrawable(oval);
        aspectExpand.setLayoutParams(marginParams);
        aspectExpand.setOnClickListener(view -> {
            boolean isVisible = aspectLayout.getVisibility() == View.VISIBLE;
            ((Button) view).setText(isVisible ? String.format("▽ %s ▽", aspectName) : String.format("△ %s △", aspectName));
            aspectLayout.setVisibility(isVisible ? View.GONE : View.VISIBLE);
        });
        TextView aspectInfo = new TextView(this);
        aspectInfo.setGravity(Gravity.CENTER);
        aspectInfo.setText(R.string.addon_aspect_info);
        aspectInfo.setTextSize(15f);

        CheckBox fullscreenCheckBox = new CheckBox(this);
        fullscreenCheckBox.setText(R.string.addon_aspect_fullscreen);
        fullscreenCheckBox.setTextSize(16f);
        fullscreenCheckBox.setPadding(0, 30, 0, 30);
        fullscreenCheckBox.setChecked(!sharedPreferences.getBoolean("shiLiuBiJiu", true));
        fullscreenCheckBox.setLayoutParams(wrapWrapParams);

        String fullscreenText1 = getString(R.string.addon_aspect_fullscreen_text1);
        String fullscreenText2 = getString(R.string.addon_aspect_fullscreen_text2);
        TextView fullscreenInfo = new TextView(this);
        fullscreenInfo.setGravity(Gravity.CENTER);
        fullscreenInfo.setText(fullscreenCheckBox.isChecked() ? fullscreenText1 : fullscreenText2);
        fullscreenInfo.setTextSize(15f);


        LinearLayout aspectChooser = new LinearLayout(this);
        aspectChooser.setOrientation(LinearLayout.HORIZONTAL);

        NumberPicker widthPicker = new NumberPicker(this);
        widthPicker.setMaxValue(20);
        widthPicker.setMinValue(1);
        widthPicker.setLayoutParams(weightParams);
        widthPicker.setValue(sharedPreferences.getInt("width", 16));
        NumberPicker heightPicker = new NumberPicker(this);
        heightPicker.setMaxValue(20);
        heightPicker.setMinValue(1);
        heightPicker.setLayoutParams(weightParams);
        heightPicker.setValue(sharedPreferences.getInt("height", 9));
        aspectChooser.addView(widthPicker);
        aspectChooser.addView(heightPicker);
        aspectChooser.setVisibility(fullscreenCheckBox.isChecked() ? View.GONE : View.VISIBLE);
        fullscreenCheckBox.setOnCheckedChangeListener((compoundButton, b) -> {
            aspectChooser.setVisibility(b ? View.GONE : View.VISIBLE);
            fullscreenInfo.setText(b ? fullscreenText1 : fullscreenText2);
        });

        LinearLayout container2 = new LinearLayout(this);
        container2.setOrientation(LinearLayout.HORIZONTAL);
        TextView textView2 = new TextView(this);
        textView2.setGravity(Gravity.CENTER);
        textView2.setText(R.string.addon_aspect_scale);
        textView2.setTextSize(16f);
        textView2.setTextColor(isNight ? Color.WHITE : Color.BLACK);
        textView2.setLayoutParams(weightParams);
        SeekBar scaleSeekBar = new SeekBar(this);
        scaleSeekBar.setMax(20);
        scaleSeekBar.setProgress(sharedPreferences.getInt("scaleX", 0));
        scaleSeekBar.setLayoutParams(matchWrapParams);
        InputFilter[] filters2 = new InputFilter[1];
        filters2[0] = new InputFilter.LengthFilter(2);
        EditText editText2 = new EditText(this);
        editText2.setText(String.valueOf(sharedPreferences.getInt("scaleX", 0)));
        scaleSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
                editText2.setText(String.valueOf(i));
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });
        editText2.setHint("(0-20)");
        editText2.setGravity(Gravity.CENTER);
        editText2.setInputType(InputType.TYPE_CLASS_NUMBER);
        editText2.setFilters(filters2);
        editText2.setSelectAllOnFocus(true);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN)
            editText2.setBackground(null);
        editText2.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {
            }

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {
                if (charSequence == null || charSequence.length() == 0) return;
                int num = Integer.parseInt(String.valueOf(charSequence));
                scaleSeekBar.setProgress(Math.min(num, 20));
            }

            @Override
            public void afterTextChanged(Editable editable) {
            }
        });
        container2.addView(textView2);
        container2.addView(editText2);

        TextView scaleInfo = new TextView(this);
        scaleInfo.setGravity(Gravity.CENTER);
        scaleInfo.setText(R.string.addon_aspect_scale_info);
        scaleInfo.setTextSize(15f);
        scaleInfo.setPadding(0, 20, 0, 20);

        Button saveAspect = new Button(this);
        saveAspect.setText(R.string.addon_aspect_save);
        saveAspect.setOnClickListener(view -> {
            sharedPreferences.edit().putInt("width", widthPicker.getValue()).putInt("height", heightPicker.getValue()).putInt("scaleX", scaleSeekBar.getProgress()).putInt("scaleY", scaleSeekBar.getProgress()).putBoolean("shiLiuBiJiu", !fullscreenCheckBox.isChecked()).apply();
            Toast.makeText(
                    SetActivity.this,
                    fullscreenCheckBox.isChecked() ? getString(R.string.addon_aspect_toast1) : getString(R.string.addon_aspect_toast2, widthPicker.getValue(), heightPicker.getValue(), scaleSeekBar.getProgress()),
                    Toast.LENGTH_SHORT
            ).show();
        });

        CheckBox immersiveCheckBox = new CheckBox(this);
        immersiveCheckBox.setText(R.string.addon_aspect_immersive);
        immersiveCheckBox.setTextSize(16f);
        immersiveCheckBox.setPadding(0, 30, 0, 30);
        immersiveCheckBox.setChecked(sharedPreferences.getBoolean("immersive", true));
        immersiveCheckBox.setLayoutParams(wrapWrapParams);
        immersiveCheckBox.setOnCheckedChangeListener((buttonView, isChecked) -> sharedPreferences.edit().putBoolean("immersive", isChecked).apply());

        TextView immersiveInfo = new TextView(this);
        immersiveInfo.setGravity(Gravity.CENTER);
        immersiveInfo.setText(R.string.addon_aspect_immersive_text);
        immersiveInfo.setTextSize(15f);
        immersiveInfo.setPadding(0, 20, 0, 20);

        aspectLayout.setVisibility(View.GONE);
        aspectLayout.setOrientation(LinearLayout.VERTICAL);
        aspectLayout.setLayoutTransition(transition);
        aspectLayout.addView(aspectInfo);
        aspectLayout.addView(fullscreenCheckBox);
        aspectLayout.addView(fullscreenInfo);
        aspectLayout.addView(aspectChooser);
        aspectLayout.addView(container2);
        aspectLayout.addView(scaleSeekBar);
        aspectLayout.addView(scaleInfo);
        aspectLayout.addView(saveAspect);
        aspectLayout.addView(immersiveCheckBox);
        aspectLayout.addView(immersiveInfo);


        String keyboardName = getString(R.string.addon_keyboard_name);
        Button keyboardExpand = new Button(this);
        LinearLayout keyboardLayout = new LinearLayout(this);

        keyboardExpand.setText(String.format("▽ %s ▽", keyboardName));
        keyboardExpand.setTypeface(Typeface.DEFAULT_BOLD);
        keyboardExpand.setTextSize(15f);
        keyboardExpand.setBackgroundDrawable(oval);
        keyboardExpand.setLayoutParams(marginParams);
        keyboardExpand.setOnClickListener(view -> {
            boolean isVisible = keyboardLayout.getVisibility() == View.VISIBLE;
            ((Button) view).setText(isVisible ? String.format("▽ %s ▽", keyboardName) : String.format("△ %s △", keyboardName));
            keyboardLayout.setVisibility(isVisible ? View.GONE : View.VISIBLE);
        });

        TextView keyboardInfo = new TextView(this);
        keyboardInfo.setGravity(Gravity.CENTER);
        keyboardInfo.setText(R.string.addon_keyboard_info);
        keyboardInfo.setTextSize(15f);


        CheckBox useKeyboard = new CheckBox(this);
        useKeyboard.setChecked(sharedPreferences.getBoolean("useInGameKeyboard", true));
        useKeyboard.setText(R.string.addon_keyboard_usekeyboard);
        useKeyboard.setTextSize(16f);
        useKeyboard.setOnCheckedChangeListener((compoundButton, b) -> sharedPreferences.edit().putBoolean("useInGameKeyboard", b).apply());
        useKeyboard.setPadding(0, 30, 0, 30);
        useKeyboard.setLayoutParams(wrapWrapParams);
        TextView useKeyboardInfo = new TextView(this);
        useKeyboardInfo.setGravity(Gravity.CENTER);
        useKeyboardInfo.setText(R.string.addon_keyboard_usekeyboard_info);
        useKeyboardInfo.setTextSize(15f);


        CheckBox useSpecialPause = new CheckBox(this);
        useSpecialPause.setText(R.string.addon_keyboard_specialpause);
        useSpecialPause.setTextSize(16f);
        useSpecialPause.setPadding(0, 30, 0, 30);
        useSpecialPause.setChecked(sharedPreferences.getBoolean("useSpecialPause", false));
        useSpecialPause.setOnCheckedChangeListener((compoundButton, b) -> sharedPreferences.edit().putBoolean("useSpecialPause", b).apply());
        useSpecialPause.setLayoutParams(wrapWrapParams);
        TextView specialPauseInfo = new TextView(this);
        specialPauseInfo.setGravity(Gravity.CENTER);
        specialPauseInfo.setText(R.string.addon_keyboard_specialpause_info);
        specialPauseInfo.setTextSize(15f);


        CheckBox lockButtonPosition = new CheckBox(this);
        lockButtonPosition.setChecked(sharedPreferences.getBoolean("isVisibilityLockPosition", false));
        lockButtonPosition.setText(R.string.addon_keyboard_lock);
        lockButtonPosition.setTextSize(16f);
        lockButtonPosition.setOnCheckedChangeListener((compoundButton, b) -> sharedPreferences.edit().putBoolean("isVisibilityLockPosition", b).apply());
        lockButtonPosition.setPadding(0, 30, 0, 30);
        lockButtonPosition.setLayoutParams(wrapWrapParams);
        TextView lockInfo = new TextView(this);
        lockInfo.setGravity(Gravity.CENTER);
        lockInfo.setText(R.string.addon_keyboard_lock_info);
        lockInfo.setTextSize(15f);


        LinearLayout container1 = new LinearLayout(this);
        container1.setOrientation(LinearLayout.HORIZONTAL);
        TextView textView = new TextView(this);
        textView.setGravity(Gravity.CENTER);
        textView.setText(R.string.addon_keyboard_longpress);
        textView.setTextSize(16f);
        textView.setTextColor(isNight ? Color.WHITE : Color.BLACK);
        textView.setLayoutParams(weightParams);
        SeekBar seekBar = new SeekBar(this);
        seekBar.setMax(20);
        seekBar.setProgress(sharedPreferences.getInt("longPress", 8));
        seekBar.setLayoutParams(matchWrapParams);
        InputFilter[] filters1 = new InputFilter[1];
        filters1[0] = new InputFilter.LengthFilter(2);
        EditText editText = new EditText(this);
        editText.setText(String.valueOf(sharedPreferences.getInt("longPress", 8)));
        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
                if (i == 0) {
                    seekBar.setProgress(1);
                    return;
                }
                editText.setText(String.valueOf(i));
                sharedPreferences.edit().putInt("longPress", i).apply();
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });
        editText.setHint("(1-20)");
        editText.setGravity(Gravity.CENTER);
        editText.setInputType(InputType.TYPE_CLASS_NUMBER);
        editText.setFilters(filters1);
        editText.setSelectAllOnFocus(true);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN)
            editText.setBackground(null);
        editText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {
            }

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {
                if (charSequence == null || charSequence.length() == 0) return;
                int num = Integer.parseInt(String.valueOf(charSequence));
                seekBar.setProgress(Math.min(num, 20));
            }

            @Override
            public void afterTextChanged(Editable editable) {
            }
        });
        container1.addView(textView);
        container1.addView(editText);

        TextView longInfo = new TextView(this);
        longInfo.setGravity(Gravity.CENTER);
        longInfo.setText(R.string.addon_keyboard_longpress_info);
        longInfo.setTextSize(15f);

        Button keyboardSet = new Button(this);
        keyboardSet.setText(R.string.addon_keyboard_set);
        keyboardSet.setOnClickListener(view -> {
            startActivity(new Intent(SetActivity.this, ButtonSetActivity.class));
            finish();
        });

        TextView keyboardSetInfo = new TextView(this);
        keyboardSetInfo.setGravity(Gravity.CENTER);
        keyboardSetInfo.setText(R.string.addon_keyboard_set_info);
        keyboardSetInfo.setTextSize(15f);

        keyboardLayout.setVisibility(View.GONE);
        keyboardLayout.setOrientation(LinearLayout.VERTICAL);
        keyboardLayout.addView(keyboardInfo);
        keyboardLayout.addView(useKeyboard);
        keyboardLayout.addView(useKeyboardInfo);
        keyboardLayout.addView(useSpecialPause);
        keyboardLayout.addView(specialPauseInfo);
        keyboardLayout.addView(lockButtonPosition);
        keyboardLayout.addView(lockInfo);
        keyboardLayout.addView(container1);
        keyboardLayout.addView(seekBar);
        keyboardLayout.addView(longInfo);
        keyboardLayout.addView(keyboardSet);
        keyboardLayout.addView(keyboardSetInfo);

        String userdataName = getString(R.string.addon_userdata_name);
        Button userdataExpand = new Button(this);
        LinearLayout userdataLayout = new LinearLayout(this);

        userdataExpand.setText(String.format("▽ %s ▽", userdataName));
        userdataExpand.setTypeface(Typeface.DEFAULT_BOLD);
        userdataExpand.setTextSize(15f);
        userdataExpand.setBackgroundDrawable(oval);
        userdataExpand.setLayoutParams(marginParams);
        userdataExpand.setOnClickListener(view -> {
            boolean isVisible = userdataLayout.getVisibility() == View.VISIBLE;
            ((Button) view).setText(isVisible ? String.format("▽ %s ▽", userdataName) : String.format("△ %s △", userdataName));
            userdataLayout.setVisibility(isVisible ? View.GONE : View.VISIBLE);
        });

        TextView userdataInfo = new TextView(this);
        userdataInfo.setGravity(Gravity.CENTER);
        userdataInfo.setText(R.string.addon_userdata_info);
        userdataInfo.setTextSize(15f);

        final Button buttonImportFromExternal = new Button(this);
        buttonImportFromExternal.setText(R.string.addon_userdata_importfromexternal);
        buttonImportFromExternal.setOnClickListener(view -> Toast.makeText(this, R.string.addon_userdata_importfromexternal_toast1, Toast.LENGTH_SHORT).show());
        buttonImportFromExternal.setOnLongClickListener(view -> {
            try {
                InputStream inputStream = getAssets().open("userdata.zip");
                if (inputStream != null) {
                    File userdata_import = new File(getUserDataFile(this), "userdata");

                    if (!userdata_import.exists()) userdata_import.mkdir();
                    ZipInputStream zipInputStream = new ZipInputStream(inputStream);
                    // 读取ZipEntry对象
                    ZipEntry zipEntry = zipInputStream.getNextEntry();
                    // 遍历ZipEntry对象并解压缩文件
                    byte[] buffer = new byte[1024];
                    while (zipEntry != null) {
                        // 提取zip文件中的唯一一个目录结构下的所有文件
                        if (!zipEntry.isDirectory()) {
                            // 获取文件名并创建新文件。
                            String fileName = new File(zipEntry.getName()).getName();
                            File newFile = new File(userdata_import, fileName);
                            // 写出文件流
                            FileOutputStream fileOutputStream = new FileOutputStream(newFile);
                            int readLength;
                            while ((readLength = zipInputStream.read(buffer)) > 0) {
                                fileOutputStream.write(buffer, 0, readLength);
                            }
                            fileOutputStream.close();
                        }
                        // 关闭当前ZipEntry，并移到下一个ZipEntry。
                        zipInputStream.closeEntry();
                        zipEntry = zipInputStream.getNextEntry();
                    }
                    // 关闭ZipInputStream对象
                    zipInputStream.close();
                    inputStream.close();
                }
                Toast.makeText(SetActivity.this, R.string.addon_userdata_importfromexternal_toast2, Toast.LENGTH_SHORT).show();
            } catch (IOException e) {
                Toast.makeText(SetActivity.this, R.string.addon_userdata_importfromexternal_toast3, Toast.LENGTH_SHORT).show();
                e.printStackTrace();
            }
            return true;
        });

        TextView externalInfo = new TextView(this);
        externalInfo.setGravity(Gravity.CENTER);
        externalInfo.setText(R.string.addon_userdata_importfromexternal_info);
        externalInfo.setTextSize(15f);
        externalInfo.setPadding(0, 0, 0, 30);

        final Button buttonImport = new Button(this);
        buttonImport.setText(R.string.addon_userdata_import);

        buttonImport.setOnClickListener(view -> {
            Toast.makeText(SetActivity.this, R.string.addon_userdata_import_toast1, Toast.LENGTH_SHORT).show();
            //通过系统的文件浏览器选择一个文件
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            //筛选，只显示可以“打开”的结果，如文件(而不是联系人或时区列表)
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("application/zip");
            startActivityForResult(intent, REQUEST_CODE_IMPORT_SAVEDATA);
        });
        final Button buttonImportFromBackup = new Button(this);
        buttonImportFromBackup.setText(R.string.addon_userdata_importfrombackup);
        buttonImportFromBackup.setOnClickListener(view -> {
            PopFragment popFragment = new PopFragment();
            popFragment.show(getFragmentManager(), "pop");
        });

        final Button buttonExport = new Button(this);
        buttonExport.setText(R.string.addon_userdata_export);
        buttonExport.setOnClickListener(view -> {
            Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            // 文件类型
            intent.setType("application/zip");
            // 文件名称
            intent.putExtra(Intent.EXTRA_TITLE, getString(R.string.addon_userdata_export_filename));
            startActivityForResult(intent, REQUEST_CODE_EXPORT_SAVEDATA);
        });
        final CheckBox useExternalPath = new CheckBox(this);
        useExternalPath.setText(R.string.addon_userdata_useExternalPath);
        useExternalPath.setChecked(sharedPreferences.getBoolean("useExternalPath", Build.VERSION.SDK_INT >= Build.VERSION_CODES.M));
        useExternalPath.setOnCheckedChangeListener((compoundButton, bool) -> sharedPreferences.edit().putBoolean("useExternalPath", bool).apply());
        useExternalPath.setLayoutParams(wrapWrapParams);
        useExternalPath.setPadding(0, 30, 0, 30);

        TextView useExternalPathInfo = new TextView(this);
        useExternalPathInfo.setGravity(Gravity.CENTER);
        useExternalPathInfo.setText(R.string.addon_userdata_useExternalPath_info);
        useExternalPathInfo.setTextSize(15f);

        final CheckBox autoBackup = new CheckBox(this);
        autoBackup.setText(R.string.addon_userdata_autobackup);
        autoBackup.setChecked(sharedPreferences.getBoolean("autoBackUp", true));
        autoBackup.setOnCheckedChangeListener((compoundButton, bool) -> sharedPreferences.edit().putBoolean("autoBackUp", bool).apply());
        autoBackup.setLayoutParams(wrapWrapParams);
        autoBackup.setPadding(0, 30, 0, 30);

        TextView autoBackupInfo = new TextView(this);
        autoBackupInfo.setGravity(Gravity.CENTER);
        autoBackupInfo.setText(R.string.addon_userdata_autobackup_info);
        autoBackupInfo.setTextSize(15f);
        userdataLayout.setVisibility(View.GONE);
        userdataLayout.setOrientation(LinearLayout.VERTICAL);

        userdataLayout.addView(userdataInfo);
        userdataLayout.addView(buttonImportFromExternal);
        userdataLayout.addView(externalInfo);
        userdataLayout.addView(buttonImport);
        userdataLayout.addView(buttonImportFromBackup);
        userdataLayout.addView(buttonExport);
        userdataLayout.addView(useExternalPath);
        userdataLayout.addView(useExternalPathInfo);
        userdataLayout.addView(autoBackup);
        userdataLayout.addView(autoBackupInfo);


        String secondPlayerName = getString(R.string.addon_secondplayer_name);
        Button secondPlayerExpand = new Button(this);
        LinearLayout secondPlayerLayout = new LinearLayout(this);

        secondPlayerExpand.setText(String.format("▽ %s ▽", secondPlayerName));
        secondPlayerExpand.setTypeface(Typeface.DEFAULT_BOLD);
        secondPlayerExpand.setTextSize(15f);
        secondPlayerExpand.setBackgroundDrawable(oval);
        secondPlayerExpand.setLayoutParams(marginParams);
        secondPlayerExpand.setOnClickListener(view -> {
            boolean isVisible = secondPlayerLayout.getVisibility() == View.VISIBLE;
            ((Button) view).setText(isVisible ? String.format("▽ %s ▽", secondPlayerName) : String.format("△ %s △", secondPlayerName));
            secondPlayerLayout.setVisibility(isVisible ? View.GONE : View.VISIBLE);
        });

        TextView secondPlayerInfo = new TextView(this);
        secondPlayerInfo.setGravity(Gravity.CENTER);
        secondPlayerInfo.setText(R.string.addon_secondplayer_info);
        secondPlayerInfo.setTextSize(15f);


        final Button secondPlayerSet = new Button(this);
        secondPlayerSet.setText(R.string.addon_secondplayer_set);
        secondPlayerSet.setOnClickListener(view -> {
            startActivity(new Intent(SetActivity.this, KeyboardSetActivity.class));
            finish();
        });

        TextView secondPlayerSetInfo = new TextView(this);
        secondPlayerSetInfo.setGravity(Gravity.CENTER);
        secondPlayerSetInfo.setText(R.string.addon_secondplayer_set_info);
        secondPlayerSetInfo.setTextSize(15f);


        secondPlayerLayout.setVisibility(View.GONE);
        secondPlayerLayout.setOrientation(LinearLayout.VERTICAL);
        secondPlayerLayout.addView(secondPlayerInfo);
        secondPlayerLayout.addView(secondPlayerSet);
        secondPlayerLayout.addView(secondPlayerSetInfo);

        String inGameName = getString(R.string.addon_ingame_name);
        Button inGameExpand = new Button(this);
        LinearLayout inGameLayout = new LinearLayout(this);

        inGameExpand.setText(String.format("▽ %s ▽", inGameName));
        inGameExpand.setTypeface(Typeface.DEFAULT_BOLD);
        inGameExpand.setTextSize(15f);
        inGameExpand.setBackgroundDrawable(oval);
        inGameExpand.setLayoutParams(marginParams);
        inGameExpand.setOnClickListener(view -> {
            boolean isVisible = inGameLayout.getVisibility() == View.VISIBLE;
            ((Button) view).setText(isVisible ? String.format("▽ %s ▽", inGameName) : String.format("△ %s △", inGameName));
            inGameLayout.setVisibility(isVisible ? View.GONE : View.VISIBLE);
        });

        TextView inGameInfo = new TextView(this);
        inGameInfo.setGravity(Gravity.CENTER);
        inGameInfo.setText(R.string.addon_ingame_info);
        inGameInfo.setTextSize(15f);

        LinearLayout container = new LinearLayout(this);
        container.setOrientation(LinearLayout.VERTICAL);
        container.setLayoutParams(wrapWrapParams);
        final CheckBox manualCollect = new CheckBox(this);
        manualCollect.setText(R.string.addon_ingame_manualcollect);
        manualCollect.setChecked(sharedPreferences.getBoolean("enableManualCollect", false));
        manualCollect.setOnCheckedChangeListener((compoundButton, bool) -> sharedPreferences.edit().putBoolean("enableManualCollect", bool).apply());
        manualCollect.setLayoutParams(matchWrapParams);
        manualCollect.setOnLongClickListener(v -> {
            new AlertDialog.Builder(SetActivity.this)
                    .setTitle(R.string.addon_ingame_manualcollect)
                    .setMessage(R.string.addon_ingame_manualcollect_info) // 设置对话框内容
                    .setPositiveButton("OK", null)
                    .create().show();
            return true;
        });

        final CheckBox canShop = new CheckBox(this);
        canShop.setText(R.string.addon_ingame_canshop);
        canShop.setChecked(sharedPreferences.getBoolean("disableShop", false));
        canShop.setOnCheckedChangeListener((compoundButton, bool) -> sharedPreferences.edit().putBoolean("disableShop", bool).apply());
        canShop.setLayoutParams(matchWrapParams);
        canShop.setOnLongClickListener(v -> {
            new AlertDialog.Builder(SetActivity.this)
                    .setTitle(R.string.addon_ingame_canshop)
                    .setMessage(R.string.addon_ingame_canshop_info)
                    .setPositiveButton("OK", null)
                    .create().show();
            return true;
        });

        final CheckBox showCoolDown = new CheckBox(this);
        showCoolDown.setText(R.string.addon_ingame_showcooldown);
        showCoolDown.setChecked(sharedPreferences.getBoolean("showCoolDown", false));
        showCoolDown.setOnCheckedChangeListener((compoundButton, bool) -> sharedPreferences.edit().putBoolean("showCoolDown", bool).apply());
        showCoolDown.setLayoutParams(matchWrapParams);
        showCoolDown.setOnLongClickListener(v -> {
            new AlertDialog.Builder(SetActivity.this)
                    .setTitle(R.string.addon_ingame_showcooldown)
                    .setMessage(R.string.addon_ingame_showcooldown_info)
                    .setPositiveButton("OK", null)
                    .create().show();
            return true;
        });

        final CheckBox disableTrashBin = new CheckBox(this);
        disableTrashBin.setText(R.string.addon_ingame_disabletrashbin);
        disableTrashBin.setChecked(sharedPreferences.getBoolean("disableTrashBin", false));
        disableTrashBin.setOnCheckedChangeListener((compoundButton, bool) -> sharedPreferences.edit().putBoolean("disableTrashBin", bool).apply());
        disableTrashBin.setLayoutParams(matchWrapParams);
        disableTrashBin.setOnLongClickListener(v -> {
            new AlertDialog.Builder(SetActivity.this)
                    .setTitle(R.string.addon_ingame_disabletrashbin)
                    .setMessage(R.string.addon_ingame_disabletrashbin_info)
                    .setPositiveButton("OK", null)
                    .create().show();
            return true;
        });

        final CheckBox heavyWeapon = new CheckBox(this);
        heavyWeapon.setText(R.string.addon_ingame_heavyweaponaccl);
        heavyWeapon.setChecked(sharedPreferences.getBoolean("heavyWeaponAccel", false));
        heavyWeapon.setOnCheckedChangeListener((compoundButton, bool) -> sharedPreferences.edit().putBoolean("heavyWeaponAccel", bool).apply());
        heavyWeapon.setLayoutParams(matchWrapParams);
        heavyWeapon.setOnLongClickListener(v -> {
            new AlertDialog.Builder(SetActivity.this)
                    .setTitle(R.string.addon_ingame_heavyweaponaccl)
                    .setMessage(R.string.addon_ingame_heavyweaponaccl_info)
                    .setPositiveButton("OK", null)
                    .create().show();
            return true;
        });


        final CheckBox newShovel = new CheckBox(this);
        newShovel.setText(R.string.addon_ingame_newshovel);
        newShovel.setChecked(sharedPreferences.getBoolean("useNewShovel", true));
        newShovel.setOnCheckedChangeListener((compoundButton, bool) -> sharedPreferences.edit().putBoolean("useNewShovel", bool).apply());
        newShovel.setLayoutParams(matchWrapParams);
        newShovel.setOnLongClickListener(v -> {
            new AlertDialog.Builder(SetActivity.this)
                    .setTitle(R.string.addon_ingame_newshovel)
                    .setMessage(R.string.addon_ingame_newshovel_info)
                    .setPositiveButton("OK", null)
                    .create().show();
            return true;
        });

        final CheckBox positionAutoFix = new CheckBox(this);
        positionAutoFix.setText(R.string.addon_ingame_positionautofix);
        positionAutoFix.setChecked(sharedPreferences.getBoolean("positionAutoFix", true));
        positionAutoFix.setOnCheckedChangeListener((compoundButton, bool) -> sharedPreferences.edit().putBoolean("positionAutoFix", bool).apply());
        positionAutoFix.setLayoutParams(matchWrapParams);
        positionAutoFix.setOnLongClickListener(v -> {
            new AlertDialog.Builder(SetActivity.this)
                    .setTitle(R.string.addon_ingame_positionautofix)
                    .setMessage(R.string.addon_ingame_positionautofix_info)
                    .setPositiveButton("OK", null)
                    .create().show();
            return true;
        });

        final CheckBox dynamicPreview = new CheckBox(this);
        dynamicPreview.setText(R.string.addon_ingame_dynamicpreview);
        dynamicPreview.setChecked(sharedPreferences.getBoolean("dynamicPreview", true));
        dynamicPreview.setOnCheckedChangeListener((compoundButton, bool) -> sharedPreferences.edit().putBoolean("dynamicPreview", bool).apply());
        dynamicPreview.setLayoutParams(matchWrapParams);
        dynamicPreview.setOnLongClickListener(v -> {
            new AlertDialog.Builder(SetActivity.this)
                    .setTitle(R.string.addon_ingame_dynamicpreview)
                    .setMessage(R.string.addon_ingame_dynamicpreview_info)
                    .setPositiveButton("OK", null)
                    .create().show();
            return true;
        });

        final CheckBox newOptions = new CheckBox(this);
        newOptions.setText(R.string.addon_ingame_newoptions);
        newOptions.setChecked(sharedPreferences.getBoolean("enableNewOptionsDialog", true));
        newOptions.setOnCheckedChangeListener((compoundButton, bool) -> sharedPreferences.edit().putBoolean("enableNewOptionsDialog", bool).apply());
        newOptions.setLayoutParams(matchWrapParams);
        newOptions.setOnLongClickListener(v -> {
            new AlertDialog.Builder(SetActivity.this)
                    .setTitle(R.string.addon_ingame_newoptions)
                    .setMessage(R.string.addon_ingame_newoptions_info)
                    .setPositiveButton("OK", null)
                    .create().show();
            return true;
        });

        final CheckBox useXboxMusics = new CheckBox(this);
        useXboxMusics.setText(R.string.addon_ingame_usexboxmusics);
        useXboxMusics.setChecked(sharedPreferences.getBoolean("useXboxMusics", false));
        useXboxMusics.setOnCheckedChangeListener((compoundButton, bool) -> sharedPreferences.edit().putBoolean("useXboxMusics", bool).apply());
        useXboxMusics.setLayoutParams(matchWrapParams);
        useXboxMusics.setOnLongClickListener(v -> {
            new AlertDialog.Builder(SetActivity.this)
                    .setTitle(R.string.addon_ingame_usexboxmusics)
                    .setMessage(R.string.addon_ingame_usexboxmusics_info)
                    .setPositiveButton("OK", null)
                    .create().show();
            return true;
        });

        final CheckBox useOpenSL = new CheckBox(this);
        useOpenSL.setText(R.string.addon_ingame_useopensl);
        useOpenSL.setChecked(sharedPreferences.getBoolean("useOpenSL", false));
        useOpenSL.setOnCheckedChangeListener((compoundButton, bool) -> sharedPreferences.edit().putBoolean("useOpenSL", bool).apply());
        useOpenSL.setLayoutParams(matchWrapParams);
        useOpenSL.setOnLongClickListener(v -> {
            new AlertDialog.Builder(SetActivity.this)
                    .setTitle(R.string.addon_ingame_useopensl)
                    .setMessage(R.string.addon_ingame_useopensl_info)
                    .setPositiveButton("OK", null)
                    .create().show();
            return true;
        });

        final CheckBox playVideo = new CheckBox(this);
        playVideo.setText(R.string.addon_ingame_playvideo);
        playVideo.setChecked(sharedPreferences.getBoolean("playVideo", false));
        playVideo.setOnCheckedChangeListener((compoundButton, bool) -> sharedPreferences.edit().putBoolean("playVideo", bool).apply());
        playVideo.setLayoutParams(matchWrapParams);
        playVideo.setOnLongClickListener(v -> {
            new AlertDialog.Builder(SetActivity.this)
                    .setTitle(R.string.addon_ingame_playvideo)
                    .setMessage(R.string.addon_ingame_playvideo_info)
                    .setPositiveButton("OK", null)
                    .create().show();
            return true;
        });

        final CheckBox jumpLogo = new CheckBox(this);
        jumpLogo.setText(R.string.addon_ingame_jumplogo);
        jumpLogo.setChecked(!sharedPreferences.getBoolean("jumpLogo", false));
        jumpLogo.setOnCheckedChangeListener((compoundButton, bool) -> sharedPreferences.edit().putBoolean("jumpLogo", !bool).apply());
        jumpLogo.setLayoutParams(matchWrapParams);
        jumpLogo.setOnLongClickListener(v -> {
            new AlertDialog.Builder(SetActivity.this)
                    .setTitle(R.string.addon_ingame_jumplogo)
                    .setMessage(R.string.addon_ingame_jumplogo_info)
                    .setPositiveButton("OK", null)
                    .create().show();
            return true;
        });

        container.addView(manualCollect);
        container.addView(canShop);
        container.addView(showCoolDown);
        container.addView(disableTrashBin);
        container.addView(heavyWeapon);
        container.addView(newShovel);
        container.addView(positionAutoFix);
        container.addView(dynamicPreview);
        container.addView(newOptions);
        container.addView(useXboxMusics);
        container.addView(useOpenSL);
        container.addView(playVideo);
        container.addView(jumpLogo);
        try {
            Class.forName("com.android.support.CkHomuraMenu");
            final CheckBox useMenu = new CheckBox(this);
            useMenu.setText(R.string.addon_ingame_usemenu);
            useMenu.setChecked(sharedPreferences.getBoolean("useMenu", true));
            useMenu.setOnCheckedChangeListener((compoundButton, bool) -> sharedPreferences.edit().putBoolean("useMenu", bool).apply());
            useMenu.setLayoutParams(matchWrapParams);
            useMenu.setOnLongClickListener(v -> {
                new AlertDialog.Builder(SetActivity.this)
                        .setTitle(R.string.addon_ingame_usemenu)
                        .setMessage(R.string.addon_ingame_usemenu_info)
                        .setPositiveButton("OK", null)
                        .create().show();
                return true;
            });
            container.addView(useMenu);
        } catch (ClassNotFoundException ignored) {
        }

        // final Button customBalanceAdjustment = new Button(this);
        // customBalanceAdjustment.setText(R.string.addon_custom_balance_adjustment);
        // customBalanceAdjustment.setOnClickListener(view -> {
        //     startActivity(new Intent(SetActivity.this, CustomBalanceAdjustment.class));
        //     finish();
        // });

        // TextView customBalanceAdjustmentInfo = new TextView(this);
        // customBalanceAdjustmentInfo.setGravity(Gravity.CENTER);
        // customBalanceAdjustmentInfo.setText(R.string.addon_custom_balance_adjustment_info);
        // customBalanceAdjustmentInfo.setTextSize(15f);

        inGameLayout.setVisibility(View.GONE);
        inGameLayout.setOrientation(LinearLayout.VERTICAL);
        inGameLayout.addView(inGameInfo);
        inGameLayout.addView(container);
//        inGameLayout.addView(customBalanceAdjustment);
//        inGameLayout.addView(customBalanceAdjustmentInfo);

        String appearanceName = getString(R.string.addon_appearance_name);
        Button appearanceExpand = new Button(this);
        LinearLayout appearanceLayout = new LinearLayout(this);

        appearanceExpand.setText(String.format("▽ %s ▽", appearanceName));
        appearanceExpand.setTypeface(Typeface.DEFAULT_BOLD);
        appearanceExpand.setTextSize(15f);
        appearanceExpand.setBackgroundDrawable(oval);
        appearanceExpand.setLayoutParams(marginParams);
        appearanceExpand.setOnClickListener(view -> {
            boolean isVisible = appearanceLayout.getVisibility() == View.VISIBLE;
            ((Button) view).setText(isVisible ? String.format("▽ %s ▽", appearanceName) : String.format("△ %s △", appearanceName));
            appearanceLayout.setVisibility(isVisible ? View.GONE : View.VISIBLE);
        });

        final CheckBox hideCoverLayer = new CheckBox(this);
        hideCoverLayer.setText(R.string.addon_appearance_hidecoverlayer);
        hideCoverLayer.setChecked(sharedPreferences.getBoolean("hideCoverLayer", false));
        hideCoverLayer.setOnCheckedChangeListener((compoundButton, bool) -> sharedPreferences.edit().putBoolean("hideCoverLayer", bool).apply());
        hideCoverLayer.setPadding(0, 20, 0, 20);
        hideCoverLayer.setLayoutParams(wrapWrapParams);

        final CheckBox imitater = new CheckBox(this);
        imitater.setText(R.string.addon_appearance_imitater);
        imitater.setChecked(sharedPreferences.getBoolean("imitater", true));
        imitater.setOnCheckedChangeListener((compoundButton, bool) -> sharedPreferences.edit().putBoolean("imitater", bool).apply());
        imitater.setPadding(0, 20, 0, 20);
        imitater.setLayoutParams(wrapWrapParams);

        final CheckBox seedBankPin = new CheckBox(this);
        seedBankPin.setText(R.string.addon_appearance_seedbankpin);
        seedBankPin.setChecked(sharedPreferences.getBoolean("seedBankPin", false));
        seedBankPin.setOnCheckedChangeListener((compoundButton, bool) -> sharedPreferences.edit().putBoolean("seedBankPin", bool).apply());
        seedBankPin.setPadding(0, 20, 0, 20);
        seedBankPin.setLayoutParams(wrapWrapParams);

        final CheckBox showHouse = new CheckBox(this);
        showHouse.setText(R.string.addon_appearance_showhouse);
        showHouse.setChecked(sharedPreferences.getBoolean("showHouse", true));
        showHouse.setOnCheckedChangeListener((compoundButton, bool) -> sharedPreferences.edit().putBoolean("showHouse", bool).apply());
        showHouse.setPadding(0, 20, 0, 20);
        showHouse.setLayoutParams(wrapWrapParams);

        final CheckBox newCobCannon = new CheckBox(this);
        newCobCannon.setText(R.string.addon_appearance_newcobcannon);
        newCobCannon.setChecked(sharedPreferences.getBoolean("useNewCobCannon", true));
        newCobCannon.setOnCheckedChangeListener((compoundButton, bool) -> sharedPreferences.edit().putBoolean("useNewCobCannon", bool).apply());
        newCobCannon.setPadding(0, 20, 0, 20);
        newCobCannon.setLayoutParams(wrapWrapParams);

        Button zombatarButton = new Button(this);
        zombatarButton.setText(R.string.addon_appearance_export_zombatar);
        zombatarButton.setOnClickListener(view -> {
            File file = new File(getUserDataFile(this), "ZOMBATAR.PNG");
            if (file.exists()) {
                try {
                    MediaStore.Images.Media.insertImage(getContentResolver(), file.getAbsolutePath(), "", "");
                    Toast.makeText(this, R.string.addon_appearance_toast6, Toast.LENGTH_SHORT).show();
                } catch (FileNotFoundException ignored) {
                    Toast.makeText(this, R.string.addon_appearance_toast7, Toast.LENGTH_SHORT).show();
                }
            } else {
                Toast.makeText(this, R.string.addon_appearance_toast8, Toast.LENGTH_SHORT).show();
            }
        });

        appearanceLayout.setVisibility(View.GONE);
        appearanceLayout.setOrientation(LinearLayout.VERTICAL);
        appearanceLayout.addView(hideCoverLayer);
        appearanceLayout.addView(imitater);
        appearanceLayout.addView(showHouse);
        appearanceLayout.addView(newCobCannon);
        appearanceLayout.addView(seedBankPin);
        appearanceLayout.addView(zombatarButton);
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
            LinearLayout layout = new LinearLayout(this);
            pakListLayout = layout;
            layout.setOrientation(LinearLayout.VERTICAL);
            layout.setPadding(0, 20, 0, 20);
            LinearLayout.LayoutParams tmpParams = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
            tmpParams.gravity = Gravity.CENTER;
            tmpParams.width = 500;
            layout.setLayoutParams(tmpParams);
            TextView paksInfo = new TextView(this);
            paksInfo.setText(R.string.addon_appearance_paksinfo);
            paksInfo.setGravity(Gravity.CENTER);
            paksInfo.setLayoutParams(wrapWrapParams);
            refreshPakListUi();
            appearanceLayout.addView(paksInfo);
            appearanceLayout.addView(layout);

            Button importPak = new Button(this);
            importPak.setText(R.string.addon_appearance_importpak);
            importPak.setOnClickListener(view -> {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                intent.setType("application/zip");
                startActivityForResult(intent, REQUEST_CODE_IMPORT_PAK);
            });
            appearanceLayout.addView(importPak);
        }

        String adventureName = getString(R.string.addon_adventure_name);
        Button adventureExpand = new Button(this);
        LinearLayout adventureLayout = new LinearLayout(this);
        adventureLayout.setGravity(Gravity.CENTER);

        adventureExpand.setText(String.format("▽ %s ▽", adventureName));
        adventureExpand.setTypeface(Typeface.DEFAULT_BOLD);
        adventureExpand.setTextSize(15f);
        adventureExpand.setBackgroundDrawable(oval);
        adventureExpand.setLayoutParams(marginParams);
        adventureExpand.setOnClickListener(view -> {
            boolean isVisible = adventureLayout.getVisibility() == View.VISIBLE;
            ((Button) view).setText(isVisible ? String.format("▽ %s ▽", adventureName) : String.format("△ %s △", adventureName));
            adventureLayout.setVisibility(isVisible ? View.GONE : View.VISIBLE);
        });

        TextView hardAdventureInfo = new TextView(this);
        hardAdventureInfo.setGravity(Gravity.CENTER);
        hardAdventureInfo.setText(R.string.addon_adventure_info);
        hardAdventureInfo.setTextSize(15f);


        CheckBox normalLevel = new CheckBox(this);
        normalLevel.setText(R.string.addon_adventure_normallevel);
        normalLevel.setTextSize(16f);
        normalLevel.setPadding(0, 30, 0, 30);
        normalLevel.setChecked(sharedPreferences.getBoolean("normalLevel", true));
        normalLevel.setLayoutParams(wrapWrapParams);

        String normalLevelText1 = getString(R.string.addon_adventure_nornallevel_text1);
        String normalLevelText2 = getString(R.string.addon_adventure_nornallevel_text2);
        TextView normalLevelInfo = new TextView(this);
        normalLevelInfo.setGravity(Gravity.CENTER);
        normalLevelInfo.setText(normalLevel.isChecked() ? normalLevelText1 : normalLevelText2);
        normalLevelInfo.setTextSize(15f);


        TextView adventurePickerInfo = new TextView(this);
        adventurePickerInfo.setGravity(Gravity.CENTER);
        adventurePickerInfo.setText(String.format("%s↓", getString(R.string.addon_adventure_selectadventure)));
        adventurePickerInfo.setTextSize(15f);

        // Create a RadioGroup
        RadioGroup adventurePicker = new RadioGroup(this);
        adventurePicker.setLayoutParams(wrapWrapParams);
        adventurePicker.setOrientation(RadioGroup.VERTICAL);


        final String[] adventureFiles = new String[]{"levels-normal.xml", "levels-middle.xml", "levels-hard.xml"};
        final String[] adventureNames = new String[]{getString(R.string.addon_adventure_adventurename1), getString(R.string.addon_adventure_adventurename2), getString(R.string.addon_adventure_adventurename3)};

        for (int i = 0; i < adventureNames.length; i++) {

            // Create RadioButtons
            RadioButton radioButton = new RadioButton(this);
            radioButton.setLayoutParams(new RadioGroup.LayoutParams(
                    RadioGroup.LayoutParams.WRAP_CONTENT,
                    RadioGroup.LayoutParams.WRAP_CONTENT));
            radioButton.setText(adventureNames[i]);
            final int adventureId = i;

            radioButton.setOnClickListener(view -> {
                sharedPreferences.edit().putInt("adventureId", adventureId).apply();
                File destDir = new File(getUserDataFile(this), "properties");
                if (!destDir.exists()) {
                    destDir.mkdirs();
                }
                try {
                    InputStream is = getAssets().open("levels/" + adventureFiles[adventureId]);
                    FileOutputStream fos = new FileOutputStream(new File(destDir, "levels.xml"));
                    byte[] buffer = new byte[1024];
                    int byteCount;
                    while ((byteCount = is.read(buffer)) != -1) {// 循环从输入流读取
                        // buffer字节
                        fos.write(buffer, 0, byteCount);// 将读取的输入流写入到输出流
                    }
                    fos.flush();// 刷新缓冲区
                    is.close();
                    fos.close();
                    Toast.makeText(this, getString(R.string.addon_adventure_importlevels_toast4, adventureNames[adventureId]), Toast.LENGTH_SHORT).show();
                } catch (IOException e) {
                    throw new RuntimeException(e);
                }
            });
            adventurePicker.addView(radioButton);
        }
        RadioButton radioButton = new RadioButton(this);
        radioButton.setLayoutParams(new RadioGroup.LayoutParams(
                RadioGroup.LayoutParams.WRAP_CONTENT,
                RadioGroup.LayoutParams.WRAP_CONTENT));
        radioButton.setText(R.string.addon_adventure_importlevels);

        radioButton.setOnClickListener(view -> {
            Toast.makeText(this, R.string.addon_adventure_importlevels_toast1, Toast.LENGTH_SHORT).show();
            //通过系统的文件浏览器选择一个文件
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            //筛选，只显示可以“打开”的结果，如文件(而不是联系人或时区列表)
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("text/xml");
            startActivityForResult(intent, REQUEST_CODE_IMPORT_LEVEL_CONFIG);
        });
        adventurePicker.addView(radioButton);

        final int selectedId = sharedPreferences.getInt("adventureId", 0);
        RadioButton nthRadioButton = selectedId == -1 ? radioButton : (RadioButton) adventurePicker.getChildAt(selectedId);
        if (nthRadioButton != null) adventurePicker.check(nthRadioButton.getId());
        setRadioGroupEnabled(adventurePicker, !normalLevel.isChecked());

        String hardAdventureSelectInfoText1 = getString(R.string.addon_adventure_selectadventure_info_text1);
        String hardAdventureSelectInfoText2 = getString(R.string.addon_adventure_selectadventure_info_text2);
        TextView hardAdventureSelectInfo = new TextView(this);
        hardAdventureSelectInfo.setPadding(20, 0, 20, 0);
        hardAdventureSelectInfo.setGravity(Gravity.CENTER);
        hardAdventureSelectInfo.setText(normalLevel.isChecked() ? hardAdventureSelectInfoText1 : hardAdventureSelectInfoText2);
        hardAdventureSelectInfo.setTextSize(15f);

        normalLevel.setOnCheckedChangeListener((compoundButton, b) -> {
            setRadioGroupEnabled(adventurePicker, !b);
            normalLevelInfo.setText(b ? normalLevelText1 : normalLevelText2);
            hardAdventureSelectInfo.setText(b ? hardAdventureSelectInfoText1 : hardAdventureSelectInfoText2);
            sharedPreferences.edit().putBoolean("normalLevel", b).apply();
            if (!b) {
                int adventureId = sharedPreferences.getInt("adventureId", 0);
                if (adventureId == -1) return;
                File destDir = new File(getUserDataFile(this), "properties");
                if (!destDir.exists()) {
                    destDir.mkdirs();
                }
                try {

                    InputStream is = getAssets().open("levels/" + adventureFiles[adventureId]);
                    FileOutputStream fos = new FileOutputStream(new File(destDir, "levels.xml"));
                    byte[] buffer = new byte[1024];
                    int byteCount;
                    while ((byteCount = is.read(buffer)) != -1) {// 循环从输入流读取
                        // buffer字节
                        fos.write(buffer, 0, byteCount);// 将读取的输入流写入到输出流
                    }
                    fos.flush();// 刷新缓冲区
                    is.close();
                    fos.close();
                } catch (IOException ignored) {
                }
            }
        });

        adventureLayout.setVisibility(View.GONE);
        adventureLayout.setOrientation(LinearLayout.VERTICAL);
        adventureLayout.addView(hardAdventureInfo);
        adventureLayout.addView(normalLevel);
        adventureLayout.addView(normalLevelInfo);
        adventureLayout.addView(adventurePickerInfo);
        adventureLayout.addView(adventurePicker);
        adventureLayout.addView(hardAdventureSelectInfo);


        final Button buttonAbout = new Button(this);
        buttonAbout.setText(R.string.addon_contact_me);
        buttonAbout.setTextSize(15f);
        buttonAbout.setTypeface(Typeface.DEFAULT_BOLD);
        buttonAbout.setOnClickListener(view -> new AlertDialog.Builder(this)
                .setTitle(R.string.addon_contact_me_title)
                .setMessage(R.string.addon_contact_me_message)
                .setPositiveButton(R.string.addon_contact_me_button, (dialogInterface, i1) -> {
                    String url = "https://space.bilibili.com/480240783";
                    Intent intent = new Intent(Intent.ACTION_VIEW);
                    intent.setData(Uri.parse(url));
                    startActivity(intent);
                })
                .show());

        final Button buttonExportLog = new Button(this);
        buttonExportLog.setText(R.string.addon_export_log);
        buttonExportLog.setTextSize(15f);
        buttonExportLog.setTypeface(Typeface.DEFAULT_BOLD);
        buttonExportLog.setOnClickListener(view -> {
            var intent = new Intent(Intent.ACTION_CREATE_DOCUMENT)
                    .addCategory(Intent.CATEGORY_OPENABLE)
                    .setType("text/plain")
                    .putExtra(Intent.EXTRA_TITLE, "log.txt");
            startActivityForResult(intent, REQUEST_CODE_EXPORT_LOG);
        });

        final Button buttonOpenGame = new Button(this);
        buttonOpenGame.setText(R.string.addon_launch);
        buttonOpenGame.setTextSize(15f);
        buttonOpenGame.setTypeface(Typeface.DEFAULT_BOLD);
        buttonOpenGame.setOnClickListener(view -> {
            startActivity(new Intent(SetActivity.this, EnhanceActivity.class));
            finish();
        });

        final Button buttonHide = new Button(this);
        PackageManager packageManager = getPackageManager();
        ComponentName set = new ComponentName(getPackageName(), SetActivityEntrance.class.getName());
        buttonHide.setText(getString(
                R.string.addon_hide_self,
                (packageManager.getComponentEnabledSetting(set) == PackageManager.COMPONENT_ENABLED_STATE_DISABLED) ? getString(R.string.show) : getString(R.string.hide)
        ));
        buttonHide.setTextSize(15f);
        buttonHide.setTypeface(Typeface.DEFAULT_BOLD);
        buttonHide.setOnClickListener(view -> hideOrNot());

        linearLayout.addView(aspectExpand);
        linearLayout.addView(aspectLayout);

        linearLayout.addView(keyboardExpand);
        linearLayout.addView(keyboardLayout);

        linearLayout.addView(userdataExpand);
        linearLayout.addView(userdataLayout);

        linearLayout.addView(secondPlayerExpand);
        linearLayout.addView(secondPlayerLayout);

        linearLayout.addView(inGameExpand);
        linearLayout.addView(inGameLayout);

        linearLayout.addView(appearanceExpand);
        linearLayout.addView(appearanceLayout);

        linearLayout.addView(adventureExpand);
        linearLayout.addView(adventureLayout);

        linearLayout.addView(buttonAbout);
        linearLayout.addView(buttonExportLog);
        linearLayout.addView(buttonOpenGame);
        linearLayout.addView(buttonHide);
        scrollView.addView(linearLayout);
        setContentView(scrollView);


    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == REQUEST_CODE_IMPORT_PAK && resultCode == Activity.RESULT_OK) {
            Uri uri = data.getData();
            if (uri != null) {
                if (importExternalPak(uri)) {
                    Toast.makeText(this, R.string.addon_appearance_importpak_toast1, Toast.LENGTH_SHORT).show();
                    refreshPakListUi();
                } else {
                    Toast.makeText(this, R.string.addon_appearance_importpak_toast2, Toast.LENGTH_SHORT).show();
                }
            }
        }

        if (requestCode == REQUEST_CODE_EXPORT_LOG && resultCode == Activity.RESULT_OK && data != null && data.getData() != null) {

            Uri destination = data.getData();
            // Reading a blocked/denied logcat must not block the settings UI.
            new Thread(() -> {
                try (OutputStream out = getContentResolver().openOutputStream(destination)) {
                    if (out == null) throw new IOException();
                    LogDiagnostics.export(getApplicationContext(), out);
                } catch (Exception e) {
                    runOnUiThread(() -> Toast.makeText(getApplicationContext(), e.getMessage(), Toast.LENGTH_SHORT).show());
                }
            }, "pvztv-log-export").start();
        }

        if (requestCode == REQUEST_CODE_IMPORT_LEVEL_CONFIG && resultCode == Activity.RESULT_OK) {
            Uri uri = data.getData();
            if (uri != null) {
                try {
                    InputStream inputStream = getContentResolver().openInputStream(uri);
                    if (inputStream != null) {
                        File destDir = new File(getUserDataFile(this), "properties");
                        if (!destDir.exists()) {
                            destDir.mkdirs();
                        }
                        FileOutputStream fos = new FileOutputStream(new File(destDir, "levels.xml"));
                        byte[] buffer = new byte[1024];
                        int byteCount;
                        while ((byteCount = inputStream.read(buffer)) != -1) {// 循环从输入流读取
                            // buffer字节
                            fos.write(buffer, 0, byteCount);// 将读取的输入流写入到输出流
                        }
                        fos.flush();// 刷新缓冲区
                        inputStream.close();
                        fos.close();
                    }
                    getSharedPreferences("data", 0).edit().putInt("adventureId", -1).apply();
                    Toast.makeText(this, R.string.addon_adventure_importlevels_toast2, Toast.LENGTH_SHORT).show();
                } catch (IOException e) {
                    Toast.makeText(this, R.string.addon_adventure_importlevels_toast3, Toast.LENGTH_SHORT).show();
                    e.printStackTrace();
                }

            }
        }

        if (requestCode == REQUEST_CODE_EXPORT_SAVEDATA && resultCode == Activity.RESULT_OK) {
            Uri uri = data.getData();
            if (uri != null) {
                try {
                    OutputStream outputStream = getContentResolver().openOutputStream(uri);
                    if (outputStream != null) {
                        File userdata = new File(getUserDataFile(this), "userdata");
                        ZipOutputStream zipOutputStream = new ZipOutputStream(outputStream);
                        addFolderToZip(userdata, zipOutputStream);
                        zipOutputStream.close();
                        outputStream.close();
                    }
                    Toast.makeText(this, R.string.addon_export_toast1, Toast.LENGTH_SHORT).show();
                } catch (IOException e) {
                    Toast.makeText(this, R.string.addon_export_toast2, Toast.LENGTH_SHORT).show();
                    e.printStackTrace();
                }

            }
        }

        if (requestCode == REQUEST_CODE_IMPORT_SAVEDATA && resultCode == Activity.RESULT_OK) {
            Uri uri = data.getData();
            if (uri != null) {
                try {
                    InputStream inputStream = getContentResolver().openInputStream(uri);
                    if (inputStream != null) {
                        File userdata_import = new File(getUserDataFile(this), "userdata");
                        deleteRecursive(userdata_import);
                        if (!userdata_import.exists()) userdata_import.mkdir();
                        ZipInputStream zipInputStream = new ZipInputStream(inputStream);
                        // 读取ZipEntry对象
                        ZipEntry zipEntry = zipInputStream.getNextEntry();
                        // 遍历ZipEntry对象并解压缩文件
                        byte[] buffer = new byte[1024];
                        while (zipEntry != null) {
                            // 提取zip文件中的唯一一个目录结构下的所有文件
                            if (!zipEntry.isDirectory()) {
                                // 获取文件名并创建新文件。
                                String fileName = new File(zipEntry.getName()).getName();
                                File newFile = new File(userdata_import, fileName);
                                // 写出文件流
                                FileOutputStream fileOutputStream = new FileOutputStream(newFile);
                                int readLength;
                                while ((readLength = zipInputStream.read(buffer)) > 0) {
                                    fileOutputStream.write(buffer, 0, readLength);
                                }
                                fileOutputStream.close();
                            }
                            // 关闭当前ZipEntry，并移到下一个ZipEntry。
                            zipInputStream.closeEntry();
                            zipEntry = zipInputStream.getNextEntry();
                        }
                        // 关闭ZipInputStream对象
                        zipInputStream.close();
                        inputStream.close();
                    }
                    Toast.makeText(this, R.string.addon_import_toast1, Toast.LENGTH_SHORT).show();
                } catch (IOException e) {
                    Toast.makeText(this, R.string.addon_import_toast2, Toast.LENGTH_SHORT).show();
                    e.printStackTrace();
                }

            }
        }
    }

    private static class PakInfo {
        String readmeContent;
        Bitmap infoImage;
    }

    private static class PakItem {
        final String displayName;
        final String prefKey;
        final boolean isExternal;
        final File externalFile;

        private PakItem(String displayName, String prefKey, boolean isExternal, File externalFile) {
            this.displayName = displayName;
            this.prefKey = prefKey;
            this.isExternal = isExternal;
            this.externalFile = externalFile;
        }

        static PakItem asset(String zipFileName) {
            String displayName = zipFileName.substring(0, zipFileName.length() - 4);
            return new PakItem(displayName, zipFileName, false, null);
        }

        static PakItem external(File file) {
            String name = file.getName();
            String displayName = name.substring(0, name.length() - 4);
            return new PakItem(displayName, EXTERNAL_PAK_PREFIX + name, true, file);
        }
    }

    private List<PakItem> getExternalPakItems() {
        List<PakItem> items = new ArrayList<>();
        File pakDir = new File(getUserDataFile(this), EXTERNAL_PAK_DIR);
        File[] files = pakDir.listFiles();
        if (files == null) {
            return items;
        }
        for (File file : files) {
            if (file.isFile() && file.getName().toLowerCase(Locale.US).endsWith(".zip")) {
                items.add(PakItem.external(file));
            }
        }
        return items;
    }

    private void refreshPakListUi() {
        if (pakListLayout == null || dataSharedPreferences == null || pakItemLayoutParams == null) {
            return;
        }
        pakListLayout.removeAllViews();
        List<PakItem> pakItems = new ArrayList<>();
        try {
            String[] filesOfPaks = getAssets().list("paks");
            if (filesOfPaks != null) {
                for (String filesOfPak : filesOfPaks) {
                    if (filesOfPak.endsWith(".zip")) {
                        pakItems.add(PakItem.asset(filesOfPak));
                    }
                }
            }
        } catch (IOException ignored) {
        }
        pakItems.addAll(getExternalPakItems());
        Collections.sort(pakItems, (a, b) -> a.displayName.compareToIgnoreCase(b.displayName));
        for (PakItem pakItem : pakItems) {
            pakListLayout.addView(createPakSwitch(pakItem));
        }
    }

    private Switch createPakSwitch(PakItem pakItem) {
        Switch pakCheckBox = new Switch(this);
        pakCheckBox.setText(pakItem.displayName);
        pakCheckBox.setEllipsize(TextUtils.TruncateAt.END);
        pakCheckBox.setMaxLines(1);
        pakCheckBox.setChecked(dataSharedPreferences.getBoolean(pakItem.prefKey, false));
        pakCheckBox.setLayoutParams(pakItemLayoutParams);
        pakCheckBox.setPadding(0, 15, 0, 15);
        pakCheckBox.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked) {
                if (loadPak(pakItem)) {
                    dataSharedPreferences.edit().putBoolean(pakItem.prefKey, true).apply();
                    Toast.makeText(SetActivity.this, R.string.addon_appearance_toast1, Toast.LENGTH_SHORT).show();
                } else {
                    dataSharedPreferences.edit().putBoolean(pakItem.prefKey, false).apply();
                    Toast.makeText(SetActivity.this, R.string.addon_appearance_toast2, Toast.LENGTH_SHORT).show();
                }
            } else {
                if (unloadPak(pakItem)) {
                    dataSharedPreferences.edit().putBoolean(pakItem.prefKey, false).apply();
                    Toast.makeText(SetActivity.this, R.string.addon_appearance_toast3, Toast.LENGTH_SHORT).show();
                } else {
                    Toast.makeText(SetActivity.this, R.string.addon_appearance_toast4, Toast.LENGTH_SHORT).show();
                }
            }
        });
        pakCheckBox.setOnLongClickListener(v -> {
            showPakInfoDialog(pakItem, pakCheckBox, dataSharedPreferences);
            return true;
        });
        return pakCheckBox;
    }

    private InputStream openPakStream(PakItem pakItem) throws IOException {
        if (pakItem.isExternal && pakItem.externalFile != null) {
            return new FileInputStream(pakItem.externalFile);
        }
        return getAssets().open("paks/" + pakItem.prefKey);
    }

    private boolean loadPak(PakItem pakItem) {
        try (InputStream inputStream = openPakStream(pakItem);
             ZipInputStream zipInputStream = new ZipInputStream(inputStream)) {
            File pakFileDestDir = getUserDataFile(this);
            if (!pakFileDestDir.exists()) {
                pakFileDestDir.mkdir();
            }
            ZipEntry zipEntry = zipInputStream.getNextEntry();
            while (zipEntry != null) {
                String entryName = zipEntry.getName();
                if (zipEntry.isDirectory()) {
                    new File(pakFileDestDir, entryName).mkdirs();
                } else {
                    File target = new File(pakFileDestDir, entryName);
                    File parent = target.getParentFile();
                    if (parent != null && !parent.exists()) {
                        parent.mkdirs();
                    }
                    byte[] buffer = new byte[1024];
                    int count;
                    try (FileOutputStream fos = new FileOutputStream(target);
                         BufferedOutputStream bos = new BufferedOutputStream(fos, buffer.length)) {
                        while ((count = zipInputStream.read(buffer, 0, buffer.length)) != -1) {
                            bos.write(buffer, 0, count);
                        }
                        bos.flush();
                    }
                }
                zipInputStream.closeEntry();
                zipEntry = zipInputStream.getNextEntry();
            }
            return true;
        } catch (IOException e) {
            return false;
        }
    }

    private boolean unloadPak(PakItem pakItem) {
        try (InputStream inputStream = openPakStream(pakItem);
             ZipInputStream zipInputStream = new ZipInputStream(inputStream)) {
            File pakFileDestDir = getUserDataFile(this);
            ZipEntry zipEntry = zipInputStream.getNextEntry();
            while (zipEntry != null) {
                if (!zipEntry.isDirectory()) {
                    File file = new File(pakFileDestDir, zipEntry.getName());
                    if (file.exists()) {
                        file.delete();
                    }
                }
                zipInputStream.closeEntry();
                zipEntry = zipInputStream.getNextEntry();
            }
            return true;
        } catch (IOException e) {
            return false;
        }
    }

    private PakInfo readPakInfo(PakItem pakItem) {
        PakInfo info = new PakInfo();
        try (InputStream inputStream = openPakStream(pakItem);
             ZipInputStream zipInputStream = new ZipInputStream(inputStream)) {
            ZipEntry zipEntry = zipInputStream.getNextEntry();
            while (zipEntry != null) {
                String entryName = zipEntry.getName();
                if (!zipEntry.isDirectory() && !entryName.contains("/")) {
                    if ("readme.txt".equalsIgnoreCase(entryName)) {
                        ByteArrayOutputStream byteArrayOutputStream = new ByteArrayOutputStream();
                        byte[] buffer = new byte[1024];
                        int length;
                        while ((length = zipInputStream.read(buffer)) != -1) {
                            byteArrayOutputStream.write(buffer, 0, length);
                        }
                        info.readmeContent = byteArrayOutputStream.toString("UTF-8");
                    } else if ("info.png".equalsIgnoreCase(entryName) || "info.jpg".equalsIgnoreCase(entryName)) {
                        ByteArrayOutputStream byteArrayOutputStream = new ByteArrayOutputStream();
                        byte[] buffer = new byte[1024];
                        int length;
                        while ((length = zipInputStream.read(buffer)) != -1) {
                            byteArrayOutputStream.write(buffer, 0, length);
                        }
                        byte[] imageBytes = byteArrayOutputStream.toByteArray();
                        info.infoImage = BitmapFactory.decodeByteArray(imageBytes, 0, imageBytes.length);
                    }
                }
                zipInputStream.closeEntry();
                zipEntry = zipInputStream.getNextEntry();
            }
        } catch (IOException ignored) {
        }
        return info;
    }

    private void showPakInfoDialog(PakItem pakItem, Switch pakCheckBox, SharedPreferences sharedPreferences) {
        PakInfo pakInfo = readPakInfo(pakItem);

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(getString(R.string.addon_appearance_readmetitle, pakItem.displayName));

        LinearLayout content = new LinearLayout(this);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setPadding(40, 20, 40, 20);
        if (pakInfo.readmeContent != null) {
            TextView textView = new TextView(this);
            textView.setText(pakInfo.readmeContent);
            content.addView(textView);
        } else {
            TextView textView = new TextView(this);
            textView.setText(R.string.addon_appearance_noinfo);
            content.addView(textView);
        }
        if (pakInfo.infoImage != null) {
            ImageView imageView = new ImageView(this);
            imageView.setAdjustViewBounds(true);
            imageView.setImageBitmap(pakInfo.infoImage);
            content.addView(imageView);
        }
        ScrollView scrollView = new ScrollView(this);
        scrollView.addView(content);
        builder.setView(scrollView);
        builder.setPositiveButton("OK", null);
        if (pakItem.isExternal) {
            builder.setNeutralButton(R.string.addon_appearance_uninstallpak, (dialog, which) -> {
                if (sharedPreferences.getBoolean(pakItem.prefKey, false)) {
                    unloadPak(pakItem);
                    sharedPreferences.edit().putBoolean(pakItem.prefKey, false).apply();
                }
                if (pakItem.externalFile != null) {
                    pakItem.externalFile.delete();
                }
                Toast.makeText(this, R.string.addon_appearance_uninstallpak_toast1, Toast.LENGTH_SHORT).show();
                ViewGroup parent = (ViewGroup) pakCheckBox.getParent();
                if (parent != null) {
                    parent.removeView(pakCheckBox);
                }
            });
        }
        builder.create().show();
    }

    private boolean importExternalPak(Uri uri) {
        String rawName = getDisplayNameFromUri(uri);
        if (rawName == null && uri.getPath() != null) {
            rawName = new File(uri.getPath()).getName();
        }
        if (rawName == null || !rawName.toLowerCase(Locale.US).endsWith(".zip")) {
            rawName = "external-" + System.currentTimeMillis() + ".zip";
        }
        String safeName = sanitizePakFileName(rawName);
        if (!safeName.toLowerCase(Locale.US).endsWith(".zip")) {
            safeName += ".zip";
        }
        File pakDir = new File(getUserDataFile(this), EXTERNAL_PAK_DIR);
        if (!pakDir.exists()) {
            pakDir.mkdirs();
        }
        File dest = new File(pakDir, safeName);
        int suffix = 1;
        while (dest.exists()) {
            String baseName = safeName.substring(0, safeName.length() - 4);
            dest = new File(pakDir, baseName + "_" + suffix + ".zip");
            suffix++;
        }
        try (InputStream inputStream = getContentResolver().openInputStream(uri);
             FileOutputStream fos = new FileOutputStream(dest)) {
            if (inputStream == null) {
                return false;
            }
            byte[] buffer = new byte[1024];
            int read;
            while ((read = inputStream.read(buffer)) != -1) {
                fos.write(buffer, 0, read);
            }
            fos.flush();
            return true;
        } catch (IOException e) {
            return false;
        }
    }

    private String getDisplayNameFromUri(Uri uri) {
        Cursor cursor = null;
        try {
            cursor = getContentResolver().query(uri, new String[]{OpenableColumns.DISPLAY_NAME}, null, null, null);
            if (cursor != null && cursor.moveToFirst()) {
                int index = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (index >= 0) {
                    return cursor.getString(index);
                }
            }
        } catch (Exception ignored) {
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return null;
    }

    private String sanitizePakFileName(String fileName) {
        if (fileName == null || fileName.trim().isEmpty()) {
            return "external-" + System.currentTimeMillis() + ".zip";
        }
        String sanitized = fileName
                .replace("/", "_")
                .replace("\\", "_")
                .replace(":", "_")
                .replace("*", "_")
                .replace("?", "_")
                .replace("\"", "_")
                .replace("<", "_")
                .replace(">", "_")
                .replace("|", "_");
        sanitized = sanitized.trim();
        if (sanitized.isEmpty() || ".".equals(sanitized) || "..".equals(sanitized)) {
            sanitized = "external-" + System.currentTimeMillis() + ".zip";
        }
        return sanitized;
    }

    @Override
    public void onBackPressed() {
        finish();
        super.onBackPressed();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuItem hideSelfItem = menu.add(Menu.NONE, 1, Menu.NONE, R.string.addon_hide_self_itemname);
        hideSelfItem.setCheckable(true);

        menu.add(Menu.NONE, 2, Menu.NONE, R.string.about);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        menu.findItem(1).setChecked(getPackageManager().getComponentEnabledSetting(new ComponentName(getPackageName(), SetActivityEntrance.class.getName())) == PackageManager.COMPONENT_ENABLED_STATE_DISABLED);
        return super.onPrepareOptionsMenu(menu);
    }

    @Override
    public boolean onMenuItemSelected(int i, MenuItem menuItem) {
        switch (menuItem.getItemId()) {
            case 1:
                hideOrNot();
                break;
            case 2:
                String aboutMsg = String.join("\n",
                        getString(R.string.about_version, BuildConfig.VERSION_NAME),
                        getString(R.string.about_branch, BuildConfig.GIT_BRANCH),
                        getString(R.string.about_commit_num, BuildConfig.GIT_COMMIT_NUM),
                        getString(R.string.about_commit_hash, BuildConfig.GIT_COMMIT_HASH)
                );
                new AlertDialog.Builder(this)
                        .setTitle(R.string.about)
                        .setMessage(aboutMsg)
                        .setNegativeButton(android.R.string.copy, (dialogInterface, n) -> {
                            var clipboard = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
                            clipboard.setPrimaryClip(
                                    ClipData.newPlainText(getString(R.string.about), aboutMsg)
                            );
                            Toast.makeText(
                                    this,
                                    getString(R.string.copied_to_clipboard),
                                    Toast.LENGTH_SHORT
                            ).show();
                        })
                        .setPositiveButton(R.string.source_code, (dialogInterface, n) -> {
                            Intent intent = new Intent(Intent.ACTION_VIEW)
                                    .setData(Uri.parse(BuildConfig.GIT_REPO_URL));
                            startActivity(intent);
                        }).show();
                break;
            default:
                break;
        }
        return super.onMenuItemSelected(i, menuItem);
    }

    public void hideOrNot() {
        PackageManager packageManager = getPackageManager();
        ComponentName set = new ComponentName(getPackageName(), SetActivityEntrance.class.getName());
        if (packageManager.getComponentEnabledSetting(set) == PackageManager.COMPONENT_ENABLED_STATE_DISABLED) {
            new AlertDialog.Builder(this)
                    .setTitle(R.string.addon_hide_self_title1)
                    .setMessage(R.string.addon_hide_self_message1)
                    .setNegativeButton(android.R.string.cancel, null)
                    .setPositiveButton(android.R.string.ok, (dialogInterface, i1) -> packageManager.setComponentEnabledSetting(set, PackageManager.COMPONENT_ENABLED_STATE_ENABLED, 0))
                    .show();
        } else {
            new AlertDialog.Builder(this)
                    .setTitle(R.string.addon_hide_self_title2)
                    .setMessage(R.string.addon_hide_self_message2)
                    .setNegativeButton(android.R.string.cancel, null)
                    .setPositiveButton(android.R.string.ok, (dialogInterface, i1) -> packageManager.setComponentEnabledSetting(set, PackageManager.COMPONENT_ENABLED_STATE_DISABLED, 0))
                    .show();
        }
    }

    private void setRadioGroupEnabled(RadioGroup radioGroup, boolean enabled) {
        int count = radioGroup.getChildCount();
        for (int i = 0; i < count; i++) {
            radioGroup.getChildAt(i).setEnabled(enabled);
        }
    }

    public static class PopFragment extends DialogFragment {

        @Override
        public void onActivityCreated(Bundle savedInstanceState) {

            Window window = getDialog().getWindow();
            window.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
            super.onActivityCreated(savedInstanceState);
        }

        @Override
        public void onStart() {
            Window window = getDialog().getWindow();
            window.setLayout(getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE ? getResources().getDisplayMetrics().heightPixels : getResources().getDisplayMetrics().widthPixels - 150, -2);
            super.onStart();
        }

        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            Dialog dialog = new Dialog(getActivity());
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
                dialog.requestWindowFeature(Window.FEATURE_CONTENT_TRANSITIONS);
            dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
            dialog.setCanceledOnTouchOutside(true);
            return dialog;
        }

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
            List<Pair<String, Long>> backupsList = findNumDirs(getUserDataFile(getActivity()));
            ListView listView = new ListView(getActivity());
            if (backupsList.isEmpty()) {
                Toast.makeText(getActivity(), R.string.addon_auto_backup_toast1, Toast.LENGTH_SHORT).show();
                new Handler().postDelayed(this::dismiss, 1000);
                return listView;
            }
            boolean isNight = (getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_YES) == Configuration.UI_MODE_NIGHT_YES;
            float density = getResources().getDisplayMetrics().density;
            ShapeDrawable oval = new ShapeDrawable(new RoundRectShape(new float[]{20 * density, 20 * density, 20 * density, 20 * density, 20 * density, 20 * density, 20 * density, 20 * density}, null, null));
            oval.getPaint().setColor(isNight ? 0xff424c50 : Color.LTGRAY);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN)
                listView.setBackground(oval);


            listView.setAdapter(new BaseAdapter() {
                @Override
                public int getCount() {
                    return backupsList.size();
                }

                @Override
                public Object getItem(int i) {
                    return backupsList.get(i);
                }

                @Override
                public long getItemId(int i) {
                    return i;
                }

                @Override
                public View getView(int i, View view, ViewGroup viewGroup) {
                    view = new TextView(getActivity());
                    ((TextView) view).setText(backupsList.get(i).first);
                    ((TextView) view).setGravity(Gravity.CENTER);
                    ((TextView) view).setHeight((int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 40, getActivity().getResources().getDisplayMetrics()));
                    ((TextView) view).setTextSize(16f);
                    ((TextView) view).setTextColor(isNight ? Color.WHITE : Color.BLACK);
                    view.setOnClickListener(view1 -> Toast.makeText(getActivity(), R.string.addon_auto_backup_toast2, Toast.LENGTH_SHORT).show());
                    view.setOnLongClickListener(view12 -> {
                        String backupFile = backupsList.get(i).first;
                        File file = new File(getUserDataFile(getActivity()), backupFile);
                        File destFile = new File(getUserDataFile(getActivity()), "userdata");
                        deleteRecursive(destFile);
                        try {
                            copyDir(file, destFile);
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                        Toast.makeText(getActivity(), R.string.addon_auto_backup_toast3, Toast.LENGTH_SHORT).show();
                        return true;
                    });
                    return view;
                }
            });
            listView.setLayoutTransition(new LayoutTransition());
            listView.setOnItemClickListener((parent, view, position, id) -> dismiss());
            return listView;
        }
    }
}
