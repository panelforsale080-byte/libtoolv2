package libtool.v2;

import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import imgui.il2cpp.tool.NativeMethods;

public class MainActivity extends AppCompatActivity {
    private EditText pidInput;
    private TextView status;
    private boolean nativeOk;
    private MenuView menu;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        pidInput = findViewById(R.id.pidInput);
        status = findViewById(R.id.statusText);
        try {
            Class.forName("imgui.il2cpp.tool.NativeMethods");
            nativeOk = true;
            status.setText("conRentV1 source in this APK. Enter PID, then Connect.");
        } catch (Throwable t) {
            status.setText("Native failed: " + t.getMessage());
        }
        findViewById(R.id.btnConnect).setOnClickListener(v -> connect());
    }

    private void connect() {
        if (!nativeOk) {
            toast("Native missing");
            return;
        }
        int pid;
        try {
            pid = Integer.parseInt(pidInput.getText().toString().trim());
        } catch (NumberFormatException e) {
            pidInput.setError("PID required");
            return;
        }
        if (pid <= 0 || pid == android.os.Process.myPid()) {
            pidInput.setError("Own-app PID only");
            return;
        }
        boolean ok;
        try {
            ok = NativeMethods.connect(pid);
        } catch (Throwable t) {
            status.setText("Connect native failed: " + t.getMessage());
            return;
        }
        if (!ok) {
            status.setText("Connect failed: " + NativeMethods.connectError());
            toast("Connect failed");
            return;
        }
        try {
            menu = new MenuView(this);
            setContentView(menu);
        } catch (Throwable t) {
            status.setText("Menu failed: " + t.getMessage());
            toast("Menu failed");
        }
    }

    private void toast(String s) {
        Toast.makeText(this, s, Toast.LENGTH_SHORT).show();
    }

    static final class MenuView extends GLSurfaceView implements GLSurfaceView.Renderer {
        private boolean ready;

        MenuView(android.content.Context ctx) {
            super(ctx);
            setEGLContextClientVersion(3);
            setEGLConfigChooser(8, 8, 8, 8, 0, 0);
            getHolder().setFormat(PixelFormat.TRANSLUCENT);
            setRenderer(this);
            setRenderMode(RENDERMODE_CONTINUOUSLY);
        }

        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            NativeMethods.onSurfaceCreated(getHolder().getSurface());
            NativeMethods.onSurfaceCreate();
            ready = true;
        }

        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height) {
            if (ready) NativeMethods.onSurfaceChanged(width, height);
        }

        @Override
        public void onDrawFrame(GL10 gl) {
            if (ready) NativeMethods.onDrawFrame();
        }

        @Override
        public boolean onTouchEvent(MotionEvent e) {
            if (!ready) return false;
            NativeMethods.onTouchEvent(e.getActionMasked(), e.getX(), (int) e.getRawX(), e.getY(), (int) e.getRawY());
            return true;
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            try { NativeMethods.onSurfaceCreated(null); } catch (Throwable ignored) {}
            ready = false;
            super.surfaceDestroyed(holder);
        }
    }
}
