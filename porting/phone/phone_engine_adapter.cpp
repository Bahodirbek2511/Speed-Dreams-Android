#include "phone_engine_adapter.h"

#include "phone_input.h"
#include "vr_events.h"

#include <array>
#include <cstddef>
#include <mutex>

namespace {

constexpr std::size_t kEngineEventCapacity = 256;

struct EventQueue {
    std::array<VrEvent, kEngineEventCapacity> items{};
    std::size_t head = 0;
    std::size_t tail = 0;
    std::size_t count = 0;
    int mouse_x = 0;
    int mouse_y = 0;
    int mouse_button_state = 0;
    std::mutex mutex;
};

EventQueue& queue()
{
    static EventQueue q;
    return q;
}

void push_event_locked(EventQueue& q, const VrEvent& event)
{
    if (q.count == kEngineEventCapacity) {
        q.head = (q.head + 1) % kEngineEventCapacity;
        --q.count;
    }
    q.items[q.tail] = event;
    q.tail = (q.tail + 1) % kEngineEventCapacity;
    ++q.count;
}

void queue_event(int type, int a, int b, int c, int d)
{
    EventQueue& q = queue();
    std::lock_guard<std::mutex> lock(q.mutex);
    push_event_locked(q, VrEvent{type, a, b, c, d});
}

bool& menu_mouse_pressed()
{
    static bool pressed = false;
    return pressed;
}

} // namespace

extern "C" int VrJoyRead(int joystick_index, int *buttons, float *axes)
{
    return SDPhoneInputRead(joystick_index, buttons, axes);
}

extern "C" void SDPhoneInputPumpEvents(void)
{
    SDPhoneTouchEvent touch{};
    while (SDPhoneInputPollMenuTouch(&touch)) {
        /* Keep the cursor in sync for both passive movement and click events. */
        VrQueueMouseMotion(touch.x, touch.y, menu_mouse_pressed() ? 1 : 0);

        if (touch.action == SD_PHONE_TOUCH_DOWN) {
            menu_mouse_pressed() = true;
            VrQueueMouseButton(1, 1, touch.x, touch.y);
        } else if (touch.action == SD_PHONE_TOUCH_UP) {
            if (menu_mouse_pressed()) {
                VrQueueMouseButton(1, 0, touch.x, touch.y);
            }
            menu_mouse_pressed() = false;
        } else if (touch.action == SD_PHONE_TOUCH_CANCEL) {
            if (menu_mouse_pressed()) {
                VrQueueMouseButton(1, 0, touch.x, touch.y);
            }
            menu_mouse_pressed() = false;
        }
    }
}

extern "C" void VrQueueEvent(int type, int a, int b, int c, int d)
{
    queue_event(type, a, b, c, d);
}

extern "C" void VrQueueKey(int keycode, int down, int modifiers, int unicode)
{
    VrQueueEvent(down ? VR_EV_KEY_DOWN : VR_EV_KEY_UP, keycode, modifiers, unicode, 0);
}

extern "C" void VrQueueMouseButton(int button, int down, int x, int y)
{
    EventQueue& q = queue();
    std::lock_guard<std::mutex> lock(q.mutex);
    q.mouse_x = x;
    q.mouse_y = y;
    q.mouse_button_state = down ? q.mouse_button_state | (1 << (button - 1))
                                : q.mouse_button_state & ~(1 << (button - 1));
    push_event_locked(q, VrEvent{VR_EV_MOUSE_BUTTON, button, down, x, y});
}

extern "C" void VrQueueMouseMotion(int x, int y, int state)
{
    EventQueue& q = queue();
    std::lock_guard<std::mutex> lock(q.mutex);
    q.mouse_x = x;
    q.mouse_y = y;
    push_event_locked(q, VrEvent{VR_EV_MOUSE_MOTION, x, y, state, 0});
}

extern "C" int VrPollEvent(VrEvent *event)
{
    if (event == nullptr) {
        return 0;
    }

    EventQueue& q = queue();
    std::lock_guard<std::mutex> lock(q.mutex);
    if (q.count == 0) {
        return 0;
    }

    *event = q.items[q.head];
    q.head = (q.head + 1) % kEngineEventCapacity;
    --q.count;
    return 1;
}

extern "C" void VrGetMousePos(int *x, int *y)
{
    EventQueue& q = queue();
    std::lock_guard<std::mutex> lock(q.mutex);
    if (x != nullptr) *x = q.mouse_x;
    if (y != nullptr) *y = q.mouse_y;
}

extern "C" void VrSetMousePos(int x, int y)
{
    EventQueue& q = queue();
    std::lock_guard<std::mutex> lock(q.mutex);
    q.mouse_x = x;
    q.mouse_y = y;
}
