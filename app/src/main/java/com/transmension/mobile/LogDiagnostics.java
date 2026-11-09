/*
 * Copyright (C) 2023-2026  PvZ TV Touch Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

package com.transmension.mobile;

import android.content.Context;
import android.os.Build;
import android.util.AtomicFile;
import android.util.Log;

import com.trans.pvztv.BuildConfig;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.Locale;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.atomic.AtomicBoolean;

/** Diagnostic storage and export only; independent of game/save/network state. */
final class LogDiagnostics {
    private static final String TAG = "pvztv";
    private static final String SNAPSHOT_FILE = "snapshot-first-failure.txt";
    private static String snapshotWriteStatus = "not_attempted_in_this_process";

    private LogDiagnostics() {}

    private static String metadata() {
        return "time=" + new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS Z", Locale.ROOT).format(new Date())
                + " package=" + BuildConfig.APPLICATION_ID + " version=" + BuildConfig.VERSION_NAME
                + " build_type=" + BuildConfig.BUILD_TYPE + " commit=" + BuildConfig.GIT_COMMIT_HASH
                + " pid=" + android.os.Process.myPid() + " uid=" + android.os.Process.myUid();
    }

    private static AtomicFile snapshotFile(Context context) {
        // Internal files survive process restarts and do not follow the user-data path setting.
        return new AtomicFile(new File(context.getFilesDir(), SNAPSHOT_FILE));
    }

    static synchronized boolean saveSnapshotFailure(Context context, String report) {
        AtomicFile file = snapshotFile(context);
        FileOutputStream stream = null;
        try {
            stream = file.startWrite();
            writeLine(stream, "[SNAPSHOT_LOCAL] " + metadata());
            writeLine(stream, report);
            file.finishWrite(stream);
            snapshotWriteStatus = "saved";
            return true;
        } catch (IOException | RuntimeException e) {
            file.failWrite(stream);
            snapshotWriteStatus = "failed: " + e;
            Log.e(TAG, "[SNAPSHOT_LOCAL] save failed", e);
            return false;
        }
    }

    private static void appendSnapshot(Context context, OutputStream out) throws IOException {
        byte[] report = null;
        String status;
        String error = null;
        synchronized (LogDiagnostics.class) {
            status = snapshotWriteStatus;
            try {
                report = snapshotFile(context).readFully();
            } catch (IOException e) {
                error = e.toString();
            }
        }
        // Do not hold the storage lock while writing to a possibly slow document provider.
        writeLine(out, "\n===== local snapshot: latest first failure =====");
        writeLine(out, "[LOG_EXPORT] snapshot_write_status=" + status);
        if (report != null) out.write(report);
        else writeLine(out, "[LOG_EXPORT] local_snapshot_read_error=" + error);
    }

    static void export(Context context, OutputStream out) throws IOException {
        writeLine(out, "[LOG_EXPORT] format=1 " + metadata());
        writeLine(out, "[LOG_EXPORT] sdk=" + Build.VERSION.SDK_INT + " fingerprint=" + Build.FINGERPRINT);
        // Include the durable record even if logcat cannot run or returns no matching logs.
        appendSnapshot(context, out);
        String probe = "[LOG_EXPORT_PROBE] " + android.os.Process.myPid() + "-" + System.nanoTime();
        int probeResult = Log.println(Log.ERROR, TAG, probe);
        writeLine(out, "[LOG_EXPORT] probe=" + probe + " write_result=" + probeResult
                + " error_loggable=" + Log.isLoggable(TAG, Log.ERROR));
        for (String buffer : new String[]{"main", "crash"}) {
            String[] command = {"logcat", "-d", "-b", buffer, "-v", "threadtime", "-s",
                    "pvztv:V", "DEBUG:V", "libc:V", "crash_dump32:V", "crash_dump64:V"};
            appendCommand(out, buffer, command, probe, 10000);
        }
        out.flush();
    }

    // Separate commands identify which buffer failed; stderr is deliberately preserved.
    static void appendCommand(OutputStream out, String section, String[] command, String probe,
                              long timeoutMillis) throws IOException {
        writeLine(out, "\n===== " + section + " (stdout + stderr) =====");
        writeLine(out, "[LOG_EXPORT] command=" + Arrays.toString(command));
        Process process = null;
        Timer timer = null;
        AtomicBoolean timedOut = new AtomicBoolean();
        boolean probeFound = false;
        int lines = 0;
        String exitCode = "not_started";
        try {
            process = new ProcessBuilder(command).redirectErrorStream(true).start();
            exitCode = "unavailable";
            Process running = process;
            timer = new Timer("log-export-timeout", true);
            timer.schedule(new TimerTask() {
                @Override
                public void run() {
                    timedOut.set(true);
                    running.destroy();
                }
            }, timeoutMillis);
            try (BufferedReader reader = new BufferedReader(new InputStreamReader(
                    process.getInputStream(), StandardCharsets.UTF_8))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    writeLine(out, line);
                    lines++;
                    probeFound |= line.contains(probe);
                }
            }
            exitCode = Integer.toString(process.waitFor());
        } catch (IOException | RuntimeException e) {
            writeLine(out, "[LOG_EXPORT] " + section + " error=" + e);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            writeLine(out, "[LOG_EXPORT] " + section + " interrupted=true");
        } finally {
            if (timer != null) timer.cancel();
            if (process != null) {
                process.destroy();
                try {
                    exitCode = Integer.toString(process.exitValue());
                } catch (IllegalThreadStateException ignored) {
                    // The output still states that an exit code could not be obtained.
                }
            }
        }
        writeLine(out, "[LOG_EXPORT] section=" + section + " exit_code=" + exitCode
                + " timed_out=" + timedOut.get() + " output_lines=" + lines + " probe_found=" + probeFound);
    }

    private static void writeLine(OutputStream out, String text) throws IOException {
        out.write((text + "\n").getBytes(StandardCharsets.UTF_8));
    }
}
