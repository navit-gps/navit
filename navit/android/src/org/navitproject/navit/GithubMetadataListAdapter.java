package org.navitproject.navit;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseExpandableListAdapter;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.IdentityHashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

public class GithubMetadataListAdapter extends BaseExpandableListAdapter {

    private static final String[][] REGION_PREFIXES = {
        {"africa-", "Africa"},
        {"antarctica-", "Antarctica"},
        {"asia-", "Asia"},
        {"central-america-", "Central America"},
        {"europe-", "Europe"},
        {"north-america-", "North America"},
        {"oceania-", "Oceania"},
        {"south-america-", "South America"},
        {"australia-", "Australia"}
    };

    private static class AssetGroup {
        final String name;
        final List<CountryGroup> countries = new ArrayList<>();

        AssetGroup(String name) {
            this.name = name;
        }
    }

    static class CountryGroup {
        final String name;
        final List<GithubMetadata.Artifact> assets = new ArrayList<>();

        CountryGroup(String name) {
            this.name = name;
        }
    }

    private final Context context;
    private final List<AssetGroup> groups = new ArrayList<>();
    private final Map<GithubMetadata.Artifact, String> downloadStates =
            new IdentityHashMap<>();
    private GithubMetadata metadata;

    GithubMetadataListAdapter(Context context) {
        this.context = context;
    }

    void setMetadata(GithubMetadata metadata) {
        this.metadata = metadata;
        groups.clear();
        LinkedHashMap<String, AssetGroup> groupsByPrefix = new LinkedHashMap<>();
        for (String[] prefix : REGION_PREFIXES) {
            groupsByPrefix.put(prefix[0], new AssetGroup(prefix[1]));
        }

        AssetGroup otherGroup = new AssetGroup("Other");
        for (GithubMetadata.Artifact asset : metadata.getAssets()) {
            String filename = asset.getFilename().toLowerCase(Locale.ROOT);
            AssetGroup matchingGroup = null;
            String matchingPrefix = null;
            for (String[] prefix : REGION_PREFIXES) {
                if (filename.startsWith(prefix[0])) {
                    matchingGroup = groupsByPrefix.get(prefix[0]);
                    matchingPrefix = prefix[0];
                    break;
                }
            }
            if (matchingGroup == null) {
                addCountryAsset(otherGroup, "Other", asset);
            } else {
                addCountryAsset(matchingGroup,
                        getCountryName(filename, matchingPrefix), asset);
            }
        }

        for (AssetGroup group : groupsByPrefix.values()) {
            if (!group.countries.isEmpty()) {
                sortCountries(group.countries);
                groups.add(group);
            }
        }
        if (!otherGroup.countries.isEmpty()) {
            sortCountries(otherGroup.countries);
            groups.add(otherGroup);
        }
        notifyDataSetChanged();
    }

    GithubMetadata getMetadata() {
        return metadata;
    }

    private static void addCountryAsset(AssetGroup group, String countryName,
            GithubMetadata.Artifact asset) {
        for (CountryGroup country : group.countries) {
            if (country.name.equals(countryName)) {
                country.assets.add(asset);
                return;
            }
        }
        CountryGroup country = new CountryGroup(countryName);
        country.assets.add(asset);
        group.countries.add(country);
    }

    private static void sortCountries(List<CountryGroup> countries) {
        countries.sort((left, right) ->
                left.name.compareToIgnoreCase(right.name));
        for (CountryGroup country : countries) {
            country.assets.sort((left, right) ->
                    left.getFilename().compareToIgnoreCase(right.getFilename()));
        }
    }

    private static String getCountryName(String filename, String regionPrefix) {
        String name = filename.substring(regionPrefix.length());
        if ("australia-".equals(regionPrefix) && name.startsWith("oceania-")) {
            name = name.substring("oceania-".length());
        }
        name = name.replaceAll("-20\\d\\d-\\d\\d-\\d\\d\\.bin$", "");

        String[] multiPartCountries = {
            "united-kingdom", "bosnia-herzegovina", "congo-brazzaville",
            "congo-democratic-republic", "costa-rica", "el-salvador",
            "equatorial-guinea", "ivory-coast", "czech-republic",
            "central-african-republic", "burkina-faso", "cape-verde",
            "dominican-republic", "northern-ireland", "papua-new-guinea",
            "saint-helena-ascension-and-tristan-da-cunha",
            "sao-tome-and-principe", "saudi-arabia", "senegal-and-gambia",
            "sierra-leone", "south-africa", "south-korea", "sri-lanka",
            "north-korea", "new-zealand", "north-macedonia"
        };
        for (String country : multiPartCountries) {
            if (name.startsWith(country + "-") || name.equals(country)) {
                name = country;
                break;
            }
        }

        if (name.indexOf('-') >= 0) {
            name = name.substring(0, name.indexOf('-'));
        }

        if ("north-america-".equals(regionPrefix) && "us".equals(name)) {
            name = "united-states";
        }

        StringBuilder displayName = new StringBuilder();
        for (String word : name.split("-")) {
            if (word.isEmpty()) {
                continue;
            }
            if (displayName.length() > 0) {
                displayName.append(' ');
            }
            displayName.append(Character.toUpperCase(word.charAt(0)))
                    .append(word.substring(1));
        }
        return displayName.length() == 0 ? "Other" : displayName.toString();
    }

    @Override
    public int getGroupCount() {
        return groups.size();
    }

    @Override
    public int getChildrenCount(int groupPosition) {
        if(groups.isEmpty()) {
            return 0;
        }
        return groups.get(groupPosition).countries.size();
    }

    @Override
    public Object getGroup(int groupPosition) {
        return groups.get(groupPosition).name;
    }

    @Override
    public Object getChild(int groupPosition, int childPosition) {
        return groups.get(groupPosition).countries.get(childPosition);
    }

    List<GithubMetadata.Artifact> getCountryAssets(int groupPosition,
            int childPosition) {
        return groups.get(groupPosition).countries.get(childPosition).assets;
    }

    int getAssetIndex(GithubMetadata.Artifact selectedAsset) {
        GithubMetadata.Artifact[] assets = metadata.getAssets();
        for (int i = 0; i < assets.length; i++) {
            if (assets[i] == selectedAsset) {
                return i;
            }
        }
        return -1;
    }

    void setDownloadState(GithubMetadata.Artifact asset, String state) {
        if (state == null) {
            downloadStates.remove(asset);
        } else {
            downloadStates.put(asset, state);
        }
        notifyDataSetChanged();
    }

    String getDownloadState(GithubMetadata.Artifact asset) {
        return downloadStates.get(asset);
    }

    @Override
    public long getGroupId(int groupPosition) {
        return groupPosition;
    }

    @Override
    public long getChildId(int groupPosition, int childPosition) {
        return childPosition;
    }

    @Override
    public boolean hasStableIds() {
        return false;
    }

    @Override
    public View getGroupView(int groupPosition, boolean isExpanded, View convertView,
            ViewGroup parent) {
        if (convertView == null) {
            convertView = LayoutInflater.from(context).inflate(
                    android.R.layout.simple_expandable_list_item_1, parent, false);
        }
        TextView textView = convertView.findViewById(android.R.id.text1);
        textView.setText((String) getGroup(groupPosition));
        return convertView;
    }

    @Override
    public View getChildView(int groupPosition, int childPosition, boolean isLastChild,
            View convertView, ViewGroup parent) {
        CountryGroup country =
                (CountryGroup) getChild(groupPosition, childPosition);
        long totalSize = 0;
        for (GithubMetadata.Artifact asset : country.assets) {
            totalSize += asset.getFileSize();
        }
        String details = String.format(Locale.getDefault(), "%.1f MB",
                totalSize / (1024.0 * 1024.0));
        if (country.assets.size() > 1) {
            details += String.format(Locale.getDefault(), " · %d options",
                    country.assets.size());
        } else {
            String state = downloadStates.get(country.assets.get(0));
            details += " · " + (state == null ? "Not downloaded" : state);
        }
        int icon = country.assets.size() == 1
                ? R.drawable.map_download : R.drawable.map_choose;
        if (country.assets.size() == 1) {
            String state = downloadStates.get(country.assets.get(0));
            if ("Downloaded".equals(state)) {
                icon = R.drawable.map_downloaded;
            } else if ("Queued".equals(state)
                    || (state != null && (state.startsWith("Downloading")
                    || state.contains("Cancelling")))) {
                icon = R.drawable.map_cancel;
            }
        }
        String description = country.assets.size() == 1 ? "Download" : "Choose map option";
        LinearLayout row = MapDownloadRow.create(context, country.name, details,
                icon, description, null);
        if (country.assets.size() == 1) {
            String state = downloadStates.get(country.assets.get(0));
            if (state != null && state.startsWith("Downloading")) {
                ProgressBar progress = new ProgressBar(context, null,
                        android.R.attr.progressBarStyleHorizontal);
                progress.setMax(100);
                progress.setProgress(MapDownloadRow.parseProgress(state));
                row.addView(progress);
            }
        }
        row.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        return row;
    }

    @Override
    public boolean isChildSelectable(int groupPosition, int childPosition) {
        return true;
    }
}
