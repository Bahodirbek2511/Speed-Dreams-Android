#include "../phone_input.h"

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
