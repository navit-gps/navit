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

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.RelativeLayout.LayoutParams;
import android.widget.TextView;
import android.widget.Toast;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import java.util.Locale;


public class NavitAddressSearchActivity extends Activity {
    static final class NavitAddress {
        NavitAddress(int type, float latitude, float longitude, String address) {
            mResultType = type;
            mLat = latitude;
            mLon = longitude;
            mAddr = address;
        }

        final int mResultType;
        final float mLat;
        final float mLon;
        final String mAddr;
    }

    private static final String TAG                         = "NavitAddress";
    private static final int    ADDRESS_RESULT_PROGRESS_MAX = 10;

    private List<NavitAddress> mAddressesFound = null;
    private List<NavitAddress> mAddressesShown = null;
    private String             mAddressString               = "";
    private boolean            mPartialSearch               = false;
    private String             mCountry;
    private ImageButton        mCountryButton;
    private ProgressDialog mSearchResultsWait = null;
    private int mSearchResultsTowns = 0;
    private int mSearchResultsStreets = 0;
    private int mSearchResultsStreetsHn = 0;
    private long mSearchHandle = 0;
    private static String sLastAddressSearchString = "";
    private static Boolean sLastAddressPartialMatch = false;
    private static String sLastCountry = "";

    private int getDrawableID(String resourceName) {
        int drawableId = 0;
        try {
            Class<?> res = R.drawable.class;
            Field field = res.getField(resourceName);
            drawableId = field.getInt(null);
        } catch (Exception e) {
            Log.e(TAG, "Failure to get drawable id.", e);
        }
        return drawableId;
    }

    private void setCountryButtonImage() {
        // We have all images stored as drawable_nodpi resources which allows native code to manipulate them
        // without interference with android builtin choosing and scaling system. But that makes us to
        // reinvent the wheel here to show an image in android native interface.
        int[] flagIconSizes = {24, 32, 64, 96};
        int nearestSize;
        int exactSize = (int)(getResources().getDisplayMetrics().density * 24 - 0.5);
        nearestSize = flagIconSizes[0];
        for (int size: flagIconSizes) {
            nearestSize = size;
            if (exactSize <= size) {
                break;
            }
        }
        mCountryButton.setImageResource(getDrawableID("country_" + mCountry + "_" + nearestSize + "_" + nearestSize));
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Bundle extras = getIntent().getExtras();
        if (extras != null) {
            String searchString = extras.getString(("search_string"));
            if (searchString != null) {
                mPartialSearch = true;
                mAddressString = searchString;
                executeSearch();
                return;
            }
        }

        mPartialSearch = sLastAddressPartialMatch;
        mAddressString = sLastAddressSearchString;

        getWindow().setFlags(WindowManager.LayoutParams.FLAG_BLUR_BEHIND, WindowManager.LayoutParams.FLAG_BLUR_BEHIND);
        LinearLayout panel = new LinearLayout(this);
        panel.setLayoutParams(new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
        panel.setOrientation(LinearLayout.VERTICAL);

        // address: label and text field
        SharedPreferences settings = getSharedPreferences(NavitAppConfig.NAVIT_PREFS, MODE_PRIVATE);
        mCountry = settings.getString(("DefaultCountry"), null);

        if (mCountry == null) {
            Locale defaultLocale = Locale.getDefault();
            mCountry = defaultLocale.getCountry().toLowerCase(defaultLocale);
            SharedPreferences.Editor editSettings = settings.edit();
            editSettings.putString("DefaultCountry", mCountry);
            editSettings.apply();
        }

        mCountryButton = new ImageButton(this);
        setCountryButtonImage();

        mCountryButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                requestCountryDialog();
            }
        });

        // address: label and text field
        TextView addrView = new TextView(this);
        addrView.setText(getTstring(R.string.address_enter_destination)); // TRANS
        addrView.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20f);
        addrView.setLayoutParams(new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
        addrView.setPadding(4, 4, 4, 4);

        // partial match checkbox
        final CheckBox checkboxPartialMatch = new CheckBox(this);
        checkboxPartialMatch.setText(getTstring(R.string.address_partial_match)); // TRANS
        checkboxPartialMatch.setChecked(sLastAddressPartialMatch);
        checkboxPartialMatch.setGravity(Gravity.CENTER);

        final EditText address_string = new EditText(this);
        address_string.setText(sLastAddressSearchString);
        address_string.setSelectAllOnFocus(true);

        // search button
        final Button btnSearch = new Button(this);
        btnSearch.setText(getTstring(R.string.address_search_button)); // TRANS
        btnSearch.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.WRAP_CONTENT));
        btnSearch.setGravity(Gravity.CENTER);
        btnSearch.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                mPartialSearch = checkboxPartialMatch.isChecked();
                mAddressString = address_string.getText().toString();
                sLastAddressPartialMatch = mPartialSearch;
                sLastAddressSearchString = mAddressString;
                executeSearch();
            }
        });

        ListView lastAddresses = new ListView(this);
        NavitAppConfig navitConfig = (NavitAppConfig) getApplicationContext();

        final List<NavitAddress> addresses = navitConfig.getLastAddresses();
        int addressCount = addresses.size();
        if (addressCount > 0) {
            String[] strAddresses = new String[addressCount];
            for (int addrIndex = 0; addrIndex < addressCount; addrIndex++) {
                strAddresses[addrIndex] = addresses.get(addrIndex).mAddr;
            }
            ArrayAdapter<String> addressList =
                    new ArrayAdapter<>(this, android.R.layout.simple_list_item_1, strAddresses);
            lastAddresses.setAdapter(addressList);
            lastAddresses.setOnItemClickListener(new OnItemClickListener() {
                public void onItemClick(AdapterView<?> arg0, View arg1, int arg2, long arg3) {
                    NavitAddress addressSelected = addresses.get(arg2);
                    Intent resultIntent = new Intent();

                    resultIntent.putExtra("lat", addressSelected.mLat);
                    resultIntent.putExtra("lon", addressSelected.mLon);
                    resultIntent.putExtra("q", addressSelected.mAddr);

                    setResult(Activity.RESULT_OK, resultIntent);
                    finish();
                }
            });
        }

        String title = getString(R.string.address_search_title);
        this.setTitle(title);

        LinearLayout searchSettingsLayout = new LinearLayout(this);
        searchSettingsLayout.setOrientation(LinearLayout.HORIZONTAL);

        searchSettingsLayout.addView(mCountryButton);
        searchSettingsLayout.addView(checkboxPartialMatch);
        panel.addView(addrView);
        panel.addView(address_string);
        panel.addView(searchSettingsLayout);
        panel.addView(btnSearch);
        panel.addView(lastAddresses);

        setContentView(panel);
    }

    private void requestCountryDialog() {
        final String[][] all_countries = NavitGraphics.getAllCountries();

        Comparator<String[]> countryComparator = new Comparator<String[]>() {
            public int compare(String[] object1, String[] object2) {
                return object1[1].compareTo(object2[1]);
            }
        };

        Arrays.sort(all_countries, countryComparator);

        AlertDialog.Builder mapModeChooser = new AlertDialog.Builder(this);
        // ToDo also show icons and country code
        String[] countryName = new String[all_countries.length];

        for (int countryIndex = 0; countryIndex < all_countries.length; countryIndex++) {
            countryName[countryIndex] = all_countries[countryIndex][1];
        }

        mapModeChooser.setItems(countryName, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int item) {
                SharedPreferences settings = getSharedPreferences(NavitAppConfig.NAVIT_PREFS, MODE_PRIVATE);
                mCountry = all_countries[item][0];
                SharedPreferences.Editor editSettings = settings.edit();
                editSettings.putString("DefaultCountry", mCountry);
                editSettings.apply();
                setCountryButtonImage();
            }
        });

        AlertDialog d = mapModeChooser.create();
        d.getListView().setFastScrollEnabled(true);
        d.show();
    }

    //start a search on the map
    void receiveAddress(int type, float latitude, float longitude, String address) {
        Log.d(TAG, "(" + latitude + ", " + longitude + ") " + address);

        switch (type) {
            case 0:
                mSearchResultsTowns++;
                break;
            case 1:
                mSearchResultsStreets++;
                break;
            case 2:
                mSearchResultsStreetsHn++;
                break;
            default:
                Log.e(TAG,"Unexpected value: " + type);
        }
        mSearchResultsWait.setMessage(getTstring(R.string.address_search_towns) + ":"
                + mSearchResultsTowns + " "
                + getTstring(R.string.address_search_streets) + ":" + mSearchResultsStreets + "/"
                + mSearchResultsStreetsHn);

        mSearchResultsWait.setProgress(mAddressesFound.size() % (ADDRESS_RESULT_PROGRESS_MAX + 1));

        mAddressesFound.add(new NavitAddress(type, latitude, longitude, address));
    }

    void finishAddressSearch() {
        if (mAddressesFound.isEmpty()) {
            // TRANS
            Toast.makeText(getApplicationContext(),
                    getString(R.string.address_search_not_found) + "\n" + mAddressString, Toast.LENGTH_LONG).show();
            setResult(Activity.RESULT_CANCELED);
            finish();
        }
        ListView addressesFound = new ListView(this);
        addressesFound.setFastScrollEnabled(true);
        ArrayAdapter<String> addressList =
                new ArrayAdapter<>(this, android.R.layout.simple_list_item_1);

        mAddressesShown = new ArrayList<>();

        for (NavitAddress currentAddress : mAddressesFound) {
            if (currentAddress.mResultType != 0 || mSearchResultsStreets == 0) {
                addressList.add(currentAddress.mAddr);
                mAddressesShown.add(currentAddress);
            }
        }

        addressesFound.setAdapter(addressList);

        addressesFound.setOnItemClickListener(new OnItemClickListener() {
            public void onItemClick(AdapterView<?> arg0, View arg1, int arg2, long arg3) {
                NavitAddress addressSelected = mAddressesShown.get(arg2);
                Intent resultIntent = new Intent();

                resultIntent.putExtra("lat", addressSelected.mLat);
                resultIntent.putExtra("lon", addressSelected.mLon);
                resultIntent.putExtra("q", addressSelected.mAddr);

                setResult(Activity.RESULT_OK, resultIntent);
                finish();
            }
        });

        setContentView(addressesFound);
        mSearchResultsWait.dismiss();
    }

    native long callbackStartAddressSearch(int partialMatch, String country, String s);

    native void callbackCancelAddressSearch(long handle);

    @Override
    protected Dialog onCreateDialog(int id) {
        mSearchResultsWait = new ProgressDialog(this);
        mSearchResultsWait.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        mSearchResultsWait.setTitle("Loading search results");
        mSearchResultsWait.setMessage("--");
        mSearchResultsWait.setCancelable(true);
        mSearchResultsWait.setProgress(0);
        mSearchResultsWait.setMax(10);

        mAddressesFound = new ArrayList<>();
        mSearchResultsTowns = 0;
        mSearchResultsStreets = 0;
        mSearchResultsStreetsHn = 0;

        mSearchHandle = callbackStartAddressSearch(mPartialSearch ? 1 : 0, mCountry, mAddressString);

        mSearchResultsWait.setOnCancelListener(new DialogInterface.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                callbackCancelAddressSearch(mSearchHandle);
                mSearchHandle = 0;
                mSearchResultsWait.dismiss();
            }
        });
        return mSearchResultsWait;
    }

    private void executeSearch() {
        showDialog(0);
    }
}

