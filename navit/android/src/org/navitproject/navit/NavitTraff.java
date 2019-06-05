/**
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
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.util.Log;

import java.util.List;

/**
 * @brief The TraFF receiver implementation.
 *
 * This class registers the broadcast receiver for TraFF feeds, polls all registered sources once on creation, receives
 * TraFF feeds and forwards them to the traffic module for processing.
 */
public class NavitTraff extends BroadcastReceiver {
    public static String ACTION_TRAFF_FEED = "org.traffxml.traff.FEED";

    public static String ACTION_TRAFF_POLL = "org.traffxml.traff.POLL";

    public static String EXTRA_FEED = "feed";

    /** Identifier for the callback function. */
    private int cbid;

    private Context context = null;

    /** An intent filter for TraFF events. */
    private IntentFilter traffFilter = new IntentFilter();

    /**
     * @brief Forwards a newly received TraFF feed to the traffic module for processing.
     *
     * This is called when a TraFF feed is received.
     *
     * @param id The identifier for the native callback implementation
     * @param feed The TraFF feed
     */
    public native void onFeedReceived(int id, String feed);

    /**
     * @brief Creates a new {@code NavitTraff} instance.
     *
     * Creating a new {@code NavitTraff} instance registers a broadcast receiver for TraFF broadcasts and polls all
     * registered sources once to ensure we have messages which were received by these sources before we started up.
     *
     * @param context The context
     * @param cbid The callback identifier for the native method to call upon receiving a feed
     */
    NavitTraff(Context context, int cbid) {
        this.context = context;
        this.cbid = cbid;

        traffFilter.addAction(ACTION_TRAFF_FEED);
        traffFilter.addAction(ACTION_TRAFF_POLL);

        context.registerReceiver(this, traffFilter);
        /* TODO unregister receiver on exit */

        /* Broadcast a poll intent */
        Intent outIntent = new Intent(ACTION_TRAFF_POLL);
        PackageManager pm = context.getPackageManager();
        List<ResolveInfo> receivers = pm.queryBroadcastReceivers(outIntent, 0);
        if (receivers != null)
            for (ResolveInfo receiver : receivers) {
                ComponentName cn = new ComponentName(receiver.activityInfo.applicationInfo.packageName,
                        receiver.activityInfo.name);
                outIntent = new Intent(ACTION_TRAFF_POLL);
                outIntent.setComponent(cn);
                context.sendBroadcast(outIntent, Manifest.permission.ACCESS_COARSE_LOCATION);
            }
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        if ((intent != null) && (intent.getAction().equals(ACTION_TRAFF_FEED))) {
            String feed = intent.getStringExtra(EXTRA_FEED);
            if (feed == null)
                Log.w(this.getClass().getSimpleName(), "empty feed, ignoring");
            else
                onFeedReceived(cbid, feed);
        }
    }
}
