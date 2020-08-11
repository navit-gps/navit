/*
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2018 Navit Team
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

import android.Manifest;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.IntentFilter.MalformedMimeTypeException;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;

import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * The TraFF receiver implementation.
 *
 * <p>This class registers the broadcast receiver for TraFF feeds, polls all registered sources once on creation,
 * receives TraFF feeds and forwards them to the traffic module for processing.</p>
 */
public class NavitTraff extends BroadcastReceiver {

    private static final String ACTION_TRAFF_GET_CAPABILITIES = "org.traffxml.traff.GET_CAPABILITIES";
    private static final String ACTION_TRAFF_FEED = "org.traffxml.traff.FEED";
    private static final String ACTION_TRAFF_POLL = "org.traffxml.traff.POLL";
    private static final String ACTION_TRAFF_SUBSCRIBE = "org.traffxml.traff.SUBSCRIBE";
    private static final String ACTION_TRAFF_SUBSCRIPTION_CHANGE = "org.traffxml.traff.SUBSCRIPTION_CHANGE";
    private static final String ACTION_TRAFF_UNSUBSCRIBE = "org.traffxml.traff.UNSUBSCRIBE";
    private static final String COLUMN_DATA = "data";
    private static final String CONTENT_SCHEMA = "content";
    private static final String EXTRA_CAPABILITIES = "capabilities";
    private static final String EXTRA_FEED = "feed";
    private static final String EXTRA_FILTER_LIST = "filter_list";
    private static final String EXTRA_PACKAGE = "package";
    private static final String EXTRA_SUBSCRIPTION_ID = "subscription_id";
    private static final String MIME_TYPE_TRAFF = "vnd.android.cursor.dir/org.traffxml.message";
    private static final int RESULT_OK = -1;
    private static final int RESULT_INTERNAL_ERROR = 7;
    private static final int RESULT_INVALID = 1;
    private static final int RESULT_SUBSCRIPTION_REJECTED = 2;
    private static final int RESULT_NOT_COVERED = 3;
    private static final int RESULT_PARTIALLY_COVERED = 4;
    private static final int RESULT_SUBSCRIPTION_UNKNOWN = 5;
    private static final String TAG = "NavitTraff";
    private final long mCbid;

    private final Context context;

    /** Active subscriptions (key is the subscription ID, value is the package ID). */
    private Map<String, String> subscriptions = new HashMap<String, String>();

    /**
     * Forwards a newly received TraFF feed to the traffic module for processing.
     *
     * <p>This is called when a TraFF feed is received.</p>
     *
     * @param id The identifier for the native callback implementation
     * @param feed The TraFF feed
     */
    public native void onFeedReceived(long id, String feed);

    /**
     * Creates a new {@code NavitTraff} instance.
     *
     * <p>Creating a new {@code NavitTraff} instance registers a broadcast receiver for TraFF broadcasts and polls all
     * registered sources once to ensure we have messages which were received by these sources before we started up.</p>
     *
     * @param context The context
     * @param cbid The callback identifier for the native method to call upon receiving a feed
     */
    NavitTraff(Context context, long cbid) {
        this.mCbid = cbid;
        this.context = context.getApplicationContext();

        /* An intent filter for TraFF 0.7 events. */
        IntentFilter traffFilter07 = new IntentFilter();
        traffFilter07.addAction(ACTION_TRAFF_FEED);

        /* An intent filter for TraFF 0.8 events. */
        IntentFilter traffFilter08 = new IntentFilter();
        traffFilter08.addAction(ACTION_TRAFF_FEED);
        traffFilter08.addDataScheme(CONTENT_SCHEMA);
        try {
            traffFilter08.addDataType(MIME_TYPE_TRAFF);
        } catch (MalformedMimeTypeException e) {
            // as long as the constant is a well-formed MIME type, this exception never gets thrown
            e.printStackTrace();
        }

        this.context.registerReceiver(this, traffFilter07);
        this.context.registerReceiver(this, traffFilter08);

        /* Broadcast a poll intent to all TraFF 0.7-only receivers */
        Intent outIntent = new Intent(ACTION_TRAFF_POLL);
        PackageManager pm = this.context.getPackageManager();
        List<ResolveInfo> receivers07 = pm.queryBroadcastReceivers(outIntent, 0);
        List<ResolveInfo> receivers08 = pm.queryBroadcastReceivers(new Intent(ACTION_TRAFF_GET_CAPABILITIES), 0);
        if (receivers07 != null) {
            /* get receivers which support only TraFF 0.7 and poll them */
            if (receivers08 != null)
                receivers07.removeAll(receivers08);
            for (ResolveInfo receiver : receivers07) {
                ComponentName cn = new ComponentName(receiver.activityInfo.applicationInfo.packageName,
                        receiver.activityInfo.name);
                outIntent = new Intent(ACTION_TRAFF_POLL);
                outIntent.setComponent(cn);
                this.context.sendBroadcast(outIntent, Manifest.permission.ACCESS_COARSE_LOCATION);
            }
        }
    }

    void close() {
        for (Map.Entry<String, String> subscription : subscriptions.entrySet()) {
            Bundle extras = new Bundle();
            extras.putString(EXTRA_SUBSCRIPTION_ID, subscription.getKey());
            sendTraffIntent(this.context, ACTION_TRAFF_UNSUBSCRIBE, null, extras, subscription.getValue(),
                    Manifest.permission.ACCESS_COARSE_LOCATION, this);
        }
        this.context.unregisterReceiver(this);
    }

    void onFilterUpdate(String filterList) {
        /* change existing subscriptions */
        for (Map.Entry<String, String> entry : subscriptions.entrySet()) {
            Bundle extras = new Bundle();
            extras.putString(EXTRA_SUBSCRIPTION_ID, entry.getKey());
            extras.putString(EXTRA_FILTER_LIST, filterList);
            sendTraffIntent(context, ACTION_TRAFF_SUBSCRIPTION_CHANGE, null, extras,
                    entry.getValue(),
                    Manifest.permission.ACCESS_COARSE_LOCATION, this);
        }

        /* set up missing subscriptions */
        PackageManager pm = this.context.getPackageManager();
        List<ResolveInfo> receivers = pm.queryBroadcastReceivers(new Intent(ACTION_TRAFF_GET_CAPABILITIES), 0);
        if (receivers != null) {
            /* filter out receivers to which we are already subscribed */
            Iterator<ResolveInfo> iter = receivers.iterator();
            while (iter.hasNext()) {
                ResolveInfo receiver = iter.next();
                if (subscriptions.containsValue(receiver.activityInfo.applicationInfo.packageName))
                    iter.remove();
            }

            for (ResolveInfo receiver : receivers) {
                Bundle extras = new Bundle();
                extras.putString(EXTRA_PACKAGE, context.getPackageName());
                extras.putString(EXTRA_FILTER_LIST, filterList);
                sendTraffIntent(context, ACTION_TRAFF_SUBSCRIBE, null, extras,
                        receiver.activityInfo.applicationInfo.packageName,
                        Manifest.permission.ACCESS_COARSE_LOCATION, this);
            }
        }
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        if (intent != null) {
            if (intent.getAction().equals(ACTION_TRAFF_FEED)) {
                Uri uri = intent.getData();
                if (uri != null) {
                    /* 0.8 feed */
                    String subscriptionId = intent.getStringExtra(EXTRA_SUBSCRIPTION_ID);
                    if (subscriptions.containsValue(subscriptionId))
                        fetchMessages(context, uri);
                    else {
                        /*
                         * If we don’t recognize the subscription, skip processing and unsubscribe.
                         * Note: if EXTRA_PACKAGE is not set, sendTraffIntent() sends the request to every
                         * manifest-declared receiver which handles the request.
                         */
                        Bundle extras = new Bundle();
                        extras.putString(EXTRA_SUBSCRIPTION_ID, subscriptionId);
                        sendTraffIntent(context, ACTION_TRAFF_UNSUBSCRIBE, null, extras,
                                intent.getStringExtra(EXTRA_PACKAGE),
                                Manifest.permission.ACCESS_COARSE_LOCATION, this);
                    }
                } else {
                    /* 0.7 feed */
                    String packageName = intent.getStringExtra(EXTRA_PACKAGE);
                    /* if the feed comes from a TraFF 0.8+ source and we are subscribed, skip it */
                    // TODO what if we don’t have a subscription yet? First subscribe, then poll (still no guarantee)
                    if ((packageName != null) && subscriptions.containsValue(packageName))
                        return;
                    String feed = intent.getStringExtra(EXTRA_FEED);
                    if (feed == null) {
                        Log.w(this.getClass().getSimpleName(), "empty feed, ignoring");
                    } else {
                        onFeedReceived(mCbid, feed);
                    }
                } // uri != null
            } else if (intent.getAction().equals(ACTION_TRAFF_SUBSCRIBE)) {
                if (this.getResultCode() != RESULT_OK)
                    return;
                Bundle extras = this.getResultExtras(true);
                String data = this.getResultData();
                String packageName = extras.getString(EXTRA_PACKAGE);
                String subscriptionId = extras.getString(EXTRA_SUBSCRIPTION_ID);
                if ((data == null) || (packageName == null) || (subscriptionId == null))
                    return;
                subscriptions.put(subscriptionId, packageName);
                fetchMessages(context, Uri.parse(data));
            } else if (intent.getAction().equals(ACTION_TRAFF_SUBSCRIPTION_CHANGE)) {
                if (this.getResultCode() != RESULT_OK)
                    return;
                Bundle extras = this.getResultExtras(true);
                String data = this.getResultData();
                String subscriptionId = extras.getString(EXTRA_SUBSCRIPTION_ID);
                if ((data == null) || (subscriptionId == null) || (!subscriptions.containsKey(subscriptionId)))
                    return;
                fetchMessages(context, Uri.parse(data));
            } else if (intent.getAction().equals(ACTION_TRAFF_UNSUBSCRIBE)) {
                /*
                 * If we ever unsubscribe for reasons other than that we are shutting down or got a feed for
                 * a subscription we don’t recognize, or if we start keeping a persistent list of
                 * subscriptions, we need to delete the subscription from our list. Until then, there is
                 * nothing to do here: either the subscription isn’t in the list, or we are about to shut
                 * down and the whole list is about to get discarded.
                 */
            } // intent.getAction()
        } // intent != null
    }

    /**
     * Fetches messages from a content provider.
     *
     * @param context The context to use for the content resolver
     * @param uri The content provider URI
     */
    private void fetchMessages(Context context, Uri uri) {
        try {
            Cursor cursor = context.getContentResolver().query(uri, new String[] {COLUMN_DATA}, null, null, null);
            if (cursor == null)
                return;
            if (cursor.getCount() < 1) {
                cursor.close();
                return;
            }
            StringBuilder builder = new StringBuilder("<feed>\n");
            while (cursor.moveToNext())
                builder.append(cursor.getString(cursor.getColumnIndex(COLUMN_DATA))).append("\n");
            builder.append("</feed>");
            cursor.close();
            onFeedReceived(mCbid, builder.toString());
        } catch (Exception e) {
            Log.w(TAG, String.format("Unable to fetch messages from %s", uri.toString()), e);
            e.printStackTrace();
        }
    }

    /**
     * Sends a TraFF intent to a source.  This encapsulates most of the low-level Android handling.
     *
     * <p>If the recipient specified in {@code packageName} declares multiple receivers for the intent in its
     * manifest, a separate intent will be delivered to each of them. The intent will not be delivered to
     * receivers registered at runtime.
     *
     * <p>All intents are sent as explicit ordered broadcasts. This means two things:
     *
     * <p>Any app which declares a matching receiver in its manifest will be woken up to process the intent.
     * This works even with certain Android 7 builds which restrict intent delivery to apps which are not
     * currently running.
     *
     * <p>It is safe for the recipient to unconditionally set result data. If the recipient does not set result
     * data, the result will have a result code of {@link #RESULT_INTERNAL_ERROR}, no data and no extras.
     *
     * @param context The context
     * @param action The intent action.
     * @param data The intent data (for TraFF, this is the content provider URI), or null
     * @param extras The extras for the intent
     * @param packageName The package name for the recipient, or null to deliver the intent to all matching receivers
     * @param receiverPermission A permission which the recipient must hold, or null if not required
     * @param resultReceiver A BroadcastReceiver which will receive the result for the intent
     */
    /* From traff-consumer-android, by the same author and re-licensed under GPL2 for Navit */
    public static void sendTraffIntent(Context context, String action, Uri data, Bundle extras, String packageName,
            String receiverPermission, BroadcastReceiver resultReceiver) {
        Intent outIntent = new Intent(action);
        PackageManager pm = context.getPackageManager();
        List<ResolveInfo> receivers = pm.queryBroadcastReceivers(outIntent, 0);
        if (receivers != null)
            for (ResolveInfo receiver : receivers) {
                if ((packageName != null) && !packageName.equals(receiver.activityInfo.applicationInfo.packageName))
                    continue;
                ComponentName cn = new ComponentName(receiver.activityInfo.applicationInfo.packageName,
                        receiver.activityInfo.name);
                outIntent = new Intent(action);
                if (data != null)
                    outIntent.setData(data);
                if (extras != null)
                    outIntent.putExtras(extras);
                outIntent.setComponent(cn);
                context.sendOrderedBroadcast(outIntent,
                        receiverPermission,
                        resultReceiver,
                        null, // scheduler,
                        RESULT_INTERNAL_ERROR, // initialCode,
                        null, // initialData,
                        null);
            }
    }
}
