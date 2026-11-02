package libtool.v2;

import android.app.ActivityManager;
import android.content.Context;
import android.os.Process;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/** Feeds native UI with PIDs visible to this app (ActivityManager + same-UID /proc). */
public final class ProcessHelper {

    private static Context appContext;

    private ProcessHelper() {}

    public static void init(Context context) {
        if (context != null) {
            appContext = context.getApplicationContext();
        }
    }

    private static boolean isOwnLibToolProcess(String name) {
        if (name == null || name.isEmpty()) return true;
        if ("libtool.v2".equals(name)) return true;
        if (name.startsWith("libtool.v2:")) return true;
        if (name.contains("libtool.v2")) return true;
        return false;
    }

    /** Each line: {@code pid<TAB>label} */
    public static String[] listProcessesForNative() {
        Map<Integer, String> byPid = new LinkedHashMap<>();
        final int myPid = Process.myPid();
        final int myUid = Process.myUid();

        if (appContext != null) {
            ActivityManager am = (ActivityManager) appContext.getSystemService(Context.ACTIVITY_SERVICE);
            if (am != null) {
                List<ActivityManager.RunningAppProcessInfo> procs = am.getRunningAppProcesses();
                if (procs != null) {
                    for (ActivityManager.RunningAppProcessInfo info : procs) {
                        if (info == null || info.pid <= 0 || info.pid == myPid) continue;
                        String name = info.processName != null ? info.processName : "?";
                        if (isOwnLibToolProcess(name)) continue;
                        byPid.put(info.pid, name);
                    }
                }
            }
        }

        File procDir = new File("/proc");
        File[] dirs = procDir.listFiles();
        if (dirs != null) {
            for (File d : dirs) {
                if (d == null || !d.isDirectory()) continue;
                String dirName = d.getName();
                if (dirName.isEmpty() || !dirName.chars().allMatch(Character::isDigit)) continue;
                int pid;
                try {
                    pid = Integer.parseInt(dirName);
                } catch (NumberFormatException e) {
                    continue;
                }
                if (pid <= 0 || pid == myPid) continue;

                int uid = -1;
                try {
                    File status = new File(d, "status");
                    if (status.canRead()) {
                        for (String line : readLines(status)) {
                            if (line.startsWith("Uid:")) {
                                String[] parts = line.split("\\s+");
                                if (parts.length >= 2) {
                                    uid = Integer.parseInt(parts[1]);
                                }
                                break;
                            }
                        }
                    }
                } catch (Throwable ignored) {
                }
                if (uid >= 0 && uid != myUid) continue;

                String label = readCmdline(new File(d, "cmdline"));
                if (label.isEmpty()) {
                    label = readComm(new File(d, "comm"));
                }
                if (label.isEmpty() || label.charAt(0) == '[') continue;
                if (isOwnLibToolProcess(label)) continue;
                if (label.length() > 96) label = label.substring(0, 96);
                byPid.putIfAbsent(pid, label);
            }
        }

        List<String> out = new ArrayList<>(byPid.size());
        for (Map.Entry<Integer, String> e : byPid.entrySet()) {
            out.add(e.getKey() + "\t" + e.getValue());
        }
        return out.toArray(new String[0]);
    }

    private static String readCmdline(File f) {
        try {
            if (!f.canRead()) return "";
            byte[] raw = java.nio.file.Files.readAllBytes(f.toPath());
            if (raw.length == 0) return "";
            int end = 0;
            while (end < raw.length && raw[end] != 0) end++;
            return new String(raw, 0, end, StandardCharsets.UTF_8).trim();
        } catch (Throwable ignored) {
            return "";
        }
    }

    private static String readComm(File f) {
        try {
            if (!f.canRead()) return "";
            String s = new String(java.nio.file.Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8).trim();
            return s.replace("\n", "");
        } catch (Throwable ignored) {
            return "";
        }
    }

    private static List<String> readLines(File f) throws java.io.IOException {
        List<String> lines = new ArrayList<>();
        try (java.io.BufferedReader br = new java.io.BufferedReader(new java.io.FileReader(f))) {
            String line;
            while ((line = br.readLine()) != null) lines.add(line);
        }
        return lines;
    }
}
