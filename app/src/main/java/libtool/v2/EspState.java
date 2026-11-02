package libtool.v2;

import java.util.ArrayList;

final class EspTarget {
    final float x;
    final float z;
    final String name;
    final int meters;
    final int hp;
    final int hpMax;
    final boolean monster;

    EspTarget(float x, float z, String name, int meters, int hp, int hpMax, boolean monster) {
        this.x = x;
        this.z = z;
        this.name = name;
        this.meters = meters;
        this.hp = hp;
        this.hpMax = hpMax;
        this.monster = monster;
    }
}

final class EspFeatures {
    static volatile boolean box = true;
    static volatile boolean line = true;
    static volatile boolean health = true;
    static volatile boolean name = true;
    static volatile boolean distance = true;

    private EspFeatures() {}
}

final class EspState {
    static volatile boolean ok;
    static volatile String text = "";
    static volatile String resolveLog = "";
    static volatile float lpX;
    static volatile float lpZ;
    static volatile boolean hasLp;
    static volatile boolean connected;
    static volatile EspTarget[] enemies = new EspTarget[0];
    static volatile long updatedNanos;

    private EspState() {}

    static void update(boolean okIn, String textIn, EspTarget[] enemiesIn, float lx, float lz, boolean lp) {
        ok = okIn;
        text = textIn;
        enemies = enemiesIn;
        lpX = lx;
        lpZ = lz;
        hasLp = lp;
        updatedNanos = System.nanoTime();
    }

    static EspTarget[] parse(String s) {
        if (s == null || !s.startsWith("OK|")) return new EspTarget[0];
        String[] parts = s.split("\\|");
        int n = 0;
        for (String p : parts) {
            if (p.startsWith("count=")) {
                try { n = Integer.parseInt(p.substring(6)); } catch (NumberFormatException ignored) {}
            } else if (p.startsWith("lp=")) {
                String[] lp = p.substring(3).split(",");
                if (lp.length >= 2) {
                    lpX = parseFloat(lp[0]);
                    lpZ = parseFloat(lp[1]);
                    hasLp = lpX != 0f || lpZ != 0f;
                }
            }
        }
        if (n <= 0) return new EspTarget[0];
        String payload = parts[parts.length - 1];
        if (payload.contains("=")) return new EspTarget[0];
        ArrayList<EspTarget> out = new ArrayList<>();
        for (String tok : payload.split(";")) {
            String[] p = tok.split(",");
            if (p.length < 4) continue;
            out.add(new EspTarget(
                    parseFloat(p[0]),
                    parseFloat(p[1]),
                    p[2],
                    parseInt(p[3]),
                    p.length > 4 ? parseInt(p[4]) : 0,
                    p.length > 5 ? parseInt(p[5]) : 0,
                    p.length > 6 && "M".equals(p[6])));
        }
        return out.toArray(new EspTarget[0]);
    }

    private static float parseFloat(String s) {
        try { return Float.parseFloat(s); } catch (NumberFormatException e) { return 0f; }
    }

    private static int parseInt(String s) {
        try { return Integer.parseInt(s); } catch (NumberFormatException e) { return 0; }
    }
}
