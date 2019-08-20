package org.navitproject.navit;

import android.content.Intent;

public interface NavitActivityResult {
    void onActivityResult(int requestCode, int resultCode, Intent data);
}
