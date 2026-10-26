#include "phone_input.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <mutex>

namespace {

constexpr std::size_t kTouchQueueCapacity = 128;
constexpr float kSteeringDeadzone = 0.08f;

struct InputState {
    std::array<float, SD_PHONE_AXIS_COUNT> axes{};
    std::uint32_t buttons = 0;
    int viewport_width = 0;
    int viewport_height = 0;
    std::array<SDPhoneTouchEvent, kTouchQueueCapacity> touch_queue{};
    std::size_t touch_head = 0;
    std::size_t touch_tail = 0;
    std::size_t touch_count = 0;
    std::mutex mutex;
};

InputState& state()
{
    static InputState input;
    return input;
}

float finite_clamp(float value, float low, float high)
{
    if (!std::isfinite(value)) {
        return 0.0f;
    }
    return std::max(low, std::min(value, high));
}

float normalize_steering(float value)
{
    value = finite_clamp(value, -1.0f, 1.0f);
    const float magnitude = std::fabs(value);
    if (magnitude <= kSteeringDeadzone) {
        return 0.0f;
    }
    const float scaled = (magnitude - kSteeringDeadzone) /
                         (1.0f - kSteeringDeadzone);
    return std::copysign(scaled, value);
}

void queue_touch_locked(InputState& input, const SDPhoneTouchEvent& event)
{
    /* On overflow, discard the oldest item rather than corrupting the ring. */
    if (input.touch_count == kTouchQueueCapacity) {
        input.touch_head = (input.touch_head + 1) % kTouchQueueCapacity;
        --input.touch_count;
    }
    input.touch_queue[input.touch_tail] = event;
    input.touch_tail = (input.touch_tail + 1) % kTouchQueueCapacity;
    ++input.touch_count;
}

} // namespace

extern "C" void SDPhoneInputReset(void)
{
    InputState& input = state();
    std::lock_guard<std::mutex> lock(input.mutex);
    input.axes.fill(0.0f);
    input.buttons = 0;
    input.touch_head = 0;
    input.touch_tail = 0;
    input.touch_count = 0;
}

extern "C" void SDPhoneInputSetSteering(float value)
{
    InputState& input = state();
    std::lock_guard<std::mutex> lock(input.mutex);
    input.axes[0] = normalize_steering(value);
}

extern "C" void SDPhoneInputSetThrottle(float value)
{
    InputState& input = state();
    std::lock_guard<std::mutex> lock(input.mutex);
    input.axes[1] = finite_clamp(value, 0.0f, 1.0f);
}

extern "C" void SDPhoneInputSetBrake(float value)
{
    InputState& input = state();
    std::lock_guard<std::mutex> lock(input.mutex);
    input.axes[4] = finite_clamp(value, 0.0f, 1.0f);
}

extern "C" void SDPhoneInputSetButton(int button_index, int pressed)
{
    if (button_index < 0 || button_index >= 31) {
        return;
    }

    InputState& input = state();
    std::lock_guard<std::mutex> lock(input.mutex);
    const std::uint32_t mask = std::uint32_t{1} << button_index;
    if (pressed != 0) {
        input.buttons |= mask;
    } else {
        input.buttons &= ~mask;
    }
}

extern "C" int SDPhoneInputRead(int joystick_index, int *buttons, float *axes)
{
    if (joystick_index != 0 || buttons == nullptr || axes == nullptr) {
        return -1;
    }

    InputState& input = state();
    std::lock_guard<std::mutex> lock(input.mutex);
    *buttons = static_cast<int>(input.buttons);
    std::copy(input.axes.begin(), input.axes.end(), axes);
    return 0;
}

extern "C" void SDPhoneInputSetViewport(int width, int height)
{
    InputState& input = state();
    std::lock_guard<std::mutex> lock(input.mutex);
    input.viewport_width = std::max(0, width);
    input.viewport_height = std::max(0, height);
}

extern "C" void SDPhoneInputQueueMenuTouch(int action, float normalized_x,
                                             float normalized_y)
{
    if (action < SD_PHONE_TOUCH_MOVE || action > SD_PHONE_TOUCH_CANCEL) {
        return;
    }

    InputState& input = state();
    std::lock_guard<std::mutex> lock(input.mutex);
    if (input.viewport_width <= 0 || input.viewport_height <= 0) {
        return;
    }

    const float x = finite_clamp(normalized_x, 0.0f, 1.0f);
    const float y = finite_clamp(normalized_y, 0.0f, 1.0f);
    SDPhoneTouchEvent event{};
    event.action = action;
    event.x = static_cast<int>(std::lround(x * (input.viewport_width - 1)));
    event.y = static_cast<int>(std::lround(y * (input.viewport_height - 1)));
    event.width = input.viewport_width;
    event.height = input.viewport_height;
    queue_touch_locked(input, event);
}

extern "C" int SDPhoneInputPollMenuTouch(SDPhoneTouchEvent *event)
{
    if (event == nullptr) {
        return 0;
    }

    InputState& input = state();
    std::lock_guard<std::mutex> lock(input.mutex);
    if (input.touch_count == 0) {
        return 0;
    }

    *event = input.touch_queue[input.touch_head];
    input.touch_head = (input.touch_head + 1) % kTouchQueueCapacity;
    --input.touch_count;
    return 1;
}
