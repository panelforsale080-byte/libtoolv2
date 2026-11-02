package imgui.il2cpp.tool;

import android.view.Surface;

import libtool.v2.MainActivity;

public final class NativeMethods {
    static {
        System.loadLibrary("toolv2");
    }

    private NativeMethods() {}

    public static native void onDrawFrame();
    public static native void onSurfaceChanged(int width, int height);
    public static native void onSurfaceCreate();
    public static native void onTouchEvent(int action, float x, int xOrig, float y, int yOrig);
    public static native void onSurfaceCreated(Surface surface);
    public static native String getWindowRect();
    public static native boolean connect(int pid);
    public static native String connectError();
    public static native void setNativeLibDir(String dir);
    public static native void setApkFilesDir(String dir);
    public static native void onImeCodePoint(int codePoint);
    public static native void onImeBackspace();

    public static void setSoftInputVisible(boolean visible) {
        MainActivity.setSoftInputVisible(visible);
    }
}
