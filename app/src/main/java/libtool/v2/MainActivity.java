package libtool.v2;

import android.content.Context;
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.text.InputType;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;

import androidx.appcompat.app.AppCompatActivity;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import imgui.il2cpp.tool.NativeMethods;

public class MainActivity extends AppCompatActivity {

    private static MainActivity sInstance;
    private static MenuView sMenuView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        sInstance = this;
        try {
            System.loadLibrary("toolv2");
        } catch (Throwable ignored) {
        }
        try {
            ProcessHelper.init(this);
            NativeMethods.setNativeLibDir(getApplicationInfo().nativeLibraryDir);
            NativeMethods.setApkFilesDir(getFilesDir().getAbsolutePath());
        } catch (Throwable ignored) {
        }
        sMenuView = new MenuView(this);
        setContentView(sMenuView);
    }

    public static void setSoftInputVisible(boolean visible) {
        if (sInstance == null || sMenuView == null) return;
        sMenuView.post(() -> {
            InputMethodManager imm = (InputMethodManager)
                    sInstance.getSystemService(Context.INPUT_METHOD_SERVICE);
            if (imm == null) return;
            if (visible) {
                sMenuView.setFocusableInTouchMode(true);
                sMenuView.requestFocus();
                imm.showSoftInput(sMenuView, InputMethodManager.SHOW_IMPLICIT);
            } else {
                imm.hideSoftInputFromWindow(sMenuView.getWindowToken(), 0);
            }
        });
    }

    static final class MenuView extends GLSurfaceView implements GLSurfaceView.Renderer {
        private boolean surfaceReady;

        MenuView(android.content.Context ctx) {
            super(ctx);
            setFocusable(true);
            setFocusableInTouchMode(true);
            setEGLContextClientVersion(3);
            setEGLConfigChooser(8, 8, 8, 8, 16, 0);
            getHolder().setFormat(PixelFormat.TRANSLUCENT);
            setPreserveEGLContextOnPause(true);
            setRenderer(this);
            setRenderMode(RENDERMODE_WHEN_DIRTY);
        }

        @Override
        public boolean onCheckIsTextEditor() {
            return true;
        }

        @Override
        public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
            outAttrs.inputType = InputType.TYPE_CLASS_NUMBER;
            outAttrs.imeOptions = EditorInfo.IME_FLAG_NO_EXTRACT_UI | EditorInfo.IME_ACTION_DONE;
            outAttrs.actionLabel = "Done";
            return new BaseInputConnection(this, false) {
                @Override
                public boolean commitText(CharSequence text, int newCursorPosition) {
                    if (text == null) return true;
                    for (int i = 0; i < text.length(); i++) {
                        NativeMethods.onImeCodePoint(text.charAt(i));
                    }
                    return true;
                }

                @Override
                public boolean deleteSurroundingText(int beforeLength, int afterLength) {
                    if (beforeLength > 0) {
                        NativeMethods.onImeBackspace();
                    }
                    return true;
                }
            };
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
                if (!surfaceReady) {
                    NativeMethods.onSurfaceCreated(getHolder().getSurface());
                }
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
