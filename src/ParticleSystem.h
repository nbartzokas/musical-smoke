//
//  ParticleSystem.h
//  SmoothDisplacementMapping
//

#ifndef ParticleSystem_h
#define ParticleSystem_h

#include "cinder/Rand.h"

class ParticleSystem{
    
public:
    void setup();
    void update();
    void draw(float volume);
    
    void loadBuffers();
    void loadShaders();
    void loadTexture();
    
    float r1 = 0.2, r2 = 0.3, g1 = 0.1, g2 = 0.2, b1 = 0.05, b2 = 0.1, a1 = 0.0, a2 = 1.0;

private:
    cinder::gl::VaoRef						mPVao[2];
    cinder::gl::TransformFeedbackObjRef		mPFeedbackObj[2];
    cinder::gl::VboRef						mPPositions[2], mPVelocities[2], mPStartTimes[2], mPInitPosition, mPInitVelocity;
    
    cinder::gl::GlslProgRef					mPUpdateGlsl, mPRenderGlsl;
    cinder::gl::TextureRef					mParticlesTexture;
    
    cinder::Rand							mRand;
    cinder::CameraPersp						mCam;
    cinder::TriMeshRef						mTrimesh;
    uint32_t                                mDrawBuff;
    
    cinder::vec3 Position0;

};

#endif /* ParticleSystem_h */
