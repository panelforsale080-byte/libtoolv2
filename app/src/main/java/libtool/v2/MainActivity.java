package libtool.v2;

import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import imgui.il2cpp.tool.NativeMethods;

public class MainActivity extends AppCompatActivity {
    private EditText pidInput;
    private TextView status;
    private boolean nativeOk;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        pidInput = findViewById(R.id.pidInput);
        status = findViewById(R.id.statusText);

        try {
            Class.forName("imgui.il2cpp.tool.NativeMethods");
            nativeOk = true;
            status.setText("Enter own-app PID, then Connect.");
        } catch (Throwable t) {
            nativeOk = false;
            status.setText("Native failed: " + t.getMessage());
        }

        findViewById(R.id.btnConnect).setOnClickListener(v -> connect());
    }

    private void connect() {
        if (!nativeOk) {
            toast("Native library missing");
            return;
        }
        String raw = pidInput.getText().toString().trim();
        int v;
        try {
            v = Integer.parseInt(raw);
        } catch (NumberFormatException e) {
            pidInput.setError("PID required");
            return;
        }
        if (v <= 0 || v == android.os.Process.myPid()) {
            pidInput.setError("Own-app PID only");
            return;
        }
        if (Build.VERSION.SDK_INT >= 23 && !Settings.canDrawOverlays(this)) {
            startActivity(new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                    Uri.parse("package:" + getPackageName())));
            toast("Allow overlay, then Connect again");
            return;
        }
        try {
            NativeMethods.getWindowRect();
        } catch (Throwable t) {
            status.setText("Native failed: " + t.getMessage());
            toast("Native failed");
            return;
        }
        Intent i = new Intent(this, FloatingService.class);
        i.putExtra("pid", v);
        try {
            if (Build.VERSION.SDK_INT >= 26)
                startForegroundService(i);
            else
                startService(i);
            status.setText("Connected PID " + v + ". LibTool menu is open.");
            toast("Connected " + v);
        } catch (Throwable t) {
            status.setText("Connect failed: " + t.getMessage());
            toast("Connect failed: " + t.getMessage());
        }
    }

    private void toast(String s) {
        Toast.makeText(this, s, Toast.LENGTH_SHORT).show();
    }
}
