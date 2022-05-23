package org.navitproject.navit;

// Heavily based on code from
// https://github.com/mburman/Android-File-Explore
// Version of Aug 13, 2011
// Also contributed:
// Sugan Krishnan (https://github.com/rgksugan) - Jan 2013.
//

// Project type now is Android library:
// http://developer.android.com/guide/developing/projects/projects-eclipse.html#ReferencingLibraryProject

import android.app.Activity;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Environment;
import android.os.StatFs;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FilenameFilter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;


public class FileBrowserActivity extends Activity {
    // Intent Action Constants
    public static final String INTENT_ACTION_SELECT_DIR = "ua.com.vassiliev.androidfilebrowser.SELECT_DIRECTORY_ACTION";
    private static final String INTENT_ACTION_SELECT_FILE = "ua.com.vassiliev.androidfilebrowser.SELECT_FILE_ACTION";

    // Intent parameters names constants
    public static final String startDirectoryParameter = "ua.com.vassiliev.androidfilebrowser.directoryPath";
    public static final String returnDirectoryParameter = "ua.com.vassiliev.androidfilebrowser.directoryPathRet";
    private static final String returnFileParameter = "ua.com.vassiliev.androidfilebrowser.filePathRet";
    private static final String showCannotReadParameter = "ua.com.vassiliev.androidfilebrowser.showCannotRead";
    private static final String filterExtension = "ua.com.vassiliev.androidfilebrowser.filterExtension";

    // Stores names of traversed directories
    private final ArrayList<String> mPathDirsList = new ArrayList<>();

    // Check if the first level of the directory structure is the one showing
    // private Boolean firstLvl = true;

    private static final String TAG = "F_PATH";

    private final List<Item> mFileList = new ArrayList<>();
    private File mPath = null;
    private String mChosenFile;
    // private static final int DIALOG_LOAD_FILE = 1000;

    private ArrayAdapter<Item> mAdapter;

    private boolean mShowHiddenFilesAndDirs = true;

    private boolean mDirectoryShownIsEmpty = false;

    private String mFilterFileExtension = null;

    // Action constants
    private static int sCurrentAction = -1;
    private static final int SELECT_DIRECTORY = 1;
    private static final int SELECT_FILE = 2;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // In case of
        // ua.com.vassiliev.androidfilebrowser.SELECT_DIRECTORY_ACTION
        // Expects com.mburman.fileexplore.directoryPath parameter to
        // point to the start folder.
        // If empty or null, will start from SDcard root.
        setContentView(R.layout.ua_com_vassiliev_filebrowser_layout);

        // Set action for this activity
        Intent thisInt = this.getIntent();
        sCurrentAction = SELECT_DIRECTORY;// This would be a default action in
        // case not set by intent
        if (thisInt.getAction().equalsIgnoreCase(INTENT_ACTION_SELECT_FILE)) {
            Log.d(TAG, "SELECT ACTION - SELECT FILE");
            sCurrentAction = SELECT_FILE;
        }

        mShowHiddenFilesAndDirs = thisInt.getBooleanExtra(
                                     showCannotReadParameter, true);

        mFilterFileExtension = thisInt.getStringExtra(filterExtension);

        setInitialDirectory();

        parseDirectoryPath();
        loadFileList();
        this.createFileListAdapter();
        this.initializeButtons();
        this.initializeFileListView();
        updateCurrentDirectoryTextView();
        Log.d(TAG, mPath.getAbsolutePath());
    }

    private void setInitialDirectory() {
        Intent thisInt = this.getIntent();
        String requestedStartDir = thisInt
                                   .getStringExtra(startDirectoryParameter);

        if (requestedStartDir != null && requestedStartDir.length() > 0) { // if(requestedStartDir!=null
            File tempFile = new File(requestedStartDir);
            if (tempFile.isDirectory()) {
                this.mPath = tempFile;
            }
        } // if(requestedStartDir!=null

        if (this.mPath == null) { // No or invalid directory supplied in intent parameter
            if (Environment.getExternalStorageDirectory().isDirectory()
                    && Environment.getExternalStorageDirectory().canRead()) {
                mPath = Environment.getExternalStorageDirectory();
            } else {
                mPath = new File("/");
            }
        } // if(this.path==null) {//No or invalid directory supplied in intent parameter
    } // private void setInitialDirectory() {

    private void parseDirectoryPath() {
        mPathDirsList.clear();
        String pathString = mPath.getAbsolutePath();
        String[] parts = pathString.split("/");
        int i = 0;
        while (i < parts.length) {
            mPathDirsList.add(parts[i]);
            i++;
        }
    }

    private void initializeButtons() {
        Button upDirButton = this.findViewById(R.id.upDirectoryButton);
        upDirButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                Log.d(TAG, "onclick for upDirButton");
                loadDirectoryUp();
                loadFileList();
                mAdapter.notifyDataSetChanged();
                updateCurrentDirectoryTextView();
            }
        });// upDirButton.setOnClickListener(

        Button selectFolderButton = this
                                    .findViewById(R.id.selectCurrentDirectoryButton);
        if (sCurrentAction == SELECT_DIRECTORY) {
            selectFolderButton.setOnClickListener(new OnClickListener() {
                public void onClick(View v) {
                    Log.d(TAG, "onclick for selectFolderButton");
                    returnDirectoryFinishActivity();
                }
            });
        } else { // if(sCurrentAction == this.SELECT_DIRECTORY) {
            selectFolderButton.setVisibility(View.GONE);
        } // } else {//if(sCurrentAction == this.SELECT_DIRECTORY) {
    } // private void initializeButtons() {

    private void loadDirectoryUp() {
        // present directory removed from list
        String s = mPathDirsList.remove(mPathDirsList.size() - 1);
        // path modified to exclude present directory
        mPath = new File(mPath.toString().substring(0,
                        mPath.toString().lastIndexOf(s)));
        mFileList.clear();
    }

    private void updateCurrentDirectoryTextView() {
        int i = 0;
        String curDirString = "";
        while (i < mPathDirsList.size()) {
            curDirString += mPathDirsList.get(i) + "/";
            i++;
        }
        if (mPathDirsList.size() == 0) {
            this.findViewById(R.id.upDirectoryButton).setEnabled(false);
            curDirString = "/";
        } else {
            this.findViewById(R.id.upDirectoryButton).setEnabled(true);
        }
        long freeSpace = getFreeSpace(curDirString);
        String formattedSpaceString = formatBytes(freeSpace);
        if (freeSpace == 0) {
            Log.d(TAG, "NO FREE SPACE");
            File currentDir = new File(curDirString);
            if (!currentDir.canWrite()) {
                formattedSpaceString = "NON Writable";
            }
        }

        ((Button) this.findViewById(R.id.selectCurrentDirectoryButton))
        .setText("Select\n[" + formattedSpaceString + "]");

        ((TextView) this.findViewById(R.id.currentDirectoryTextView))
        .setText("Current directory: " + curDirString);
    } // END private void updateCurrentDirectoryTextView() {

    private void showToast(String message) {
        Toast.makeText(this, message, Toast.LENGTH_LONG).show();
    }

    private void initializeFileListView() {
        ListView lView = this.findViewById(R.id.fileListView);
        LinearLayout.LayoutParams lParam = new LinearLayout.LayoutParams(
                LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT);
        lParam.setMargins(15, 5, 15, 5);
        lView.setAdapter(this.mAdapter);
        lView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View view,
                                    int position, long id) {
                mChosenFile = mFileList.get(position).mFile;
                File sel = new File(mPath + "/" + mChosenFile);
                Log.d(TAG, "Clicked:" + mChosenFile);
                if (sel.isDirectory()) {
                    if (sel.canRead()) {
                        // Adds chosen directory to list
                        mPathDirsList.add(mChosenFile);
                        mPath = new File(sel + "");
                        Log.d(TAG, "Just reloading the list");
                        loadFileList();
                        mAdapter.notifyDataSetChanged();
                        updateCurrentDirectoryTextView();
                        Log.d(TAG, mPath.getAbsolutePath());
                    } else { // if(sel.canRead()) {
                        showToast("Path does not exist or cannot be read");
                    } // } else {//if(sel.canRead()) {
                } else { // if (sel.isDirectory()) {
                    // File picked or an empty directory message clicked
                    Log.d(TAG, "item clicked");
                    if (!mDirectoryShownIsEmpty) {
                        Log.d(TAG, "File selected:" + mChosenFile);
                        returnFileFinishActivity(sel.getAbsolutePath());
                    }
                } // else {//if (sel.isDirectory()) {
            } // public void onClick(DialogInterface dialog, int which) {
        }); // lView.setOnClickListener(
    } // private void initializeFileListView() {

    private void returnDirectoryFinishActivity() {
        Intent retIntent = new Intent();
        retIntent.putExtra(returnDirectoryParameter, mPath.getAbsolutePath());
        this.setResult(RESULT_OK, retIntent);
        this.finish();
    } // END private void returnDirectoryFinishActivity() {

    private void returnFileFinishActivity(String filePath) {
        Intent retIntent = new Intent();
        retIntent.putExtra(returnFileParameter, filePath);
        this.setResult(RESULT_OK, retIntent);
        this.finish();
    } // END private void returnDirectoryFinishActivity() {

    private void loadFileList() {
        try {
            mPath.mkdirs();
        } catch (SecurityException e) {
            Log.i(TAG, "unable to write on the sd card ");
        }
        mFileList.clear();

        if (mPath.exists() && mPath.canRead()) {
            FilenameFilter filter = new FilenameFilter() {
                public boolean accept(File dir, String filename) {
                    File sel = new File(dir, filename);
                    boolean showReadableFile = mShowHiddenFilesAndDirs
                                               || sel.canRead();
                    // Filters based on whether the file is hidden or not
                    if (sCurrentAction == SELECT_DIRECTORY) {
                        return (sel.isDirectory() && showReadableFile);
                    }
                    if (sCurrentAction == SELECT_FILE) {

                        // If it is a file check the extension if provided
                        if (sel.isFile() && mFilterFileExtension != null) {
                            return (showReadableFile && sel.getName().endsWith(
                                    mFilterFileExtension));
                        }
                        return (showReadableFile);
                    }
                    return true;
                } // public boolean accept(File dir, String filename) {
            }; // FilenameFilter filter = new FilenameFilter() {

            String[] fList = mPath.list(filter);
            this.mDirectoryShownIsEmpty = false;
            for (int i = 0; i < fList.length; i++) {
                // Convert into file path
                File sel = new File(mPath, fList[i]);
                Log.d(TAG, "File:" + fList[i] + " readable:" + (Boolean.valueOf(sel.canRead())).toString());
                int drawableID = R.drawable.file_icon;
                boolean canRead = sel.canRead();
                // Set drawables
                if (sel.isDirectory()) {
                    if (canRead) {
                        drawableID = R.drawable.folder_icon;
                    } else {
                        drawableID = R.drawable.folder_icon_light;
                    }
                }
                mFileList.add(i, new Item(fList[i], drawableID, canRead));
            } // for (int i = 0; i < fList.length; i++) {
            if (mFileList.size() == 0) {
                // Log.d(LOGTAG, "This directory is empty");
                this.mDirectoryShownIsEmpty = true;
                mFileList.add(0, new Item("Directory is empty", -1, true));
            } else { // sort non empty list
                Collections.sort(mFileList, new ItemFileNameComparator());
            }
        } else {
            Log.e(TAG, "path does not exist or cannot be read");
        }
        // Log.d(TAG, "loadFileList finished");
    } // private void loadFileList() {

    private void createFileListAdapter() {
        mAdapter = new ArrayAdapter<Item>(this,
                                         android.R.layout.select_dialog_item, android.R.id.text1,
                mFileList) {
            @Override
            public View getView(int position, View convertView, ViewGroup parent) {
                // creates view
                View view = super.getView(position, convertView, parent);
                TextView textView = view
                                    .findViewById(android.R.id.text1);
                // put the image on the text view
                int drawableID = 0;
                if (mFileList.get(position).mIcon != -1) {
                    // If icon == -1, then directory is empty
                    drawableID = mFileList.get(position).mIcon;
                }
                textView.setCompoundDrawablesWithIntrinsicBounds(drawableID, 0,
                        0, 0);

                textView.setEllipsize(null);

                // add margin between image and text (support various screen
                // densities)
                // int dp5 = (int) (5 *
                // getResources().getDisplayMetrics().density + 0.5f);
                int dp3 = (int) (3 * getResources().getDisplayMetrics().density + 0.5f);
                // TODO: change next line for empty directory, so text will be
                // centered
                textView.setCompoundDrawablePadding(dp3);
                return view;
            } // public View getView(int position, View convertView, ViewGroup
        }; // adapter = new ArrayAdapter<Item>(this,
    } // private createFileListAdapter(){

    private class Item {
        String mFile;
        int mIcon;
        public boolean mCanRead;

        Item(String file, Integer icon, boolean canRead) {
            this.mFile = file;
            this.mIcon = icon;
        }

        @Override
        public String toString() {
            return mFile;
        }
    } // END private class Item {

    private class ItemFileNameComparator implements Comparator<Item> {
        public int compare(Item lhs, Item rhs) {
            return lhs.mFile.toLowerCase().compareTo(rhs.mFile.toLowerCase());
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
            Log.d(TAG, "ORIENTATION_LANDSCAPE");
        } else if (newConfig.orientation == Configuration.ORIENTATION_PORTRAIT) {
            Log.d(TAG, "ORIENTATION_PORTRAIT");
        }
        // Layout apparently changes itself, only have to provide good onMeasure
        // in custom components
        // TODO: check with keyboard
        // if(newConfig.keyboard == Configuration.KEYBOARDHIDDEN_YES)
    } // END public void onConfigurationChanged(Configuration newConfig) {

    private static long getFreeSpace(String path) {
        StatFs stat = new StatFs(path);
        return (long) stat.getAvailableBlocks() * (long) stat.getBlockSize();
    } // END public static long getFreeSpace(String path) {

    private static String formatBytes(long bytes) {
        // TODO: add flag to which part is needed (e.g. GB, MB, KB or bytes)
        String retStr = "";
        // One binary gigabyte equals 1,073,741,824 bytes.
        if (bytes > 1073741824) { // Add GB
            long gbs = bytes / 1073741824;
            retStr += (Long.valueOf(gbs)).toString() + "GB ";
            bytes = bytes - (gbs * 1073741824);
        }
        // One MB - 1048576 bytes
        if (bytes > 1048576) { // Add GB
            long mbs = bytes / 1048576;
            retStr += (Long.valueOf(mbs)).toString() + "MB ";
            bytes = bytes - (mbs * 1048576);
        }
        if (bytes > 1024) {
            long kbs = bytes / 1024;
            retStr += (Long.valueOf(kbs)).toString() + "KB";
            bytes = bytes - (kbs * 1024);
        } else {
            retStr += (Long.valueOf(bytes)).toString() + " bytes";
        }
        return retStr;
    } // public static String formatBytes(long bytes){

} // END public class FileBrowserActivity extends Activity {
