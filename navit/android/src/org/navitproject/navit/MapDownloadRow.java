package org.navitproject.navit;

import android.content.Context;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

final class MapDownloadRow {

    private MapDownloadRow() {
    }

    static LinearLayout create(Context context, String name, String details, int iconResource,
            String iconDescription, View.OnClickListener onClickListener) {
        LinearLayout row = new LinearLayout(context);
        row.setOrientation(LinearLayout.VERTICAL);
        row.setPadding(16, 8, 16, 8);
        if (onClickListener != null) {
            row.setClickable(true);
            row.setOnClickListener(onClickListener);
        }

        LinearLayout content = new LinearLayout(context);
        content.setGravity(Gravity.CENTER_VERTICAL);
        content.setOrientation(LinearLayout.HORIZONTAL);

        LinearLayout text = new LinearLayout(context);
        text.setOrientation(LinearLayout.VERTICAL);
        TextView nameView = new TextView(context);
        nameView.setText(name);
        nameView.setTextSize(18);
        text.addView(nameView);

        TextView detailsView = new TextView(context);
        detailsView.setText(details);
        detailsView.setTextSize(14);
        text.addView(detailsView);
        content.addView(text, new LinearLayout.LayoutParams(
                0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));

        FrameLayout iconButton = new FrameLayout(context);
        iconButton.setContentDescription(iconDescription);
        ImageView icon = new ImageView(context);
        icon.setImageResource(iconResource);
        iconButton.addView(icon, new FrameLayout.LayoutParams(
                40, 40, Gravity.CENTER));
        if (onClickListener != null) {
            iconButton.setClickable(true);
            iconButton.setFocusable(true);
            iconButton.setOnClickListener(onClickListener);
        }
        content.addView(iconButton, new LinearLayout.LayoutParams(48, 48));
        row.addView(content);
        return row;
    }

    static int parseProgress(String state) {
        int start = state.indexOf(' ');
        int end = state.indexOf('%');
        if (start < 0 || end <= start) {
            return 0;
        }
        try {
            return Integer.parseInt(state.substring(start + 1, end));
        } catch (NumberFormatException e) {
            return 0;
        }
    }
}
