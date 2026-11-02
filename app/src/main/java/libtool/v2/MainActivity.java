package libtool.v2;

import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {
    private EditText pidInput;
    private TextView status;
    private int pid;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        pidInput = findViewById(R.id.pidInput);
        status = findViewById(R.id.statusText);

        findViewById(R.id.btnConnect).setOnClickListener(v -> connect());
    }

    private void connect() {
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
        pid = v;
        Intent i = new Intent(this, FloatingService.class);
        i.putExtra("pid", pid);
        startService(i);
        status.setText("Connected PID " + pid + ". LibTool menu is open.");
        toast("Connected " + pid);
    }

    private void toast(String s) {
        Toast.makeText(this, s, Toast.LENGTH_SHORT).show();
    }
}
