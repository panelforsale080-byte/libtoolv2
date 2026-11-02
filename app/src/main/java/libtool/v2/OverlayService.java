package libtool.v2;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.view.Gravity;
import android.view.View;
import android.view.WindowManager;

public class OverlayService extends Service {
    private WindowManager wm;
    private EspView view;
    private final Handler ui = new Handler(Looper.getMainLooper());
    private final Runnable poll = new Runnable() {
        @Override
        public void run() {
            if (view != null) view.invalidate();
            ui.postDelayed(this, 33);
        }
    };

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        NotificationManager nm = getSystemService(NotificationManager.class);
        if (Build.VERSION.SDK_INT >= 26) {
            nm.createNotificationChannel(new NotificationChannel("esp", "ESP", NotificationManager.IMPORTANCE_LOW));
            startForeground(3, new Notification.Builder(this, "esp")
                    .setContentTitle("LibTool V2")
                    .setSmallIcon(android.R.drawable.ic_menu_compass)
                    .build());
        } else {
            startForeground(3, new Notification.Builder(this)
                    .setContentTitle("LibTool V2")
                    .setSmallIcon(android.R.drawable.ic_menu_compass)
                    .build());
        }
        wm = (WindowManager) getSystemService(WINDOW_SERVICE);
        view = new EspView(this);
        WindowManager.LayoutParams lp = new WindowManager.LayoutParams(
                WindowManager.LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.MATCH_PARENT,
                Build.VERSION.SDK_INT >= 26
                        ? WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY
                        : WindowManager.LayoutParams.TYPE_PHONE,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                        | WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE
                        | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,
                PixelFormat.TRANSLUCENT);
        lp.gravity = Gravity.TOP | Gravity.START;
        wm.addView(view, lp);
        ui.post(poll);
    }

    @Override
    public void onDestroy() {
        ui.removeCallbacks(poll);
        try {
            if (view != null) wm.removeView(view);
        } catch (Throwable ignored) {
        }
        super.onDestroy();
    }

    static final class EspView extends View {
        private final Paint pBox = new Paint();
        private final Paint pLine = new Paint();
        private final Paint pName = new Paint();
        private final Paint pBarBg = new Paint();
        private final Paint pBar = new Paint();
        private final Paint pInfo = new Paint();

        EspView(Context ctx) {
            super(ctx);
            pBox.setColor(Color.WHITE);
            pBox.setStyle(Paint.Style.STROKE);
            pBox.setStrokeWidth(2.2f);
            pBox.setAntiAlias(true);
            pLine.setColor(0xB3FFFFFF);
            pLine.setStyle(Paint.Style.STROKE);
            pLine.setStrokeWidth(1.6f);
            pLine.setAntiAlias(true);
            pName.setColor(Color.WHITE);
            pName.setTextSize(22f);
            pName.setAntiAlias(true);
            pName.setTextAlign(Paint.Align.CENTER);
            pBarBg.setColor(0xDC1E1E1E);
            pBarBg.setStyle(Paint.Style.FILL);
            pBar.setStyle(Paint.Style.FILL);
            pInfo.setColor(0xFFCCCCCC);
            pInfo.setTextSize(20f);
            pInfo.setAntiAlias(true);
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            canvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);
            long age = EspState.updatedNanos == 0 ? -1
                    : (System.nanoTime() - EspState.updatedNanos) / 1_000_000L;
            boolean stale = age < 0 || age > 4000L || !EspState.ok;
            EspTarget[] enemies = stale ? new EspTarget[0] : EspState.enemies;
            canvas.drawText(stale ? "LibTool V2  waiting" : ("LibTool V2  " + enemies.length), 16f, 36f, pInfo);
            if (stale || enemies.length == 0) return;

            float lx = EspState.hasLp ? EspState.lpX : avgX(enemies);
            float lz = EspState.hasLp ? EspState.lpZ : avgZ(enemies);
            float ax = getWidth() * 0.5f;
            float ay = getHeight() * 0.82f;
            float cx = getWidth() * 0.5f;
            float cy = getHeight() * 0.48f;
            float range = 45f;
            float sx = getWidth() * 0.40f / range;
            float sy = getHeight() * 0.28f / range;
            float boxH = getHeight() / 6.5f;
            float boxW = boxH * 0.48f;

            for (EspTarget e : enemies) {
                float dx = e.x - lx;
                float dz = e.z - lz;
                float dist = (float) Math.hypot(dx, dz);
                float clamp = dist > range ? range / dist : 1f;
                float x = clamp(cx + dx * clamp * sx, 16f, getWidth() - 16f);
                float y = clamp(cy - dz * clamp * sy, 90f, getHeight() - 28f);
                float left = x - boxW / 2f;
                float top = y - boxH;
                if (EspFeatures.line) canvas.drawLine(ax, ay, x, y, pLine);
                if (EspFeatures.box) canvas.drawRoundRect(left, top, left + boxW, top + boxH, 8f, 8f, pBox);
                if (EspFeatures.name) canvas.drawText(e.name, x, top - 8f, pName);
                if (EspFeatures.health && e.hpMax > 0) {
                    float pct = clamp(e.hp / (float) e.hpMax, 0f, 1f);
                    pBar.setColor(pct < 0.3f ? 0xFFB42D2D : pct <= 0.6f ? 0xFFB4B42D : 0xFF00FF00);
                    canvas.drawRoundRect(left, top + boxH + 3f, left + boxW, top + boxH + 8f, 2f, 2f, pBarBg);
                    canvas.drawRoundRect(left, top + boxH + 3f, left + boxW * pct, top + boxH + 8f, 2f, 2f, pBar);
                }
                if (EspFeatures.distance) canvas.drawText(e.meters + "m", x, top + boxH + 24f, pName);
            }
        }

        private static float avgX(EspTarget[] e) {
            float s = 0;
            for (EspTarget t : e) s += t.x;
            return s / e.length;
        }

        private static float avgZ(EspTarget[] e) {
            float s = 0;
            for (EspTarget t : e) s += t.z;
            return s / e.length;
        }

        private static float clamp(float v, float a, float b) {
            return Math.max(a, Math.min(b, v));
        }
    }
}
