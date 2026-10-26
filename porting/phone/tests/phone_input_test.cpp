#include "../phone_input.h"
#include "../phone_engine_adapter.h"
#include "../vr_events.h"

#include <cassert>
#include <cmath>
#include <limits>

int main()
{
    SDPhoneInputReset();

    int buttons = -1;
    float axes[SD_PHONE_AXIS_COUNT] = {};
    assert(SDPhoneInputRead(0, &buttons, axes) == 0);
    assert(buttons == 0);
    for (float axis : axes) {
        assert(axis == 0.0f);
    }
    assert(SDPhoneInputRead(1, &buttons, axes) == -1);
    assert(SDPhoneInputRead(0, nullptr, axes) == -1);
    assert(SDPhoneInputRead(0, &buttons, nullptr) == -1);

    SDPhoneInputSetSteering(0.04f);
    SDPhoneInputSetThrottle(0.6f);
    SDPhoneInputSetBrake(0.25f);
    assert(SDPhoneInputRead(0, &buttons, axes) == 0);
    assert(axes[0] == 0.0f); // Steering deadzone.
    assert(std::fabs(axes[1] - 0.6f) < 0.0001f);
    assert(std::fabs(axes[4] - 0.25f) < 0.0001f);

    SDPhoneInputSetSteering(0.54f);
    SDPhoneInputSetThrottle(2.0f);
    SDPhoneInputSetBrake(-1.0f);
    assert(SDPhoneInputRead(0, &buttons, axes) == 0);
    assert(std::fabs(axes[0] - 0.5f) < 0.0001f);
    assert(axes[1] == 1.0f);
    assert(axes[4] == 0.0f);

    SDPhoneInputSetSteering(std::numeric_limits<float>::quiet_NaN());
    SDPhoneInputSetThrottle(std::numeric_limits<float>::infinity());
    assert(SDPhoneInputRead(0, &buttons, axes) == 0);
    assert(axes[0] == 0.0f);
    assert(axes[1] == 0.0f);

    SDPhoneInputSetButton(SD_PHONE_BUTTON_SHIFT_UP, 1);
    SDPhoneInputSetButton(SD_PHONE_BUTTON_SHIFT_DOWN, 1);
    SDPhoneInputSetButton(SD_PHONE_BUTTON_REVERSE, 1);
    SDPhoneInputSetButton(SD_PHONE_BUTTON_ASR, 1);
    SDPhoneInputSetButton(-1, 1);  // Invalid indices are ignored.
    SDPhoneInputSetButton(31, 1);
    assert(SDPhoneInputRead(0, &buttons, axes) == 0);
    const int expected = (1 << SD_PHONE_BUTTON_SHIFT_UP) |
                         (1 << SD_PHONE_BUTTON_SHIFT_DOWN) |
                         (1 << SD_PHONE_BUTTON_REVERSE) |
                         (1 << SD_PHONE_BUTTON_ASR);
    assert(buttons == expected);

    SDPhoneInputSetButton(SD_PHONE_BUTTON_SHIFT_UP, 0);
    assert(SDPhoneInputRead(0, &buttons, axes) == 0);
    assert((buttons & (1 << SD_PHONE_BUTTON_SHIFT_UP)) == 0);

    assert(VrJoyRead(0, &buttons, axes) == 0);
    assert(std::fabs(axes[1]) < 0.0001f); // Non-finite input is normalized to zero.
    assert(VrJoyRead(7, &buttons, axes) == -1);

    SDPhoneInputSetViewport(100, 80);
    SDPhoneInputQueueMenuTouch(SD_PHONE_TOUCH_DOWN, 0.5f, 0.25f);
    SDPhoneInputQueueMenuTouch(SD_PHONE_TOUCH_UP, 1.0f, 1.0f);
    SDPhoneTouchEvent event{};
    assert(SDPhoneInputPollMenuTouch(&event) == 1);
    assert(event.action == SD_PHONE_TOUCH_DOWN);
    assert(event.x == 50);
    assert(event.y == 20);
    assert(event.width == 100);
    assert(event.height == 80);
    assert(SDPhoneInputPollMenuTouch(&event) == 1);
    assert(event.action == SD_PHONE_TOUCH_UP);
    assert(event.x == 99);
    assert(event.y == 79);
    assert(SDPhoneInputPollMenuTouch(&event) == 0);

    /* Verify the adapter translates gestures into the engine's legacy GUI events. */
    SDPhoneInputQueueMenuTouch(SD_PHONE_TOUCH_MOVE, 0.25f, 0.75f);
    SDPhoneInputQueueMenuTouch(SD_PHONE_TOUCH_DOWN, 0.5f, 0.25f);
    SDPhoneInputQueueMenuTouch(SD_PHONE_TOUCH_UP, 0.75f, 0.75f);
    SDPhoneInputPumpEvents();

    VrEvent engine_event{};
    assert(VrPollEvent(&engine_event) == 1);
    assert(engine_event.type == VR_EV_MOUSE_MOTION);
    assert(engine_event.a == 25 && engine_event.b == 59);

    assert(VrPollEvent(&engine_event) == 1);
    assert(engine_event.type == VR_EV_MOUSE_MOTION);
    assert(engine_event.a == 50 && engine_event.b == 20);
    assert(VrPollEvent(&engine_event) == 1);
    assert(engine_event.type == VR_EV_MOUSE_BUTTON);
    assert(engine_event.a == 1 && engine_event.b == 1);
    assert(engine_event.c == 50 && engine_event.d == 20);

    assert(VrPollEvent(&engine_event) == 1);
    assert(engine_event.type == VR_EV_MOUSE_MOTION);
    assert(engine_event.a == 74 && engine_event.b == 59);
    assert(engine_event.c == 1); // Mouse state remains pressed while dragging.
    assert(VrPollEvent(&engine_event) == 1);
    assert(engine_event.type == VR_EV_MOUSE_BUTTON);
    assert(engine_event.a == 1 && engine_event.b == 0);

    int mouse_x = -1;
    int mouse_y = -1;
    VrGetMousePos(&mouse_x, &mouse_y);
    assert(mouse_x == 74 && mouse_y == 59);

    SDPhoneInputSetViewport(0, 0);
    SDPhoneInputQueueMenuTouch(SD_PHONE_TOUCH_DOWN, 0.5f, 0.5f);
    assert(SDPhoneInputPollMenuTouch(&event) == 0);

    SDPhoneInputReset();
    assert(SDPhoneInputRead(0, &buttons, axes) == 0);
    assert(buttons == 0);
    for (float axis : axes) {
        assert(axis == 0.0f);
    }
    return 0;
}
