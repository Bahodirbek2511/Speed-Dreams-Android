#include "phone_controls.h"
#include <cassert>
#include <iostream>
using sdphone::ActionKey;
using sdphone::MenuEventType;
using sdphone::PhoneControls;
using sdphone::TouchPhase;
int main()
{
    PhoneControls input;
    input.onTouch(TouchPhase::Down, 1, 120.0f, 160.0f);
    auto frame = input.update(800, 400, false);
    assert(frame.menuEvents.size() == 2);
    assert(frame.menuEvents[0].type == MenuEventType::Motion);
    assert(frame.menuEvents[1].type == MenuEventType::ButtonDown);
    input.onTouch(TouchPhase::Up, 1, 120.0f, 160.0f);
    frame = input.update(800, 400, false);
    assert(frame.menuEvents.size() == 2);
    assert(frame.menuEvents[1].type == MenuEventType::ButtonUp);

    input.reset();
    input.onTouch(TouchPhase::Down, 7, 300.0f, 300.0f);
    input.update(800, 400, false);
    frame = input.update(800, 400, true);
    assert(frame.showDrivingOverlay);
    assert(frame.axes[1] == 0.0f && frame.axes[4] == 0.0f);
    input.onTouch(TouchPhase::Up, 7, 300.0f, 300.0f);
    frame = input.update(800, 400, true);
    assert(frame.axes[1] == 0.0f);

    input.onTouch(TouchPhase::Down, 8, 700.0f, 320.0f);
    input.onTouch(TouchPhase::Down, 9, 700.0f, 100.0f);
    frame = input.update(800, 400, true);
    assert(frame.axes[1] == 1.0f && frame.axes[4] == 1.0f);
    assert(frame.axes[0] == 0.0f);

    input.onTouch(TouchPhase::Down, 10, 20.0f, 300.0f);
    frame = input.update(800, 400, true);
    assert(frame.axes[0] < -0.9f);
    input.onTouch(TouchPhase::Move, 10, 320.0f, 300.0f);
    frame = input.update(800, 400, true);
    assert(frame.axes[0] > 0.9f);
    input.onTouch(TouchPhase::Up, 10, 320.0f, 300.0f);
    frame = input.update(800, 400, true);
    assert(frame.axes[0] == 0.0f);

    input.onTouch(TouchPhase::Down, 11, 25.0f, 20.0f);
    frame = input.update(800, 400, true);
    assert(frame.actionKeys.size() == 1 && frame.actionKeys[0] == ActionKey::Pause);
    assert(input.update(800, 400, true).actionKeys.empty());
    input.onTouch(TouchPhase::Up, 11, 25.0f, 20.0f);
    input.update(800, 400, true);
    input.onTouch(TouchPhase::Down, 12, 440.0f, 20.0f);
    frame = input.update(800, 400, true);
    assert(frame.actionKeys.size() == 1 && frame.actionKeys[0] == ActionKey::Camera);

    input.onTouch(TouchPhase::Up, 12, 440.0f, 20.0f);
    input.update(800, 400, true);
    input.onTouch(TouchPhase::Down, 14, 350.0f, 20.0f);
    frame = input.update(800, 400, true);
    assert((frame.buttons & (1u << 0)) != 0);
    input.onTouch(TouchPhase::Up, 14, 350.0f, 20.0f);
    input.update(800, 400, true);
    input.onTouch(TouchPhase::Down, 15, 160.0f, 20.0f);
    frame = input.update(800, 400, true);
    assert((frame.buttons & (1u << 4)) != 0);
    input.onTouch(TouchPhase::Up, 15, 160.0f, 20.0f);
    input.update(800, 400, true);

    input.onTouch(TouchPhase::Down, 13, 700.0f, 320.0f);
    input.update(800, 400, true);
    input.onTouch(TouchPhase::Cancel, -1, 0.0f, 0.0f);
    frame = input.update(800, 400, true);
    assert(frame.axes[1] == 0.0f && frame.axes[4] == 0.0f && frame.axes[0] == 0.0f);

    input.reset();
    for (int i = 0; i < 30; ++i) {
        const int id = 100 + i;
        input.onTouch(TouchPhase::Down, id, 250.0f, 180.0f);
        frame = input.update(800, 400, false);
        assert(frame.menuEvents.size() == 2);
        input.onTouch(TouchPhase::Up, id, 250.0f, 180.0f);
        frame = input.update(800, 400, false);
        assert(frame.menuEvents.size() == 2);
    }
    std::cout << "phone_controls_test: PASS\n";
    return 0;
}