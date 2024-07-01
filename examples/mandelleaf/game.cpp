#include "precomp.h"
#include "game.h"

static Kernel* testFunction;
static Buffer* outputBuffer;

void Game::Init()
{
	// prepare for OpenCL work, see opencl.cpp
	Kernel::InitCL();
	// load OpenCL code
	testFunction = new Kernel( "cl/program.cl", "fractal" );
	// wrap template rendertarget texture as an OpenCL buffer
	outputBuffer = new Buffer( GetRenderTarget()->ID, 0, Buffer::TARGET );
	screen = 0; // we will fill the template renderTarget texture directly
}

void Game::Tick( float /* deltaTime */ )
{
	// pass arguments to the OpenCL kernel
	static float t = 0, d = 319;
	testFunction->SetArguments( outputBuffer, d, t );
	t += 0.005f; if (t > 1000) t -= 2.0f;
	// run the kernel
	testFunction->Run2D( int2( SCRWIDTH, SCRHEIGHT ) );
}