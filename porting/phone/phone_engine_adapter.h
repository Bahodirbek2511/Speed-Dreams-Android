#ifndef SD_PHONE_ENGINE_ADAPTER_H
#define SD_PHONE_ENGINE_ADAPTER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Speed Dreams' Android joystick hook expects this legacy virtual-joystick API. */
int VrJoyRead(int joystick_index, int *buttons, float *axes);

/* Drain queued Android touch input into the GUI event queue on the engine thread. */
void SDPhoneInputPumpEvents(void);

/* Clear queued GUI events and release any latched virtual mouse press. */
void SDPhoneEngineResetEvents(void);

#ifdef __cplusplus
}
#endif

#endif
