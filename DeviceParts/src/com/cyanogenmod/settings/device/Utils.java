package com.cyanogenmod.settings.device;

import android.util.Log;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.SyncFailedException;
import java.lang.SecurityException;

public class Utils
{
    private static final String TAG = "DeviceParts";

    public static void writeValue(String parameter, int value) {
        FileOutputStream fos = null;
        try {
            fos = new FileOutputStream(new File(parameter));
            fos.write(String.valueOf(value).getBytes());
            fos.flush();
            // fos.getFD().sync();
        } catch (FileNotFoundException ex) {
            Log.w(TAG, "file " + parameter + " not found: " + ex);
        } catch (SyncFailedException ex) {
            Log.w(TAG, "file " + parameter + " sync failed: " + ex);
        } catch (IOException ex) {
            Log.w(TAG, "IOException trying to sync " + parameter + ": " + ex);
        } catch (RuntimeException ex) {
            Log.w(TAG, "exception while syncing file: ", ex);
        } finally {
            if (fos != null) {
                try {
                    Log.w(TAG, "file " + parameter + ": " + value);
                    fos.close();
                } catch (IOException ex) {
                    Log.w(TAG, "IOException while closing synced file: ", ex);
                } catch (RuntimeException ex) {
                    Log.w(TAG, "exception while closing file: ", ex);
                }
            }
        }
    }

    public static void setWritable(String parameter) {
        File file = new File(parameter);
        try {
            file.setWritable(true);
        } catch (SecurityException ex) {
            Log.w(TAG, "unable to set permission for file " + parameter + ": " + ex);
        }
    }

    public static void setNonWritable(String parameter) {
        File file = new File(parameter);
        try {
            file.setWritable(false);
        } catch (SecurityException ex) {
            Log.w(TAG, "unable to set permission for file " + parameter + ": " + ex);
        }
    }

}
