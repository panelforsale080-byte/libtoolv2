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

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        pidInput = findViewById(R.id.pidInput);
        status = findViewById(R.id.statusText);
        try {
            System.loadLibrary("toolv2");
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
            toast("Connect failed");
            return;
        }
        if (!ok) {
            status.setText("Connect failed: " + NativeMethods.connectError());
            toast("Connect failed");
            return;
        }
        try {
            MenuView menu = new MenuView(this);
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
        private boolean surfaceReady;

        MenuView(android.content.Context ctx) {
            super(ctx);
            setEGLContextClientVersion(3);
            setEGLConfigChooser(8, 8, 8, 8, 16, 0);
            getHolder().setFormat(PixelFormat.TRANSLUCENT);
            setPreserveEGLContextOnPause(true);
            setRenderer(this);
            setRenderMode(RENDERMODE_WHEN_DIRTY);
        }

        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            try {
                NativeMethods.onSurfaceCreated(getHolder().getSurface());
            } catch (Throwable ignored) {
            }
        }

        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height) {
            if (width <= 0 || height <= 0) return;
            try {
                NativeMethods.onSurfaceCreate();
                NativeMethods.onSurfaceChanged(width, height);
                surfaceReady = true;
                setRenderMode(RENDERMODE_CONTINUOUSLY);
            } catch (Throwable ignored) {
                surfaceReady = false;
            }
        }

        @Override
        public void onDrawFrame(GL10 gl) {
            if (!surfaceReady) return;
            try {
                NativeMethods.onDrawFrame();
            } catch (Throwable ignored) {
            }
        }

        @Override
        public boolean onTouchEvent(MotionEvent e) {
            if (!surfaceReady) return false;
            try {
                NativeMethods.onTouchEvent(
                        e.getActionMasked(),
                        e.getX(),
                        (int) e.getRawX(),
                        e.getY(),
                        (int) e.getRawY());
            } catch (Throwable ignored) {
            }
            return true;
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            surfaceReady = false;
            try {
                NativeMethods.onSurfaceCreated(null);
            } catch (Throwable ignored) {
            }
            super.surfaceDestroyed(holder);
        }
    }
}
