package org.navitproject.navit;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONTokener;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;

public class GithubMetadata {

    public static class Artifact {
        private final JSONObject artifactData;

        Artifact(JSONObject inputData) {
            artifactData = inputData;
        }

        URL getDownloadURL() {
            try {
                return new URL(artifactData.getString("browser_download_url"));
            } catch (JSONException | MalformedURLException e) {
                throw new RuntimeException(e);
            }
        }

        long getFileSize() {
            try {
                return artifactData.getLong("size");
            } catch (JSONException e) {
                throw new RuntimeException(e);
            }
        }

        String getFilename() {
            try {
                return artifactData.getString("name");
            } catch (JSONException e) {
                throw new RuntimeException(e);
            }
        }
    }

    private final String releaseName;
    private final List<Artifact> assets;

    GithubMetadata(String inputData) {
        try {
            JSONObject metadata = (JSONObject) new JSONTokener(inputData).nextValue();
            releaseName = metadata.optString("name", metadata.optString("tag_name", ""));
            JSONArray assetData = metadata.getJSONArray("assets");
            assets = new ArrayList<>(assetData.length());
            for (int i = 0; i < assetData.length(); i++) {
                assets.add(new Artifact(assetData.getJSONObject(i)));
            }
        } catch (JSONException e) {
            throw new RuntimeException("Invalid GitHub release metadata", e);
        }
    }

    String getReleaseName() {
        return releaseName;
    }

    Artifact[] getAssets() {
        return assets.toArray(new Artifact[0]);
    }
}
