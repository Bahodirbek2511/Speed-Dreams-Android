#include "phone_input_adapter.h"
#include "vr_state.h"
#include <GL/gl.h>
#include <cstring>

extern "C" {
int vr_inStereoFrame=0;
int vr_curEye=0;
float vr_eyeFov[2][4]={{-0.785398f,0.785398f,0.785398f,-0.785398f},
                       {-0.785398f,0.785398f,0.785398f,-0.785398f}};
}
namespace {
bool currentRaceFrame=false;
bool previousRaceFrame=false;
void copy3(const float* src,float* dst)
{
    if(src && dst && src!=dst) std::memcpy(dst,src,sizeof(float)*3);
}
}
extern "C" {
void VrApplyEyePose(const float* eye,const float* center,const float* up,
                    float* eyeOut,float* centerOut,float* upOut)
{
    copy3(eye,eyeOut); copy3(center,centerOut); copy3(up,upOut);
}
void VrEyeFromSeatGL(float* matrix)
{
    if(!matrix) return;
    std::memset(matrix,0,sizeof(float)*16);
    matrix[0]=matrix[5]=matrix[10]=matrix[15]=1.0f;
}
void VrApplyEyeRotationOnly(const float* center,const float* up,float* centerOut,float* upOut)
{
    copy3(center,centerOut); copy3(up,upOut);
}
void VrSetupHudProjection(float left,float right,float bottom,float top)
{
    int width=0,height=0; SDPhoneGetSurfaceSize(&width,&height);
    if(width<=0 || height<=0) return;
    glViewport(0,0,width,height);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(left,right,bottom,top,-1.0,1.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}
void VrFrameBegin(void)
{
    if(SDPhoneEnsureSurface()) {
        SDPhoneUpdateInput(previousRaceFrame?1:0);
        VrMonoBegin();
    }
}
void VrMonoBegin(void)
{
    int width=0,height=0; SDPhoneGetSurfaceSize(&width,&height);
    if(width>0 && height>0) glViewport(0,0,width,height);
    vr_inStereoFrame=0; vr_curEye=0;
}
void VrPresent(void)
{
    int width=0,height=0; SDPhoneGetSurfaceSize(&width,&height);
    if(currentRaceFrame) SDPhoneDrawControls(width,height);
    SDPhonePresent();
    previousRaceFrame=currentRaceFrame;
    currentRaceFrame=false;
    VrMonoBegin();
}
void VrRaceFrameBegin(void) { currentRaceFrame=true; vr_inStereoFrame=0; vr_curEye=0; }
void VrRaceEyeBegin(int eye)
{
    vr_curEye=0; vr_inStereoFrame=0;
    int width=0,height=0; SDPhoneGetSurfaceSize(&width,&height);
    if(width>0 && height>0) glViewport(0,0,width,height);
    (void)eye; // the phone build must compile the race renderer as a single view
}
void VrRaceEyeEnd(int) { vr_inStereoFrame=0; }
void VrRaceFrameEnd(void) { vr_inStereoFrame=0; }
void VrRequestRecenter(void) {}
void VrSystemRecentered(void) {}
void VrGetEyeSize(int* width,int* height) { SDPhoneGetSurfaceSize(width,height); }
int VrInMenu(void) { return previousRaceFrame?0:1; }
bool VR_UseScreenLayer() { return false; }
bool VrUseScreenLayerFlag() { return false; }
int VrFrameWasStereo(void) { return previousRaceFrame?1:0; }
void VrPlaceEyeRect(float,float,float,int width,int height,int* x,int* y)
{
    int sw=0,sh=0; SDPhoneGetSurfaceSize(&sw,&sh);
    if(x) *x=(sw-width)/2;
    if(y) *y=(sh-height)/2;
}
void VrPerfMark(int) {}
void VrPerfWaited(long long) {}
void VrPerfScene(int) {}
void VrPerfLeaves(int) {}
void VrPerfTrackPart(void) {}
void VrShaderCacheFlush(void) {}
}
