package com.speeddreamsphone;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.util.AttributeSet;
import android.util.SparseIntArray;
import android.view.MotionEvent;
import android.view.View;

/**
 * Low-overhead multi-touch overlay for a landscape phone screen.
 * The game view should be placed behind this view in a FrameLayout.
 */
public final class PhoneControlsView extends View {
    private static final int ZONE_MENU = 0;
    private static final int ZONE_STEER = 1;
    private static final int ZONE_THROTTLE = 2;
    private static final int ZONE_BRAKE = 3;
    private static final int ZONE_BUTTON_BASE = 100;

    private static final int BUTTON_GEAR_UP = 0;
    private static final int BUTTON_GEAR_DOWN = 1;
    private static final int BUTTON_REVERSE = 4;
    private static final int BUTTON_CAMERA = 6;
    private static final int BUTTON_PAUSE = 7;

    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final SparseIntArray pointerZones = new SparseIntArray();
    private int menuPointerId = -1;
    private int activeWidth;
    private int activeHeight;

    public PhoneControlsView(Context context) {
        super(context);
        init();
    }

    public PhoneControlsView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public PhoneControlsView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    private void init() {
        setFocusable(false);
        setClickable(true);
        paint.setTypeface(Typeface.create(Typeface.SANS_SERIF, Typeface.BOLD));
        paint.setTextAlign(Paint.Align.CENTER);
        PhoneNative.resetInput();
    }

    @Override
    protected void onSizeChanged(int width, int height, int oldWidth, int oldHeight) {
        super.onSizeChanged(width, height, oldWidth, oldHeight);
        activeWidth = width;
        activeHeight = height;
        PhoneNative.setViewport(width, height);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (activeWidth <= 0 || activeHeight <= 0) return;

        float w = activeWidth;
        float h = activeHeight;
        float radius = Math.min(w * 0.095f, h * 0.19f);

        drawCircle(canvas, w * 0.18f, h * 0.72f, radius * 1.35f, "RUL");
        drawCircle(canvas, w * 0.68f, h * 0.72f, radius, "TORMOZ");
        drawCircle(canvas, w * 0.86f, h * 0.72f, radius, "GAZ");

        float[] buttonX = {w * 0.48f, w * 0.59f, w * 0.70f, w * 0.81f, w * 0.92f};
        String[] labels = {"G-", "G+", "R", "CAM", "II"};
        for (int i = 0; i < buttonX.length; i++) {
            drawCircle(canvas, buttonX[i], h * 0.19f, radius * 0.47f, labels[i]);
        }
    }

    private void drawCircle(Canvas canvas, float x, float y, float radius, String label) {
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(Color.argb(84, 15, 20, 27));
        canvas.drawCircle(x, y, radius, paint);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(Math.max(2.0f, radius * 0.045f));
        paint.setColor(Color.argb(180, 235, 242, 250));
        canvas.drawCircle(x, y, radius, paint);
        paint.setStyle(Paint.Style.FILL);
        paint.setTextSize(Math.max(11.0f, radius * 0.28f));
        paint.setColor(Color.WHITE);
        canvas.drawText(label, x, y + paint.getTextSize() * 0.35f, paint);
    }

    private int zoneAt(float x, float y) {
        float nx = x / Math.max(1, activeWidth);
        float ny = y / Math.max(1, activeHeight);

        if (ny < 0.34f && nx >= 0.42f) {
            if (nx < 0.535f) return ZONE_BUTTON_BASE + BUTTON_GEAR_DOWN;
            if (nx < 0.645f) return ZONE_BUTTON_BASE + BUTTON_GEAR_UP;
            if (nx < 0.755f) return ZONE_BUTTON_BASE + BUTTON_REVERSE;
            if (nx < 0.865f) return ZONE_BUTTON_BASE + BUTTON_CAMERA;
            return ZONE_BUTTON_BASE + BUTTON_PAUSE;
        }
        if (ny > 0.49f && nx < 0.40f) return ZONE_STEER;
        if (ny > 0.49f && nx >= 0.78f) return ZONE_THROTTLE;
        if (ny > 0.49f && nx >= 0.56f && nx < 0.78f) return ZONE_BRAKE;
        return ZONE_MENU;
    }

    private static float clamp01(float value) {
        return Math.max(0.0f, Math.min(1.0f, value));
    }

    private void beginPointer(MotionEvent event, int pointerIndex) {
        int id = event.getPointerId(pointerIndex);
        int zone = zoneAt(event.getX(pointerIndex), event.getY(pointerIndex));
        pointerZones.put(id, zone);
        applyPointer(event, pointerIndex, zone, true);

        if (zone == ZONE_MENU && menuPointerId == -1) {
            menuPointerId = id;
            queueMenu(PhoneNative.TOUCH_DOWN, event.getX(pointerIndex), event.getY(pointerIndex));
        }
    }

    private void movePointers(MotionEvent event) {
        for (int i = 0; i < event.getPointerCount(); i++) {
            int id = event.getPointerId(i);
            int zone = pointerZones.get(id, ZONE_MENU);
            applyPointer(event, i, zone, true);
            if (zone == ZONE_MENU && id == menuPointerId) {
                queueMenu(PhoneNative.TOUCH_MOVE, event.getX(i), event.getY(i));
            }
        }
    }

    private void endPointer(MotionEvent event, int pointerIndex, boolean cancelled) {
        int id = event.getPointerId(pointerIndex);
        int zone = pointerZones.get(id, ZONE_MENU);
        applyPointer(event, pointerIndex, zone, false);
        if (zone == ZONE_MENU && id == menuPointerId) {
            queueMenu(cancelled ? PhoneNative.TOUCH_CANCEL : PhoneNative.TOUCH_UP,
                    event.getX(pointerIndex), event.getY(pointerIndex));
            menuPointerId = -1;
        }
        pointerZones.delete(id);
    }

    private void applyPointer(MotionEvent event, int pointerIndex, int zone, boolean pressed) {
        float x = event.getX(pointerIndex);
        float pressure = clamp01(event.getPressure(pointerIndex));
        if (!pressed) pressure = 0.0f;

        switch (zone) {
            case ZONE_STEER:
                float centre = activeWidth * 0.18f;
                float travel = Math.max(1.0f, activeWidth * 0.15f);
                float steering = Math.max(-1.0f, Math.min(1.0f, (x - centre) / travel));
                PhoneNative.setSteering(pressed ? steering : 0.0f);
                break;
            case ZONE_THROTTLE:
                PhoneNative.setThrottle(pressure);
                break;
            case ZONE_BRAKE:
                PhoneNative.setBrake(pressure);
                break;
            default:
                if (zone >= ZONE_BUTTON_BASE) {
                    PhoneNative.setButton(zone - ZONE_BUTTON_BASE, pressed);
                }
                break;
        }
    }

    private void queueMenu(int action, float x, float y) {
        if (activeWidth <= 0 || activeHeight <= 0) return;
        PhoneNative.queueMenuTouch(action,
                clamp01(x / activeWidth), clamp01(y / activeHeight));
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (activeWidth <= 0 || activeHeight <= 0) return false;
        int action = event.getActionMasked();
        int index = event.getActionIndex();

        switch (action) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN:
                beginPointer(event, index);
                return true;
            case MotionEvent.ACTION_MOVE:
                movePointers(event);
                return true;
            case MotionEvent.ACTION_POINTER_UP:
                endPointer(event, index, false);
                return true;
            case MotionEvent.ACTION_UP:
                endPointer(event, index, false);
                return true;
            case MotionEvent.ACTION_CANCEL:
                for (int i = event.getPointerCount() - 1; i >= 0; i--) {
                    endPointer(event, i, true);
                }
                PhoneNative.setSteering(0.0f);
                PhoneNative.setThrottle(0.0f);
                PhoneNative.setBrake(0.0f);
                return true;
            default:
                return true;
        }
    }
}
