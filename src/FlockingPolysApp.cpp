#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
// 3d stuff.
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Batch.h"
#include "cinder/params/Params.h"
#include "cinder/CameraUi.h"
#include "cinder/Rand.h"
#include "Resources.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class FlockingPolysApp : public App {
  public:
    static void prepareSettings ( Settings* settings );
	void setup() override;
	void mouseDown( MouseEvent event ) override;
    void keyDown( KeyEvent event ) override;
	void update() override;
	void draw() override;
    void spawnBird(const vec3&);
    void setDefaultValues();
    
    vector<vec3> birds;
    vector<vec3> orients;
    vector<vec3> velocs;
    
    Rand    rng;
    CameraPersp					mCam;
    gl::GlslProgRef				mObjectGlsl;
    gl::GlslProgRef				lshad;
    
    gl::TextureCubeMapRef		mCubeMap;
    std::vector<gl::BatchRef>	mObjects;
    size_t						mCurrObject;

    // params for the main camera
    params::InterfaceGlRef		mParams;
    vec3						mEyePoint;
    vec3						mLookAt;
    float						mFov;
    float						mAspectRatio;
    float						mNearPlane;
    float						mFarPlane;
    float                       m_fLastTime;
    vec2						mLensShift;
    
    vector<string>				mObjectNames;
    int							nBirds;
    
    ivec2						mLastMousePos;
};

void FlockingPolysApp::prepareSettings( Settings* settings )
{
    settings->setResizable( false );
}

void FlockingPolysApp::setup()
{
    setWindowSize( 1024, 768 );
    gl::enableDepthRead();
    gl::enableDepthWrite();
    gl::enableAlphaBlending();
    setDefaultValues();
    
    
    try {
        auto lambert = gl::ShaderDef().lambert().color();
        lshad = gl::getStockShader( lambert );

        mCubeMap = gl::TextureCubeMap::create( loadImage( loadResource( RES_ENV_MAP ) ), gl::TextureCubeMap::Format().mipmap() );
        mObjectGlsl = gl::GlslProg::create( loadAsset( "objectshader.vert" ), loadAsset( "objectshader.frag" ) );
    } catch ( std::exception& e ) {
        console() << e.what() << endl;
        quit();
    }
    
//    gl::BatchRef ref = gl::Batch::create( geom::Teapot().subdivisions( 16 ), mObjectGlsl );
//    ref->getGlslProg()->uniform( "uCubeMapTex", 0 );
//    mObjects.push_back( ref );
    
    spawnBird(vec3(2.0,2.0,4.0));
    mParams = params::InterfaceGl::create( getWindow(), "CameraPersp", toPixels( ivec2( 200, 300 ) ) );
    nBirds = birds.size();
    mParams->addParam( "NBirds", &nBirds);
//    .keyDecr( "[" )
//    .keyIncr( "]" )
//    .updateFn( [this] { mCurrObject = (size_t)nBirds; } );
    mParams->addSeparator();
    mParams->addParam( "Eye Point X", &mEyePoint.x ).step( 0.01f );
    mParams->addParam( "Eye Point Y", &mEyePoint.y ).step( 0.01f );
    mParams->addParam( "Eye Point Z", &mEyePoint.z ).step( 0.01f );
    mParams->addSeparator();
    mParams->addParam( "Look At X", &mLookAt.x ).step( 0.01f );
    mParams->addParam( "Look At Y", &mLookAt.y ).step( 0.01f );
    mParams->addParam( "Look At Z", &mLookAt.z ).step( 0.01f );
    mParams->addSeparator();
    mParams->addParam( "FOV", &mFov ).min( 1.0f ).max( 179.0f );
    mParams->addParam( "Near Plane", &mNearPlane ).step( 0.02f ).min( 0.1f );
    mParams->addParam( "Far Plane", &mFarPlane ).step( 0.02f ).min( 0.1f );
    mParams->addParam( "Lens Shift X", &mLensShift.x ).step( 0.01f );
    mParams->addParam( "Lens Shift Y", &mLensShift.y ).step( 0.01f );
    mParams->addSeparator();
    mParams->addButton( "Reset Defaults", bind ( &FlockingPolysApp::setDefaultValues, this ) );
}

void FlockingPolysApp::spawnBird(const vec3& arg_=vec3(2.0,2.0,4.0))
{
    birds.push_back(arg_);
    orients.push_back(vec3(rng.randFloat(),rng.randFloat(),0.5));
    velocs.push_back(vec3(rng.randFloat(),rng.randFloat(),0.5));
    nBirds++;
}

void FlockingPolysApp::update()
{
    float fCurrentTime = cinder::app::getElapsedSeconds();
    float fDeltaTime = fCurrentTime - m_fLastTime;
    
    vec3 fTopLeft, fTopRight, fBottomLeft, fBottomRight;
    vec3 nTopLeft, nTopRight, nBottomLeft, nBottomRight;
    mCam.getFarClipCoordinates( &fTopLeft, &fTopRight, &fBottomLeft, &fBottomRight );
    mCam.getNearClipCoordinates( &nTopLeft, &nTopRight, &nBottomLeft, &nBottomRight );
    vec3 com = (fTopLeft+ fTopRight+ fBottomLeft+ fBottomRight+ nTopLeft+ nTopRight+ nBottomLeft+ nBottomRight);
    com *= 1.0f/8.0f;
    
    nBirds=birds.size();
    // Integrate bird eom.
    for (int i =0; i<birds.size(); ++i)
    {
        vec3 frc = 0.1f*rng.randVec3();
        // Birds experience a random force + attraction to center
        frc -= 0.05f*(birds[i] - com);
        velocs[i] += frc;
        birds[i] += velocs[i]*fDeltaTime;
    }
    mObjects.clear();
    for (int i =0; i<birds.size(); ++i)
    {
        mObjects.push_back( gl::Batch::create( geom::Cone().subdivisionsAxis(10).base(0.2).set(birds[i],birds[i]+velocs[i]),lshad));
    }
    
    m_fLastTime = fCurrentTime;
}

void FlockingPolysApp::setDefaultValues()
{
    mCurrObject			= 0;
    nBirds              = 0;
    mEyePoint			= vec3( 0.0f,0.0f,0.0f );
    mLookAt				= vec3( 1.0f,1.0f,4.0f );
    mFov				= 40.0f;
    mAspectRatio		= getWindowAspectRatio();
    mNearPlane			= 2.5f;
    mFarPlane			= 15.0f;
    mLensShift			= vec2 ( 0 );
}

void FlockingPolysApp::mouseDown( MouseEvent event )
{
    vec3 newbird;
    newbird[0] = event.getX()/getWindowWidth();
    newbird[1] = event.getY()/getWindowHeight();
    newbird[2] = 5.0;
    spawnBird(newbird);
}

void FlockingPolysApp::keyDown( KeyEvent event )
{
    if ( event.getCode() == KeyEvent::KEY_ESCAPE ) quit();
    if ( event.getCode() == KeyEvent::KEY_UP )
        mLookAt.y += 0.01;
    if ( event.getCode() == KeyEvent::KEY_DOWN )
        mLookAt.y -= 0.01;
    if ( event.getCode() == KeyEvent::KEY_LEFT )
        mLookAt.x -= 0.01;
    if ( event.getCode() == KeyEvent::KEY_RIGHT )
        mLookAt.x += 0.01;
}

void FlockingPolysApp::draw()
{
    gl::clear();
    
    vec2 ten(10,10);
    string nbirds=to_string(birds.size());
    gl::drawString("NBirds:"+nbirds, ten, ColorA(1.0f,1.0f,1.0f,1.0f),Font("Arial",22));
    string bird1 = to_string(birds[0].x)+" "+to_string(birds[0].y)+" "+to_string(birds[0].z);
    gl::drawString("Bird1:"+bird1, ten+ten+ten, ColorA(1.0f,1.0f,1.0f,1.0f),Font("Arial",22));

    gl::enableDepthRead();
    gl::enableDepthWrite();
    mCam.lookAt(mEyePoint,mLookAt);
    mCam.setLensShift( mLensShift );
    mCam.setPerspective( mFov, getWindowAspectRatio(), mNearPlane, mFarPlane );
    gl::setMatrices( mCam );
     
    // Draw the objects in the scene
//    gl::pushModelMatrix();
    gl::color( ColorA( 1.0f, 1.0f, 1.0f, 1.0f ) );
    for (int i=0; i<birds.size(); ++i)
    {
//        gl::ScopedModelMatrix smm;
//        gl::translate(birds[i]);
        mObjects[i]->draw();
    }
//    gl::popModelMatrix();
    
    // Draw Params
    mParams->draw();
}

CINDER_APP( FlockingPolysApp, RendererGl( RendererGl::Options().msaa( 16 ) ) , FlockingPolysApp::prepareSettings )
