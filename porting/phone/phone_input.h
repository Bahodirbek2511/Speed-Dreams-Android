#ifndef SD_PHONE_INPUT_H
#define SD_PHONE_INPUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Platform-neutral input contract for the flat-screen Android port.
 *
 * The layout deliberately matches the virtual joystick currently consumed by
 * the Speed Dreams Android engine port: axis 0 = steering, axis 1 = throttle,
 * axis 4 = brake/clutch. Button bit N maps to preferences.xml BTN(N+1)-0.
 */

enum {
    SD_PHONE_AXIS_COUNT = 8,
    SD_PHONE_BUTTON_SHIFT_UP = 0,
    SD_PHONE_BUTTON_SHIFT_DOWN = 1,
    SD_PHONE_BUTTON_ABS = 2,
    SD_PHONE_BUTTON_REVERSE = 4,
    SD_PHONE_BUTTON_ASR = 5,
    SD_PHONE_BUTTON_CAMERA = 6,
    SD_PHONE_BUTTON_PAUSE = 7,
    SD_PHONE_BUTTON_CONFIRM = 8,
    SD_PHONE_BUTTON_CANCEL = 9
};

enum {
    SD_PHONE_TOUCH_MOVE = 0,
    SD_PHONE_TOUCH_DOWN = 1,
    SD_PHONE_TOUCH_UP = 2,
    SD_PHONE_TOUCH_CANCEL = 3
};

typedef struct SDPhoneTouchEvent {
    int action;
    int x;
    int y;
    int width;
    int height;
} SDPhoneTouchEvent;

/* Safe to call from the Android UI thread while the game reads on its own thread. */
void SDPhoneInputReset(void);
void SDPhoneInputSetSteering(float value);
void SDPhoneInputSetThrottle(float value);
void SDPhoneInputSetBrake(float value);
void SDPhoneInputSetButton(int button_index, int pressed);
int SDPhoneInputRead(int joystick_index, int *buttons, float *axes);

/* Menu input uses normalized [0,1] coordinates and is converted to screen pixels. */
void SDPhoneInputSetViewport(int width, int height);
void SDPhoneInputQueueMenuTouch(int action, float normalized_x, float normalized_y);
int SDPhoneInputPollMenuTouch(SDPhoneTouchEvent *event);

#ifdef __cplusplus
}
#endif

#endif
