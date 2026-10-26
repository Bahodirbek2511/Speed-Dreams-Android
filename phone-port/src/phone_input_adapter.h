#ifndef SD_PHONE_INPUT_ADAPTER_H
#define SD_PHONE_INPUT_ADAPTER_H
#ifdef __cplusplus
extern "C" {
#endif
void SDPhoneTouchEvent(int phase, int pointerId, float x, float y);
void SDPhoneUpdateInput(int racing);
void SDPhoneDrawControls(int width, int height);
int SDPhoneEnsureSurface(void);
void SDPhoneGetSurfaceSize(int* width, int* height);
int SDPhonePresent(void);
#ifdef __cplusplus
}
#endif
#endif
