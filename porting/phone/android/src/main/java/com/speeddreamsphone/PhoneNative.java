package com.speeddreamsphone;

/**
 * JNI entry points for the flat-screen phone input backend.
 *
 * The native library contains the shared input state used by both the Android
 * touch overlay and the engine's virtual joystick / GUI-event adapter.
 */
public final class PhoneNative {
    public static final int TOUCH_MOVE = 0;
    public static final int TOUCH_DOWN = 1;
    public static final int TOUCH_UP = 2;
    public static final int TOUCH_CANCEL = 3;

    /* SDL keycodes used by the Android Speed Dreams GUI event bridge. */
    public static final int KEY_ESCAPE = 27;
    public static final int KEY_RETURN = 13;
    public static final int KEY_F2 = (1 << 30) | 59;

    static {
        System.loadLibrary("sdphoneinput");
    }

    private PhoneNative() { }

    public static native void resetInput();
    public static native void setSteering(float value);
    public static native void setThrottle(float value);
    public static native void setBrake(float value);
    public static native void setButton(int index, boolean pressed);
    public static native void setViewport(int width, int height);
    public static native void queueMenuTouch(int action, float normalizedX, float normalizedY);
    public static native void queueKey(int keycode, boolean down);
}
