package libtool.v2;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.IBinder;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.WindowManager;
import android.widget.Toast;

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
        try {
            startAsForeground();
            wm = (WindowManager) getSystemService(WINDOW_SERVICE);
            view = new MenuView(this);
            WindowManager.LayoutParams lp = new WindowManager.LayoutParams(
                    WindowManager.LayoutParams.MATCH_PARENT,
                    WindowManager.LayoutParams.MATCH_PARENT,
                    Build.VERSION.SDK_INT >= 26
                            ? WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY
                            : WindowManager.LayoutParams.TYPE_PHONE,
                    WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                            | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL
                            | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN
                            | WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED,
                    PixelFormat.TRANSLUCENT);
            lp.gravity = Gravity.TOP | Gravity.START;
            wm.addView(view, lp);
        } catch (Throwable t) {
            Toast.makeText(this, "Overlay failed: " + t.getMessage(), Toast.LENGTH_LONG).show();
            stopSelf();
        }
    }

    private void startAsForeground() {
        NotificationManager nm = getSystemService(NotificationManager.class);
        Notification.Builder b;
        if (Build.VERSION.SDK_INT >= 26) {
            nm.createNotificationChannel(new NotificationChannel("libtool", "LibTool", NotificationManager.IMPORTANCE_LOW));
            b = new Notification.Builder(this, "libtool");
        } else {
            b = new Notification.Builder(this);
        }
        Notification n = b.setContentTitle("LibTool V2")
                .setContentText("Menu overlay")
                .setSmallIcon(android.R.drawable.ic_menu_compass)
                .build();
        if (Build.VERSION.SDK_INT >= 34) {
            startForeground(4, n, ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE);
        } else {
            startForeground(4, n);
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        try {
            if (view != null && wm != null) wm.removeView(view);
        } catch (Throwable ignored) {
        }
        super.onDestroy();
    }

    static final class MenuView extends GLSurfaceView implements GLSurfaceView.Renderer {
        private boolean ready;

        MenuView(Context ctx) {
            super(ctx);
            setEGLContextClientVersion(3);
            setEGLConfigChooser(8, 8, 8, 8, 0, 0);
            getHolder().setFormat(PixelFormat.TRANSLUCENT);
            setZOrderOnTop(true);
            setRenderer(this);
            setRenderMode(RENDERMODE_CONTINUOUSLY);
        }

        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            try {
                NativeMethods.onSurfaceCreated(getHolder().getSurface());
                NativeMethods.onSurfaceCreate();
                ready = true;
            } catch (Throwable t) {
                ready = false;
            }
        }

        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height) {
            if (!ready) return;
            try {
                NativeMethods.onSurfaceChanged(width, height);
            } catch (Throwable ignored) {
            }
        }

        @Override
        public void onDrawFrame(GL10 gl) {
            if (!ready) return;
            try {
                NativeMethods.onDrawFrame();
            } catch (Throwable ignored) {
            }
        }

        @Override
        public boolean onTouchEvent(MotionEvent e) {
            if (!ready) return false;
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
            try {
                NativeMethods.onSurfaceCreated(null);
            } catch (Throwable ignored) {
            }
            ready = false;
            super.surfaceDestroyed(holder);
        }
    }
}
