#include "phone_controls.h"

#include <algorithm>
#include <cmath>

namespace sdphone {
namespace {
float clampUnit(float value)
{
    return std::max(-1.0f, std::min(1.0f, value));
}

bool insideTopButton(float x, float y, int w, int h)
{
    return w > 0 && h > 0 && y < static_cast<float>(h) * 0.22f &&
           x >= 0.0f && x < static_cast<float>(w);
}

std::uint32_t buttonForPoint(float x, float y, int w, int h)
{
    if (!insideTopButton(x, y, w, h)) return 0;
    const float nx = x / static_cast<float>(w);
    if (nx < 0.12f || nx >= 0.86f) return 0;
    if (nx < 0.24f) return 1u << 4;
    if (nx < 0.36f) return 1u << 1;
    if (nx < 0.48f) return 1u << 0;
    if (nx < 0.60f) return 0;
    if (nx < 0.72f) return 1u << 2;
    return 1u << 5;
}

bool actionForPoint(float x, float y, int w, int h, ActionKey& key)
{
    if (!insideTopButton(x, y, w, h) || w <= 0) return false;
    const float nx = x / static_cast<float>(w);
    if (nx < 0.12f) {
        key = ActionKey::Pause;
        return true;
    }
    if (nx >= 0.48f && nx < 0.60f) {
        key = ActionKey::Camera;
        return true;
    }
    return false;
}
} // namespace

void PhoneControls::enqueueEvent(TouchPhase phase, int pointerId, float x, float y)
{
    if (events_.size() >= kMaxQueuedEvents) {
        events_.clear();
        for (Touch& touch : touches_) touch = Touch{};
        ignoreUntilAllUp_ = true;
        events_.push_back({TouchPhase::Cancel, pointerId, x, y});
        return;
    }
    events_.push_back({phase, pointerId, x, y});
}

void PhoneControls::onTouch(TouchPhase phase, int pointerId, float x, float y)
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (phase == TouchPhase::Cancel) {
        enqueueEvent(phase, pointerId, x, y);
        for (Touch& touch : touches_) touch = Touch{};
        return;
    }

    int slot = -1;
    for (int i = 0; i < kMaxTouches; ++i) {
        if (touches_[i].id == pointerId) {
            slot = i;
            break;
        }
    }
    if (slot < 0 && phase == TouchPhase::Down) {
        for (int i = 0; i < kMaxTouches; ++i) {
            if (touches_[i].id < 0) {
                slot = i;
                break;
            }
        }
    }
    if (slot < 0) return;

    Touch& touch = touches_[slot];
    if (phase == TouchPhase::Down) {
        touch.id = pointerId;
        touch.down = true;
    } else if (phase == TouchPhase::Up) {
        touch.down = false;
    } else if (phase == TouchPhase::Move && !touch.down) {
        return;
    }
    touch.x = x;
    touch.y = y;
    enqueueEvent(phase, pointerId, x, y);
    // Android pointer IDs are reused over time; free slots so repeated taps
    // cannot exhaust this fixed-size table.
    if (phase == TouchPhase::Up) touch = Touch{};
}

PhoneFrameInput PhoneControls::update(int width, int height, bool racing)
{
    PhoneFrameInput out;
    if (width <= 0 || height <= 0) return out;

    std::array<Touch, kMaxTouches> touches;
    std::vector<RawEvent> events;
    {
        std::lock_guard<std::mutex> guard(mutex_);
        touches = touches_;
        events.swap(events_);

        if (lastModeKnown_ && racing != lastModeRacing_) ignoreUntilAllUp_ = true;
        lastModeKnown_ = true;
        lastModeRacing_ = racing;

        if (ignoreUntilAllUp_) {
            bool anyDown = false;
            for (const Touch& touch : touches_) anyDown = anyDown || touch.down;
            if (!anyDown) ignoreUntilAllUp_ = false;
        }
        if (ignoreUntilAllUp_) {
            out.showDrivingOverlay = racing;
            return out;
        }
    }

    out.showDrivingOverlay = racing;
    if (!racing) {
        for (const RawEvent& event : events) {
            const int x = std::max(0, std::min(width - 1, static_cast<int>(std::lround(event.x))));
            const int y = std::max(0, std::min(height - 1, static_cast<int>(std::lround(event.y))));
            if (event.phase == TouchPhase::Down) {
                out.menuEvents.push_back({MenuEventType::Motion, x, y, 1});
                out.menuEvents.push_back({MenuEventType::ButtonDown, x, y, 1});
            } else if (event.phase == TouchPhase::Move) {
                out.menuEvents.push_back({MenuEventType::Motion, x, y, 1});
            } else if (event.phase == TouchPhase::Up || event.phase == TouchPhase::Cancel) {
                out.menuEvents.push_back({MenuEventType::Motion, x, y, 0});
                out.menuEvents.push_back({MenuEventType::ButtonUp, x, y, 1});
            }
        }
        return out;
    }

    float steering = 0.0f;
    bool hasSteering = false;
    bool throttle = false;
    bool brake = false;
    for (const Touch& touch : touches) {
        if (touch.id < 0 || !touch.down) continue;
        const float nx = touch.x / static_cast<float>(width);
        const float ny = touch.y / static_cast<float>(height);
        if (ny >= 0.42f && nx <= 0.42f) {
            steering = clampUnit((nx - 0.22f) / 0.16f);
            hasSteering = true;
            continue;
        }
        if (ny >= 0.22f && nx >= 0.65f) {
            if (ny >= 0.58f) throttle = true;
            else brake = true;
            continue;
        }
        out.buttons |= buttonForPoint(touch.x, touch.y, width, height);
    }
    out.axes[0] = hasSteering ? steering : 0.0f;
    out.axes[1] = throttle ? 1.0f : 0.0f;
    out.axes[4] = brake ? 1.0f : 0.0f;

    for (const RawEvent& event : events) {
        if (event.phase != TouchPhase::Down) continue;
        ActionKey key;
        if (actionForPoint(event.x, event.y, width, height, key)) out.actionKeys.push_back(key);
    }
    return out;
}

void PhoneControls::reset()
{
    std::lock_guard<std::mutex> guard(mutex_);
    for (Touch& touch : touches_) touch = Touch{};
    events_.clear();
    lastModeKnown_ = false;
    lastModeRacing_ = false;
    ignoreUntilAllUp_ = false;
}
} // namespace sdphone
