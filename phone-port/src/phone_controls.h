#ifndef SD_PHONE_CONTROLS_H
#define SD_PHONE_CONTROLS_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

namespace sdphone {

constexpr int kMaxTouches = 10;
constexpr std::size_t kMaxQueuedEvents = 256;

enum class TouchPhase : int { Down = 0, Up = 1, Move = 2, Cancel = 3 };
enum class ActionKey : int { Pause = 0, Camera = 1 };
enum class MenuEventType : int { Motion = 0, ButtonDown = 1, ButtonUp = 2 };

struct MenuEvent {
    MenuEventType type{};
    int x = 0;
    int y = 0;
    int button = 1;
};

struct PhoneFrameInput {
    std::array<float, 8> axes{};
    std::uint32_t buttons = 0;
    std::vector<MenuEvent> menuEvents;
    std::vector<ActionKey> actionKeys;
    bool showDrivingOverlay = false;
};

class PhoneControls {
public:
    void onTouch(TouchPhase phase, int pointerId, float x, float y);
    PhoneFrameInput update(int width, int height, bool racing);
    void reset();

private:
    struct Touch {
        int id = -1;
        float x = 0.0f;
        float y = 0.0f;
        bool down = false;
    };
    struct RawEvent {
        TouchPhase phase{};
        int pointerId = -1;
        float x = 0.0f;
        float y = 0.0f;
    };

    void enqueueEvent(TouchPhase phase, int pointerId, float x, float y);

    std::mutex mutex_;
    std::array<Touch, kMaxTouches> touches_{};
    std::vector<RawEvent> events_;
    bool lastModeKnown_ = false;
    bool lastModeRacing_ = false;
    bool ignoreUntilAllUp_ = false;
};

} // namespace sdphone
#endif
