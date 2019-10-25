package org.navitproject.navit;


import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

class NavitUtils {


    static long getFreeSpace(String path) {

        File file = new File(path);
        return file.getUsableSpace();
    }

    static void removeFileIfExists(String source) {
        File file = new File(source);

        if (!file.exists()) {
            return;
        }

        file.delete();
    }

    static void copyFileIfExists(String source, String destination) throws IOException {
        File file = new File(source);

        if (!file.exists()) {
            return;
        }

        FileInputStream is = null;
        FileOutputStream os = null;

        try {
            is = new FileInputStream(source);
            os = new FileOutputStream(destination);

            int len;
            byte[] buffer = new byte[1024];

            while ((len = is.read(buffer)) != -1) {
                os.write(buffer, 0, len);
            }
        } finally {
            /* Close the FileStreams to prevent Resource leaks */
            if (is != null) {
                is.close();
            }

            if (os != null) {
                os.close();
            }
        }
    }

}
