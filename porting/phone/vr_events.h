#ifndef SD_PHONE_VR_EVENTS_H
#define SD_PHONE_VR_EVENTS_H

/*
 * Flat-screen replacement for the event interface in SpeedDreams-VR's
 * src/vr/vr_events.h. Keep the event values and C ABI compatible with the
 * Android GUI event-loop hooks, without pulling OpenXR headers into phone code.
 */

#ifdef __cplusplus
extern "C" {
#endif

enum VrEventType {
    VR_EV_KEY_DOWN = 0,
    VR_EV_KEY_UP,
    VR_EV_MOUSE_BUTTON,
    VR_EV_MOUSE_MOTION,
    VR_EV_QUIT
};

typedef struct VrEvent {
    int type;
    int a;
    int b;
    int c;
    int d;
} VrEvent;

void VrQueueEvent(int type, int a, int b, int c, int d);
void VrQueueKey(int keycode, int down, int modifiers, int unicode);
void VrQueueMouseButton(int button, int down, int x, int y);
void VrQueueMouseMotion(int x, int y, int state);
int VrPollEvent(VrEvent *event);
void VrGetMousePos(int *x, int *y);
void VrSetMousePos(int x, int y);

#ifdef __cplusplus
}
#endif

#endif
