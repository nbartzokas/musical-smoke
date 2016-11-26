/*
 Copyright (c) 2010-2012, Paul Houx - All rights reserved.
 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this list of conditions and
 the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/MonitorNode.h"
#include "cinder/audio/Utilities.h"
#include "cinder/audio/audio.h"
#include "cinder/Camera.h"
#include "cinder/CameraUi.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/params/Params.h"
#include "cinder/Surface.h"

#include "ParticleSystem.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#pragma mark Class

class MusicalSmokeApp : public App {
  public:
	static void prepare( Settings *settings );

	void setup() override;
	void update() override;
	void draw() override;

	void resize() override;

	void mouseMove( MouseEvent event ) override;
	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void mouseUp( MouseEvent event ) override;

	void keyDown( KeyEvent event ) override;
	void keyUp( KeyEvent event ) override;

  private:
	void createMesh();
	void createTextures();
	bool compileShaders();

    void renderPingPong();
	void renderDisplacementMap();
	void renderNormalMap();

	void resetCamera();

  private:
	float mAmplitude;
	float mAmplitudeTarget;

	CameraPersp mCamera;
	CameraUi    mCameraUi;
    
    
    vector<gl::FboRef> mPingPong;
    int drawFbo = 0;
    
    gl::GlslProgRef  mPingPongShader;

    gl::FboRef mDispMapFbo;
	gl::GlslProgRef mDispMapShader;

	gl::FboRef      mNormalMapFbo;
	gl::GlslProgRef mNormalMapShader;

	gl::VboMeshRef  mVboMesh;
	gl::GlslProgRef mMeshShader;
	gl::BatchRef    mBatch;

	gl::Texture2dRef mBackgroundTexture;
    gl::GlslProgRef  mBackgroundShader;
    
    Color color = Color(1,1,1);
    
    ci::params::InterfaceGlRef params;
    void setupParams();
    
    audio::DelayNodeRef             mDelayNode;
    audio::MonitorNodeRef           mMonitorNode;
    audio::MonitorSpectralNodeRef	mMonitorSpectralNode;
    audio::FilterBandPassNodeRef    mFilterBandPassNode;
    vector<float>					mMagSpectrum;
    ci::audio::GainNodeRef			mGain;
    ci::audio::BufferPlayerNodeRef	mBufferPlayerNode;
    gl::TextureFontRef				mTextureFont;
    void setupAudio();
    float mVolume = 0;
    float mVolumeSmoothed = 0;

    ParticleSystem particleSystem;
    
    const vec2 fboBounds = vec2(256,256);
    
#pragma mark Settings
    
    // toggles
    bool showParams=true;
    bool hiddenCursor=false;
    bool mDrawTextures = false;
    bool mDrawWireframe = false;
    bool mDrawOriginalMesh = false;
    bool mEnableShader = false;
    
    // movement
    float dx = 0.005f; // speed of audio propegation across mesh
    float mAudioAmplitude = 10.0; // amplitude of audio displacement of mesh
    bool audioMovementStraight = true;
    float mSmoothness = 0.5;
    
    // colors
    Color mLineColor1 = Color(.5,.5,.4);
    Color mLineColor2 = Color(1,.8,.7);
    float mLineGapAlpha = 0.5;
    Color mFalloffColor = Color(0,0,0);
    Color mVolumeColor = Color(1,0,0);
    
    // lines
    bool mEnableLines = false;
    bool mLengthLines = true;
    float mLineWidth = 1.0;
    
    // background
    bool bgSolid = false;
    float mHue = 3.06, mBrightness = 0.17;
    Color bgColor = Color(0,0,0);
    
    float freqMag = 0.2f;
    float timeMag = 0.2f;
    float dirMag = 0.2f;
    float posMag = 1.0f;
    float gainLevel = 1.0f;
    float filterFreq = 10000.0f;
    float filterQ = 100.0f;
    float delay = 0.015;
    
};




#pragma mark Class Methods

void MusicalSmokeApp::prepare( Settings *settings )
{
	settings->setTitle( "Vertex Displacement Mapping with Smooth Normals" );
    settings->setFullScreen();
	settings->disableFrameRate();
}

void MusicalSmokeApp::setup()
{
    hideCursor();
    setupParams();
    setupAudio();
    
    particleSystem.setup();

	mAmplitude = 0.0f;
	mAmplitudeTarget = 10.0f;

	// initialize our camera
	mCameraUi.setCamera( &mCamera );
	resetCamera();

	// load and compile shaders
	if( !compileShaders() )
		quit();

	// create the basic mesh (a flat plane)
	createMesh();

	// create the textures
	createTextures();

	// create the frame buffer objects for the displacement map and the normal map
	gl::Fbo::Format fmt;
	fmt.enableDepthBuffer( false );

	// use a single channel (red) for the displacement map
	fmt.setColorTextureFormat( gl::Texture2d::Format().wrap( GL_CLAMP_TO_EDGE ).internalFormat( GL_R32F ) );
    mPingPong = {
        gl::Fbo::create( fboBounds.x, fboBounds.y, fmt ),
        gl::Fbo::create( fboBounds.x, fboBounds.y, fmt )
    };
    {
        ci::gl::ScopedFramebuffer fbo( mPingPong[0] );
        gl::clear(  ColorA( 0, 0, 0, 1 ) );
    }
    {
        ci::gl::ScopedFramebuffer fbo( mPingPong[1] );
        gl::clear(  ColorA( 0, 0, 0, 1 ) );
    }
    
    mDispMapFbo = gl::Fbo::create( fboBounds.x, fboBounds.y, fmt );

	// use 3 channels (rgb) for the normal map
	fmt.setColorTextureFormat( gl::Texture2d::Format().wrap( GL_CLAMP_TO_EDGE ).internalFormat( GL_RGB32F ) );
	mNormalMapFbo = gl::Fbo::create( fboBounds.x, fboBounds.y, fmt );
    
}

void MusicalSmokeApp::setupParams(){
    
    params = params::InterfaceGl::create( getWindow(), "App parameters", toPixels( ivec2( 200, 300 ) ) );
    params->addParam( "Draw Textures",    &mDrawTextures );
    params->addParam( "Draw Wireframes",    &mDrawWireframe );
    params->addParam( "Draw Original Mesh",    &mDrawOriginalMesh );
    params->addParam( "Enable Shader",    &mEnableShader );
    params->addSeparator();
    
    params->addParam( "DX",    &dx );
    params->addParam( "Audio Amplitude",    &mAudioAmplitude );
    params->addParam( "Audio Movement Straight",    &audioMovementStraight );
    params->addParam( "Volume Smoothness",    &mSmoothness );
    params->addSeparator();
    
    params->addParam( "Line Color 1",    &mLineColor1 );
    params->addParam( "Line Color 2",    &mLineColor2 );
    params->addParam( "Falloff Color",    &mFalloffColor );
    params->addParam( "Volume Color",    &mVolumeColor );
    params->addParam( "Line Gap Alpha", &mLineGapAlpha );
    params->addSeparator();
    
    params->addParam( "Enable Lines",    &mEnableLines );
    params->addParam( "Length Lines",    &mLengthLines );
    params->addParam( "Line Width",    &mLineWidth );
    params->addSeparator();
    
    params->addParam( "BG Solid", &bgSolid);
    params->addParam( "BG Color", &bgColor);
    params->addParam( "BG Hue", &mHue ).step(0.01);
    params->addParam( "BG Brightness", &mBrightness).step(0.01);
    params->addSeparator();
    
    params->addParam( "r1", &particleSystem.r1);
    params->addParam( "r2", &particleSystem.r2);
    params->addParam( "g1", &particleSystem.g1);
    params->addParam( "g2", &particleSystem.g2);
    params->addParam( "b1", &particleSystem.b1);
    params->addParam( "b2", &particleSystem.b2);
    params->addParam( "a1", &particleSystem.a1);
    params->addParam( "a2", &particleSystem.a2);
    
    params->addParam( "Gain Level", &gainLevel );
    params->addParam( "Delay", &delay).updateFn( [&](){
        mDelayNode->setDelaySeconds(delay);
    });
    
    params->addParam( "Dir Mag", &dirMag );
    params->addParam( "Pos Mag", &posMag );
    params->addParam( "Time Mag", &timeMag );
    params->addParam( "Freq Mag", &freqMag );
    params->addParam( "Filter Freq", &filterFreq );
    params->addParam( "Filter Q", &filterQ );
    params->addSeparator();
    
}

void MusicalSmokeApp::setupAudio(){
    
    // audio
    auto ctx = audio::Context::master();
    
    // MP3
    ci::audio::SourceFileRef sourceFile = ci::audio::load(ci::app::loadAsset("sample.mp3") , ctx->getSampleRate() );
    audio::BufferRef buffer = sourceFile->loadBuffer();
    mBufferPlayerNode = ctx->makeNode( new ci::audio::BufferPlayerNode( buffer ) );
    mGain = ctx->makeNode( new audio::GainNode( gainLevel ) );
    mDelayNode = ctx->makeNode( new audio::DelayNode() );
    mDelayNode->setDelaySeconds(delay);
    
    // Filter
    mFilterBandPassNode = ctx->makeNode( new audio::FilterBandPassNode() );
    mFilterBandPassNode->setCenterFreq(filterFreq);
    mFilterBandPassNode->setQ(filterQ);
    
    // Time Domain
    auto monitorFormat = audio::MonitorNode::Format().windowSize( 1024 );
    mMonitorNode = ctx->makeNode( new audio::MonitorNode( monitorFormat ) );
    
    // Frequency Domain (FFT)
    auto monitorSpectralFormat = audio::MonitorSpectralNode::Format().fftSize( 2048 ).windowSize( 1024 );
    mMonitorSpectralNode = ctx->makeNode( new audio::MonitorSpectralNode( monitorSpectralFormat ) );
    
    mBufferPlayerNode
    >> mGain
    >> mDelayNode
    >> ctx->getOutput()
    ;
    
    mBufferPlayerNode
    >> mGain
//    >> mFilterBandPassNode
    >> mMonitorNode
    >> mMonitorSpectralNode
    ;
    
    ctx->enable();
    
    mBufferPlayerNode->start();
    
}

void MusicalSmokeApp::update()
{
    //    mGain->setValue(gainLevel);
    mFilterBandPassNode->setCenterFreq(filterFreq);
    mFilterBandPassNode->setQ(filterQ);
    mFilterBandPassNode->setGain(gainLevel);
    
    mVolume = mMonitorNode->getVolume();
    mVolumeSmoothed = (1-mSmoothness) * mVolumeSmoothed + mSmoothness * mVolume;
    color = Color( mVolume, mVolume, mVolume );
    
	mAmplitude += 0.02f * ( mAmplitudeTarget - mAmplitude );
    
    // render pingpong fbo
    renderPingPong();
	
    // render displacement map
	renderDisplacementMap();

	// render normal map
    renderNormalMap();
    
    particleSystem.update();
}

void MusicalSmokeApp::draw()
{

	// render background
	if( !bgSolid && mBackgroundTexture && mBackgroundShader ) {
        gl::clear();
		gl::ScopedTextureBind tex0( mBackgroundTexture );
		gl::ScopedGlslProg    shader( mBackgroundShader );
        mBackgroundShader->uniform( "uTex0", 0 );
        mBackgroundShader->uniform( "uHue", mHue ); //float( 0.025 * getElapsedSeconds() ) );
        mBackgroundShader->uniform( "uBrightness", mBrightness );
		gl::drawSolidRect( getWindowBounds() );
    }else{
        gl::clear( bgColor );
    }
    
    particleSystem.draw(mVolumeSmoothed);

	// if enabled, show the displacement and normal maps
    if( mDrawTextures ) {
        gl::color( Color( 1, 1, 1 ) );
        gl::draw( mPingPong[drawFbo]->getColorTexture(), vec2( 0 ) );
        gl::color( Color( 1, 1, 1 ) );
        gl::draw( mDispMapFbo->getColorTexture(), vec2( 256, 0 ) );
		gl::color( Color( 1, 1, 1 ) );
		gl::draw( mNormalMapFbo->getColorTexture(), vec2( 512, 0 ) );
	}

	// setup the 3D camera
	gl::pushMatrices();
	gl::setMatrices( mCamera );

	// setup render states
	gl::enableAdditiveBlending();
	if( mDrawWireframe )
		gl::enableWireframe();

	// draw undisplaced mesh if enabled
	if( mDrawOriginalMesh ) {
		gl::color( ColorA( 1, 1, 1, 0.2f ) );
		gl::draw( mVboMesh );
	}

	if( mDispMapFbo && mNormalMapFbo && mMeshShader ) {
		// bind the displacement and normal maps, each to their own texture unit
        gl::ScopedTextureBind tex0( mDispMapFbo->getColorTexture(), (uint8_t)0 );
        gl::ScopedTextureBind tex1( mNormalMapFbo->getColorTexture(), (uint8_t)1 );
        gl::ScopedTextureBind tex2( mPingPong[drawFbo]->getColorTexture(), (uint8_t)2 );

		// render our mesh using vertex displacement
		gl::ScopedGlslProg shader( mMeshShader );
        mMeshShader->uniform( "uTexDisplacement", 0 );
        mMeshShader->uniform( "uTexNormal", 1 );
        mMeshShader->uniform( "uTexAudio", 2 );
        mMeshShader->uniform( "uEnableFallOff", mEnableShader );
        mMeshShader->uniform( "uEnableLines", mEnableLines );
        mMeshShader->uniform( "uLineWidth", mLineWidth );
        mMeshShader->uniform( "uLineColor1", mLineColor1 );
        mMeshShader->uniform( "uLineColor2", mLineColor2 );
        mMeshShader->uniform( "uLineGapAlpha", mLineGapAlpha );
        mMeshShader->uniform( "uVolumeColor", mVolumeColor );
        mMeshShader->uniform( "uFalloffColor", mFalloffColor );

		gl::color( Color::white() );
		mBatch->draw();
	}

	// clean up after ourselves
	gl::disableWireframe();
	gl::disableAlphaBlending();

    gl::popMatrices();
    
    if (showParams) params->draw();
}

void MusicalSmokeApp::resetCamera()
{
 	mCamera.lookAt( vec3( 78.185,    4.692,   87.365 ), vec3( -0.666,   -0.040,   -0.745 ) );
}



#pragma mark Render Shaders, Supply Uniforms

void MusicalSmokeApp::renderPingPong()
{
    {
        gl::FboRef f = mPingPong[drawFbo];
        gl::FboRef f2 = mPingPong[1-drawFbo];
        
        // bind frame buffer
        gl::ScopedFramebuffer fbo( f );
        gl::ScopedViewport viewport( 0, 0, f->getWidth(), f->getHeight() );
        gl::pushMatrices();
        gl::setMatricesWindow( f->getSize() );
        gl::clear();
        
        {
            // render the displacement map
            gl::ScopedGlslProg shader( mPingPongShader );
            gl::ScopedTextureBind tex( f2->getColorTexture(), 0 );
            mPingPongShader->uniform( "uTex0", 0 );
            mPingPongShader->uniform( "dx", dx );
            gl::drawSolidRect( f->getBounds() );
        }
        {
            gl::ScopedColor c(color);
            if (audioMovementStraight){
                float width = 50.0;
                gl::drawSolidRect( Rectf( f->getWidth() - width, 0, f->getWidth(), f->getHeight() ) );
            }else{
                gl::drawSolidCircle( vec2( f->getWidth(), f->getHeight()/2 ), f->getHeight()/2 );
            }
        }
        
        gl::popMatrices();
    }
    
    drawFbo = 1 - drawFbo;
}

void MusicalSmokeApp::renderDisplacementMap()
{
	if( mDispMapShader && mDispMapFbo ) {
        
        {
            // bind frame buffer
            gl::ScopedFramebuffer fbo( mDispMapFbo );

            // setup viewport and matrices
            gl::ScopedViewport viewport( 0, 0, mDispMapFbo->getWidth(), mDispMapFbo->getHeight() );

            gl::pushMatrices();
            gl::setMatricesWindow( mDispMapFbo->getSize() );

            // clear the color buffer
            gl::clear();
            
            {
                // render the displacement map
                gl::ScopedGlslProg shader( mDispMapShader );
                gl::ScopedTextureBind tex( mPingPong[drawFbo]->getColorTexture(), 0 );
                mDispMapShader->uniform( "uTime", float( getElapsedSeconds() ) );
                mDispMapShader->uniform( "uAmplitude", mAmplitude );
                mDispMapShader->uniform( "uAudioAmplitude", mAudioAmplitude );
                mDispMapShader->uniform( "uTex0", 0 );
                gl::drawSolidRect( mDispMapFbo->getBounds() );
            }
            
            // clean up after ourselves
            gl::popMatrices();
        }
        
	}
}

void MusicalSmokeApp::renderNormalMap()
{
	if( mNormalMapShader && mNormalMapFbo ) {
		// bind frame buffer
		gl::ScopedFramebuffer fbo( mNormalMapFbo );

		// setup viewport and matrices
		gl::ScopedViewport viewport( 0, 0, mNormalMapFbo->getWidth(), mNormalMapFbo->getHeight() );

		gl::pushMatrices();
		gl::setMatricesWindow( mNormalMapFbo->getSize() );

		// clear the color buffer
		gl::clear();

		// bind the displacement map
		gl::ScopedTextureBind tex0( mDispMapFbo->getColorTexture() );

		// render the normal map
		gl::ScopedGlslProg shader( mNormalMapShader );
		mNormalMapShader->uniform( "uTex0", 0 );
		mNormalMapShader->uniform( "uAmplitude", 4.0f );

		Area bounds = mNormalMapFbo->getBounds();
		gl::drawSolidRect( bounds );

		// clean up after ourselves
		gl::popMatrices();
	}
}

bool MusicalSmokeApp::compileShaders()
{
	try {
		// this shader will render all colors using a change in hue
		mBackgroundShader = gl::GlslProg::create( loadAsset( "background.vert" ), loadAsset( "background.frag" ) );
        // this shader will propegate audio displacement across the mesh
        mPingPongShader = gl::GlslProg::create( loadAsset( "slide.vert" ), loadAsset( "slide.frag" ) );
		// this shader will render a displacement map to a floating point texture, updated every frame
		mDispMapShader = gl::GlslProg::create( loadAsset( "displacement_map.vert" ), loadAsset( "displacement_map.frag" ) );
		// this shader will create a normal map based on the displacement map
		mNormalMapShader = gl::GlslProg::create( loadAsset( "normal_map.vert" ), loadAsset( "normal_map.frag" ) );
		// this shader will use the displacement and normal maps to displace vertices of a mesh
		mMeshShader = gl::GlslProg::create( loadAsset( "mesh.vert" ), loadAsset( "mesh.frag" ) );
	}
	catch( const std::exception &e ) {
		console() << e.what() << std::endl;
		return false;
	}

	return true;
}

#pragma mark Events

void MusicalSmokeApp::resize()
{
	// if window is resized, update camera aspect ratio
	mCamera.setAspectRatio( getWindowAspectRatio() );
}

void MusicalSmokeApp::mouseMove( MouseEvent event )
{
}

void MusicalSmokeApp::mouseDown( MouseEvent event )
{
	// handle user input
	mCameraUi.mouseDown( event.getPos() );
}

void MusicalSmokeApp::mouseDrag( MouseEvent event )
{
	// handle user input
	mCameraUi.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void MusicalSmokeApp::mouseUp( MouseEvent event )
{
}

void MusicalSmokeApp::keyDown( KeyEvent event )
{
	switch( event.getCode() ) {
        case KeyEvent::KEY_ESCAPE:
            // quit
            quit();
            break;
        case KeyEvent::KEY_f:
            // toggle full screen
            setFullScreen( !isFullScreen() );
            break;
        case '`':
            showParams = !showParams;
            break;
        case KeyEvent::KEY_m:
            // toggle original mesh
            mDrawOriginalMesh = !mDrawOriginalMesh;
            break;
        case KeyEvent::KEY_s:
            // reload shaders
            compileShaders();
            break;
        case KeyEvent::KEY_t:
            // toggle draw textures
            mDrawTextures = !mDrawTextures;
            break;
        case KeyEvent::KEY_v:
            // toggle vertical sync
            gl::enableVerticalSync( !gl::isVerticalSyncEnabled() );
            break;
        case KeyEvent::KEY_w:
            // toggle wire frame
            mDrawWireframe = !mDrawWireframe;
            break;
        case KeyEvent::KEY_SPACE:
            // reset camera
            resetCamera();
            break;
        case KeyEvent::KEY_a:
            if( mAmplitudeTarget < 10.0f )
                mAmplitudeTarget = 10.0f;
            else
                mAmplitudeTarget = 0.0f;
            break;
        case KeyEvent::KEY_q:
            mEnableShader = !mEnableShader;
            break;
	}
}

void MusicalSmokeApp::keyUp( KeyEvent event )
{
}

#pragma mark Create Meshes, Textures

void MusicalSmokeApp::createMesh()
{
	// create vertex, normal and texcoord buffers
	const int  RES_X = 398;
	const int  RES_Z = 98;
	const vec3 size = vec3( 200.0f, 1.0f, 50.0f );

	std::vector<vec3> positions( RES_X * RES_Z );
	std::vector<vec3> normals( RES_X * RES_Z );
	std::vector<vec2> texcoords( RES_X * RES_Z );
    
    std::vector<Color> colors(RES_X * RES_Z);
    
	int i = 0;
	for( int x = 0; x < RES_X; ++x ) {
		for( int z = 0; z < RES_Z; ++z ) {
            
			float u = float( x ) / RES_X;
			float v = float( z ) / RES_Z;
			positions[i] = size * vec3( u - 0.5f, 0.0f, v - 0.5f );
			normals[i] = vec3( 0, 1, 0 );
			texcoords[i] = vec2( u, v );
            
            bool drawLengthLines = mLengthLines && z%2==0;
            bool drawWidthLines = !mLengthLines && x%2==0;
            if ( drawLengthLines || drawWidthLines ){
                colors[i] = Color(1,v,u);
            }else {
                colors[i] = Color(0,v,u);
            }

			i++;
		}
	}

	// create index buffer
	vector<uint16_t> indices;
	indices.reserve( 6 * ( RES_X - 1 ) * ( RES_Z - 1 ) );
	for( int x = 0; x < RES_X - 1; ++x ) {
		for( int z = 0; z < RES_Z - 1; ++z ) {
            uint16_t i = x * RES_Z + z;
            
            indices.push_back( i );
            indices.push_back( i + 1 );
            indices.push_back( i + RES_Z );
            indices.push_back( i + RES_Z );
            indices.push_back( i + 1 );
            indices.push_back( i + RES_Z + 1 );
		}
	}

	// construct vertex buffer object
	gl::VboMesh::Layout layout;
    layout.attrib( geom::POSITION, 3 );
    layout.attrib( geom::NORMAL, 3 );
    layout.attrib( geom::COLOR, 3 );
	layout.attrib( geom::TEX_COORD_0, 2 );

	mVboMesh = gl::VboMesh::create( positions.size(), GL_TRIANGLES, { layout }, indices.size() );
	mVboMesh->bufferAttrib( geom::POSITION, positions.size() * sizeof( vec3 ), positions.data() );
    mVboMesh->bufferAttrib( geom::NORMAL, normals.size() * sizeof( vec3 ), normals.data() );
    mVboMesh->bufferAttrib( geom::COLOR, colors.size() * sizeof( Color ), colors.data() );
	mVboMesh->bufferAttrib( geom::TEX_COORD_0, texcoords.size() * sizeof( vec2 ), texcoords.data() );
	mVboMesh->bufferIndices( indices.size() * sizeof( uint16_t ), indices.data() );

	// create a batch for better performance
	mBatch = gl::Batch::create( mVboMesh, mMeshShader );
}

void MusicalSmokeApp::createTextures()
{
	try {
		mBackgroundTexture = gl::Texture2d::create( loadImage( loadAsset( "background.png" ) ) );
	}
	catch( const std::exception &e ) {
		console() << "Could not load image: " << e.what() << std::endl;
	}
}

CINDER_APP( MusicalSmokeApp, RendererGl( RendererGl::Options().msaa( 16 ) ), &MusicalSmokeApp::prepare )
