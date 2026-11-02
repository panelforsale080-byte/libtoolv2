package libtool.v2;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.IBinder;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.WindowManager;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import imgui.il2cpp.tool.NativeMethods;

public class FloatingService extends Service {
    private WindowManager wm;
    private MenuView view;

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        NotificationManager nm = getSystemService(NotificationManager.class);
        if (Build.VERSION.SDK_INT >= 26) {
            nm.createNotificationChannel(new NotificationChannel("libtool", "LibTool", NotificationManager.IMPORTANCE_LOW));
            startForeground(4, new Notification.Builder(this, "libtool")
                    .setContentTitle("LibTool V2")
                    .setSmallIcon(android.R.drawable.ic_menu_compass)
                    .build());
        } else {
            startForeground(4, new Notification.Builder(this)
                    .setContentTitle("LibTool V2")
                    .setSmallIcon(android.R.drawable.ic_menu_compass)
                    .build());
        }

        wm = (WindowManager) getSystemService(WINDOW_SERVICE);
        view = new MenuView(this);
        WindowManager.LayoutParams lp = new WindowManager.LayoutParams(
                WindowManager.LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.MATCH_PARENT,
                Build.VERSION.SDK_INT >= 26
                        ? WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY
                        : WindowManager.LayoutParams.TYPE_PHONE,
                WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL
                        | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,
                PixelFormat.TRANSLUCENT);
        lp.gravity = Gravity.TOP | Gravity.START;
        wm.addView(view, lp);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        try {
            if (view != null) wm.removeView(view);
        } catch (Throwable ignored) {
        }
        super.onDestroy();
    }

    static final class MenuView extends GLSurfaceView implements GLSurfaceView.Renderer {
        MenuView(android.content.Context ctx) {
            super(ctx);
            setEGLContextClientVersion(3);
            setEGLConfigChooser(8, 8, 8, 8, 16, 0);
            getHolder().setFormat(PixelFormat.TRANSLUCENT);
            setZOrderOnTop(true);
            setRenderer(this);
            setRenderMode(RENDERMODE_CONTINUOUSLY);
        }

        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            NativeMethods.onSurfaceCreate();
            NativeMethods.onSurfaceCreated(getHolder().getSurface());
        }

        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height) {
            NativeMethods.onSurfaceChanged(width, height);
        }

        @Override
        public void onDrawFrame(GL10 gl) {
            NativeMethods.onDrawFrame();
        }

        @Override
        public boolean onTouchEvent(MotionEvent e) {
            NativeMethods.onTouchEvent(
                    e.getActionMasked(),
                    e.getX(),
                    (int) e.getRawX(),
                    e.getY(),
                    (int) e.getRawY());
            return true;
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            NativeMethods.onSurfaceCreated(null);
            super.surfaceDestroyed(holder);
        }
    }
}
