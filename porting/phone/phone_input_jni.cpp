#include <jni.h>

#include "phone_engine_adapter.h"
#include "phone_input.h"
#include "vr_events.h"

extern "C" JNIEXPORT void JNICALL
Java_com_speeddreamsphone_PhoneNative_resetInput(JNIEnv *, jclass)
{
    SDPhoneInputReset();
    SDPhoneEngineResetEvents();
}

extern "C" JNIEXPORT void JNICALL
Java_com_speeddreamsphone_PhoneNative_setSteering(JNIEnv *, jclass, jfloat value)
{
    SDPhoneInputSetSteering(value);
}

extern "C" JNIEXPORT void JNICALL
Java_com_speeddreamsphone_PhoneNative_setThrottle(JNIEnv *, jclass, jfloat value)
{
    SDPhoneInputSetThrottle(value);
}

extern "C" JNIEXPORT void JNICALL
Java_com_speeddreamsphone_PhoneNative_setBrake(JNIEnv *, jclass, jfloat value)
{
    SDPhoneInputSetBrake(value);
}

extern "C" JNIEXPORT void JNICALL
Java_com_speeddreamsphone_PhoneNative_setButton(JNIEnv *, jclass, jint index, jboolean pressed)
{
    SDPhoneInputSetButton(index, pressed == JNI_TRUE ? 1 : 0);
}

extern "C" JNIEXPORT void JNICALL
Java_com_speeddreamsphone_PhoneNative_setViewport(JNIEnv *, jclass, jint width, jint height)
{
    SDPhoneInputSetViewport(width, height);
}

extern "C" JNIEXPORT void JNICALL
Java_com_speeddreamsphone_PhoneNative_queueMenuTouch(JNIEnv *, jclass, jint action,
                                                       jfloat normalized_x, jfloat normalized_y)
{
    SDPhoneInputQueueMenuTouch(action, normalized_x, normalized_y);
}

extern "C" JNIEXPORT void JNICALL
Java_com_speeddreamsphone_PhoneNative_queueKey(JNIEnv *, jclass, jint keycode,
                                                 jboolean down)
{
    VrQueueKey(keycode, down == JNI_TRUE ? 1 : 0, 0, 0);
}
