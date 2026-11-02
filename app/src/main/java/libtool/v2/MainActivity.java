package libtool.v2;

import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.SurfaceHolder;

import androidx.appcompat.app.AppCompatActivity;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import imgui.il2cpp.tool.NativeMethods;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        try {
            System.loadLibrary("toolv2");
        } catch (Throwable ignored) {
        }
        try {
            NativeMethods.setNativeLibDir(getApplicationInfo().nativeLibraryDir);
        } catch (Throwable ignored) {
        }
        setContentView(new MenuView(this));
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
