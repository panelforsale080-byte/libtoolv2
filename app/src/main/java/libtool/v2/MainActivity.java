package libtool.v2;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("LibToolV2");
    }

    public native void nativeSetPid(int pid);
    public native String nativeConnect(int pid);
    public native String nativeResolve();
    public native String nativeSnapshot();
    public native String nativeLog();

    private TextView status;
    private TextView log;
    private EditText pidInput;
    private Button btnConnect;
    private Button btnEsp;
    private int pid;
    private boolean auto;
    private final Handler ui = new Handler(Looper.getMainLooper());

    private final Runnable tick = new Runnable() {
        @Override
        public void run() {
            if (!auto) return;
            try {
                String r = nativeSnapshot();
                append(r);
                EspTarget[] enemies = EspState.parse(r);
                boolean ok = r.startsWith("OK|");
                EspState.update(ok && enemies.length > 0, r, enemies, EspState.lpX, EspState.lpZ, EspState.hasLp);
                status.setText(enemies.length == 0 ? trim(r, 90) : ("OK · targets=" + enemies.length));
            } catch (Throwable t) {
                append("ERR|" + t.getMessage());
            }
            ui.postDelayed(this, 200);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        status = findViewById(R.id.statusText);
        log = findViewById(R.id.logText);
        pidInput = findViewById(R.id.pidInput);
        btnConnect = findViewById(R.id.btnConnect);
        btnEsp = findViewById(R.id.btnEsp);

        bind(R.id.chkBox, v -> EspFeatures.box = v);
        bind(R.id.chkLine, v -> EspFeatures.line = v);
        bind(R.id.chkHp, v -> EspFeatures.health = v);
        bind(R.id.chkName, v -> EspFeatures.name = v);
        bind(R.id.chkDist, v -> EspFeatures.distance = v);

        status.setText("Enter own-app PID, then Connect");

        btnConnect.setOnClickListener(v -> connect());
        findViewById(R.id.btnResolve).setOnClickListener(v -> resolve());
        btnEsp.setOnClickListener(v -> toggleEsp());
        findViewById(R.id.btnOverlay).setOnClickListener(v -> startOverlay());
        findViewById(R.id.btnCopy).setOnClickListener(v -> copyLog());
    }

    private void connect() {
        Integer v = parsePid();
        if (v == null) return;
        String r = nativeConnect(v);
        append(r);
        if (!r.startsWith("OK|")) {
            EspState.connected = false;
            pid = 0;
            btnConnect.setText("Connect");
            status.setText("Connect failed");
            toast(r);
            return;
        }
        pid = v;
        EspState.connected = true;
        auto = false;
        btnEsp.setText("ESP ON");
        btnConnect.setText("Disconnect");
        status.setText("Connected PID " + pid + ". Tap Resolve in a match.");
    }

    private void resolve() {
        if (!requireConnected()) return;
        String r = nativeResolve();
        EspState.resolveLog = r;
        append(r);
        if (r.contains("READY=1") && r.contains("Instance NULL")) {
            status.setText("Klass OK · Instance NULL (open a match)");
        } else if (r.contains("READY=1")) {
            status.setText("Resolved");
        } else {
            status.setText("Resolve failed");
        }
    }

    private void toggleEsp() {
        if (!requireConnected()) return;
        auto = !auto;
        btnEsp.setText(auto ? "ESP OFF" : "ESP ON");
        if (auto) ui.post(tick);
        else ui.removeCallbacks(tick);
    }

    private void startOverlay() {
        if (!requireConnected()) return;
        if (Build.VERSION.SDK_INT >= 23 && !Settings.canDrawOverlays(this)) {
            startActivity(new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                    Uri.parse("package:" + getPackageName())));
            toast("Allow overlay");
            return;
        }
        startService(new Intent(this, OverlayService.class));
        toast("Overlay started");
    }

    private void copyLog() {
        ClipboardManager cm = (ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
        String text = EspState.resolveLog != null && !EspState.resolveLog.isEmpty()
                ? EspState.resolveLog : String.valueOf(log.getText());
        cm.setPrimaryClip(ClipData.newPlainText("esp", text));
        toast("Copied");
    }

    private Integer parsePid() {
        if (EspState.connected) {
            nativeSetPid(0);
            EspState.connected = false;
            pid = 0;
            auto = false;
            ui.removeCallbacks(tick);
            btnConnect.setText("Connect");
            btnEsp.setText("ESP ON");
            status.setText("Disconnected");
            return null;
        }
        String raw = pidInput.getText().toString().trim();
        int v;
        try {
            v = Integer.parseInt(raw);
        } catch (NumberFormatException e) {
            pidInput.setError("PID required");
            return null;
        }
        if (v <= 0 || v == android.os.Process.myPid()) {
            pidInput.setError("Own-app PID only");
            return null;
        }
        return v;
    }

    private boolean requireConnected() {
        if (pid <= 0 || !EspState.connected) {
            toast("Connect first");
            return false;
        }
        return true;
    }

    private void bind(int id, BoolFn fn) {
        CheckBox box = findViewById(id);
        fn.set(box.isChecked());
        box.setOnCheckedChangeListener((b, c) -> fn.set(c));
    }

    private void append(String s) {
        ui.post(() -> {
            String cur = log.getText() == null ? "" : log.getText().toString();
            if (cur.length() > 6000) cur = cur.substring(cur.length() - 4000);
            log.setText(cur + "\n" + s);
        });
    }

    private void toast(String s) {
        Toast.makeText(this, s, Toast.LENGTH_SHORT).show();
    }

    private static String trim(String s, int n) {
        return s.length() <= n ? s : s.substring(0, n);
    }

    @Override
    protected void onDestroy() {
        auto = false;
        ui.removeCallbacks(tick);
        super.onDestroy();
    }

    private interface BoolFn {
        void set(boolean v);
    }
}
