#include "phone_engine_adapter.h"

#include "phone_input.h"
#include "vr_events.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <mutex>
#include <unordered_set>
#include <vector>

namespace {

constexpr std::size_t kEngineEventCapacity = 256;
/* This phone UI produces only a handful of keys at once. Bounding the set
   gives overflow recovery room to enqueue a release for every active input. */
constexpr std::size_t kMaxTrackedKeys = 128;

struct EventQueue {
    std::array<VrEvent, kEngineEventCapacity> items{};
    std::size_t head = 0;
    std::size_t tail = 0;
    std::size_t count = 0;
    int mouse_x = 0;
    int mouse_y = 0;
    int mouse_button_state = 0;
    std::unordered_set<int> held_keys;
    std::mutex mutex;
};

EventQueue& queue()
{
    static EventQueue q;
    return q;
}

std::atomic<bool> sMenuMousePressed{false};

void append_raw_locked(EventQueue& q, const VrEvent& event)
{
    if (q.count >= kEngineEventCapacity) {
        return;
    }
    q.items[q.tail] = event;
    q.tail = (q.tail + 1) % kEngineEventCapacity;
    ++q.count;
}

/* Discard stale events only after synthesizing release events for input that
   the engine could already have consumed. This avoids stuck keys/mouse buttons
   if the GUI loop stalls long enough to fill the bounded queue. */
void recover_from_overflow_locked(EventQueue& q)
{
    std::vector<int> keys(q.held_keys.begin(), q.held_keys.end());
    const int mouse_buttons = q.mouse_button_state;
    const int mouse_x = q.mouse_x;
    const int mouse_y = q.mouse_y;

    q.head = 0;
    q.tail = 0;
    q.count = 0;
    q.held_keys.clear();
    q.mouse_button_state = 0;

    for (const int key : keys) {
        append_raw_locked(q, VrEvent{VR_EV_KEY_UP, key, 0, 0, 0});
    }
    for (int button = 1; button <= 31; ++button) {
        const int mask = 1 << (button - 1);
        if ((mouse_buttons & mask) != 0) {
            append_raw_locked(q, VrEvent{VR_EV_MOUSE_BUTTON, button, 0, mouse_x, mouse_y});
        }
    }
}

void queue_event(int type, int a, int b, int c, int d)
{
    EventQueue& q = queue();
    std::lock_guard<std::mutex> lock(q.mutex);

    const bool new_key_down = type == VR_EV_KEY_DOWN && q.held_keys.find(a) == q.held_keys.end();
    if (q.count == kEngineEventCapacity ||
        (new_key_down && q.held_keys.size() >= kMaxTrackedKeys)) {
        recover_from_overflow_locked(q);
        sMenuMousePressed.store(false, std::memory_order_relaxed);
    }

    switch (type) {
    case VR_EV_KEY_DOWN:
        q.held_keys.insert(a);
        break;
    case VR_EV_KEY_UP:
        q.held_keys.erase(a);
        break;
    case VR_EV_MOUSE_BUTTON:
        if (a >= 1 && a <= 31) {
            const int mask = 1 << (a - 1);
            q.mouse_button_state = b != 0 ? (q.mouse_button_state | mask)
                                          : (q.mouse_button_state & ~mask);
        }
        q.mouse_x = c;
        q.mouse_y = d;
        break;
    case VR_EV_MOUSE_MOTION:
        q.mouse_x = a;
        q.mouse_y = b;
        break;
    default:
        break;
    }

    append_raw_locked(q, VrEvent{type, a, b, c, d});
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
        const bool was_pressed = sMenuMousePressed.load(std::memory_order_relaxed);
        VrQueueMouseMotion(touch.x, touch.y, was_pressed ? 1 : 0);

        if (touch.action == SD_PHONE_TOUCH_DOWN) {
            VrQueueMouseButton(1, 1, touch.x, touch.y);
            sMenuMousePressed.store(true, std::memory_order_relaxed);
        } else if (touch.action == SD_PHONE_TOUCH_UP ||
                   touch.action == SD_PHONE_TOUCH_CANCEL) {
            const bool was_down =
                sMenuMousePressed.exchange(false, std::memory_order_relaxed);
            if (was_down) {
                VrQueueMouseButton(1, 0, touch.x, touch.y);
            }
        }
    }
}

extern "C" void SDPhoneEngineResetEvents(void)
{
    EventQueue& q = queue();
    {
        std::lock_guard<std::mutex> lock(q.mutex);
        recover_from_overflow_locked(q);
    }
    sMenuMousePressed.store(false, std::memory_order_relaxed);
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
    if (button < 1 || button > 31) {
        return;
    }
    VrQueueEvent(VR_EV_MOUSE_BUTTON, button, down != 0 ? 1 : 0, x, y);
}

extern "C" void VrQueueMouseMotion(int x, int y, int state)
{
    VrQueueEvent(VR_EV_MOUSE_MOTION, x, y, state, 0);
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
