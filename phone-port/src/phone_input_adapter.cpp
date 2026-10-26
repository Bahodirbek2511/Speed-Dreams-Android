#include "phone_input_adapter.h"
#include "phone_controls.h"
#include "vr_events.h"
#include <SDL.h>
#include <SDL_keycode.h>
#include <GL/gl.h>
#include <algorithm>
#include <cmath>

namespace {
sdphone::PhoneControls gControls;
sdphone::PhoneFrameInput gLastInput;

void rect(float x0,float y0,float x1,float y1,float r,float g,float b,float a)
{
    glColor4f(r,g,b,a); glBegin(GL_QUADS);
    glVertex2f(x0,y0); glVertex2f(x1,y0); glVertex2f(x1,y1); glVertex2f(x0,y1);
    glEnd();
}
void line(float x0,float y0,float x1,float y1,float r,float g,float b,float a)
{
    glColor4f(r,g,b,a); glBegin(GL_LINES);
    glVertex2f(x0,y0); glVertex2f(x1,y1); glEnd();
}
void circle(float x,float y,float radius,float r,float g,float b,float a)
{
    glColor4f(r,g,b,a); glBegin(GL_TRIANGLE_FAN); glVertex2f(x,y);
    for(int i=0;i<=32;++i) {
        const float angle=static_cast<float>(i)*6.28318530718f/32.0f;
        glVertex2f(x+std::cos(angle)*radius,y+std::sin(angle)*radius);
    }
    glEnd();
}
void triangle(float x,float y,float radius,bool right)
{
    glColor4f(1,1,1,0.92f); glBegin(GL_TRIANGLES);
    if(right) {
        glVertex2f(x-radius*0.5f,y-radius); glVertex2f(x-radius*0.5f,y+radius);
        glVertex2f(x+radius,y);
    } else {
        glVertex2f(x+radius*0.5f,y-radius); glVertex2f(x+radius*0.5f,y+radius);
        glVertex2f(x-radius,y);
    }
    glEnd();
}
}

extern "C" void SDPhoneTouchEvent(int phase,int pointerId,float x,float y)
{
    if (phase < 0 || phase > 3) return;
    gControls.onTouch(static_cast<sdphone::TouchPhase>(phase),pointerId,x,y);
}

extern "C" void SDPhoneUpdateInput(int racing)
{
    int width=0,height=0;
    SDPhoneGetSurfaceSize(&width,&height);
    gLastInput=gControls.update(width,height,racing!=0);

#ifdef SD_PHONE_UPSTREAM_MAIN
    // Public upstream main consumes SDL events, not the VR overlay's private queue.
    for(const auto& event:gLastInput.menuEvents) {
        SDL_Event queued{};
        if(event.type==sdphone::MenuEventType::Motion) {
            queued.type=SDL_MOUSEMOTION;
            queued.motion.x=event.x;
            queued.motion.y=event.y;
            queued.motion.state=event.button ? SDL_BUTTON_LMASK : 0;
        } else {
            queued.type=(event.type==sdphone::MenuEventType::ButtonDown)
                ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
            queued.button.button=SDL_BUTTON_LEFT;
            queued.button.state=(event.type==sdphone::MenuEventType::ButtonDown)
                ? SDL_PRESSED : SDL_RELEASED;
            queued.button.x=event.x;
            queued.button.y=event.y;
        }
        SDL_PushEvent(&queued);
    }
    for(const auto action:gLastInput.actionKeys) {
        const SDL_Keycode key=action==sdphone::ActionKey::Pause ? SDLK_ESCAPE : SDLK_F2;
        SDL_Event down{};
        down.type=SDL_KEYDOWN;
        down.key.state=SDL_PRESSED;
        down.key.repeat=0;
        down.key.keysym.sym=key;
        SDL_PushEvent(&down);
        SDL_Event up{};
        up.type=SDL_KEYUP;
        up.key.state=SDL_RELEASED;
        up.key.keysym.sym=key;
        SDL_PushEvent(&up);
    }
#else
    for(const auto& event:gLastInput.menuEvents) {
        if(event.type==sdphone::MenuEventType::Motion) VrQueueMouseMotion(event.x,event.y,0);
        else if(event.type==sdphone::MenuEventType::ButtonDown) VrQueueMouseButton(event.button,1,event.x,event.y);
        else if(event.type==sdphone::MenuEventType::ButtonUp) VrQueueMouseButton(event.button,0,event.x,event.y);
    }
    for(const auto action:gLastInput.actionKeys) {
        const int key=action==sdphone::ActionKey::Pause ? SDLK_ESCAPE : SDLK_F2;
        VrQueueKey(key,1,0,0); VrQueueKey(key,0,0,0);
    }
#endif
}


extern "C" int SDPhoneReadJoystick(int* buttons, float* axes)
{
    if (!buttons || !axes) return -1;
    *buttons = static_cast<int>(gLastInput.buttons);
    for (std::size_t i=0; i<8; ++i) axes[i] = gLastInput.axes[i];
    return 0;
}

extern "C" int VrJoyRead(int index,int* buttons,float* axes)
{
    if(index!=0 || !buttons || !axes) return -1;
    *buttons=static_cast<int>(gLastInput.buttons);
    for(std::size_t i=0;i<gLastInput.axes.size();++i) axes[i]=gLastInput.axes[i];
    return 0;
}

extern "C" void SDPhoneDrawControls(int width,int height)
{
    if(!gLastInput.showDrivingOverlay || width<=0 || height<=0) return;
    const float w=static_cast<float>(width),h=static_cast<float>(height);
    GLint oldMode=GL_MODELVIEW;
    glGetIntegerv(GL_MATRIX_MODE,&oldMode);
    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_CURRENT_BIT|GL_LINE_BIT);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glOrtho(0.0,w,0.0,h,-1.0,1.0);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_TEXTURE_2D); glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(std::max(2.0f,h*0.003f));

    const float cx=w*0.22f,cy=h*0.27f,r=std::min(w*0.10f,h*0.19f);
    circle(cx,cy,r,0.10f,0.12f,0.18f,0.34f);
    circle(cx,cy,r*0.70f,0.08f,0.10f,0.15f,0.50f);
    triangle(cx-r*0.30f,cy,r*0.24f,false);
    triangle(cx+r*0.30f,cy,r*0.24f,true);

    // These visual areas share the hit zones in phone_controls.cpp.
    rect(w*0.65f,h*0.42f,w*0.98f,h*0.78f,0.75f,0.10f,0.10f,0.34f);
    rect(w*0.65f,h*0.04f,w*0.98f,h*0.42f,0.10f,0.62f,0.22f,0.36f);
    line(w*0.72f,h*0.60f,w*0.91f,h*0.60f,1.0f,0.9f,0.9f,0.85f);
    line(w*0.72f,h*0.22f,w*0.91f,h*0.22f,0.9f,1.0f,0.9f,0.85f);

    const float y0=h*0.81f,y1=h*0.97f;
    const float bx[]={0.00f,0.12f,0.24f,0.36f,0.48f,0.60f,0.72f,0.86f};
    for(int i=0;i<7;++i) rect(w*bx[i]+2,y0,w*bx[i+1]-2,y1,0.10f,0.12f,0.18f,0.64f);
    rect(w*0.035f,h*0.85f,w*0.047f,h*0.93f,1,1,1,0.95f);
    rect(w*0.070f,h*0.85f,w*0.082f,h*0.93f,1,1,1,0.95f);
    triangle(w*0.18f,h*0.89f,h*0.035f,false);
    triangle(w*0.30f,h*0.89f,h*0.035f,false);
    triangle(w*0.42f,h*0.89f,h*0.035f,true);
    rect(w*0.515f,h*0.855f,w*0.565f,h*0.925f,0.95f,0.95f,1,0.95f);
    circle(w*0.54f,h*0.89f,h*0.018f,0.08f,0.10f,0.15f,1);
    line(w*0.625f,h*0.87f,w*0.695f,h*0.91f,1,1,1,0.9f);
    line(w*0.745f,h*0.89f,w*0.815f,h*0.89f,1,1,1,0.9f);

    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(static_cast<GLenum>(oldMode)); glPopAttrib();
}
