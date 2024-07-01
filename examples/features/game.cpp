// Template, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

#include "precomp.h"
#include "game.h"

static Scene scene;
static int legocar;
Buffer* bvhData, *triData, *idxData;

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
void Game::Init()
{
	// anything that happens only once at application start goes here
	legocar = scene.AddMesh( "assets/legocar.obj" );
	scene.UpdateBVH();
	Mesh* mesh = scene.meshPool[0];
	BVH& bvh = mesh->bvh;
	bvhData = new Buffer( bvh.nodesUsed * sizeof( BVH::BVHNode ), bvh.bvhNode );
	triData = new Buffer( bvh.triCount * sizeof( float4 ) * 3, bvh.tris );
	idxData = new Buffer( bvh.triCount * sizeof( uint ), bvh.triIdx );
	bvhData->CopyToDevice();
	triData->CopyToDevice();
	idxData->CopyToDevice();
}

// -----------------------------------------------------------
// Main application tick function - Executed once per frame
// -----------------------------------------------------------
void Game::Tick( float /* deltaTime */ )
{
	// NOTE: clear this function before actual use; code is only for 
	// demonstration purposes. See _ getting started.pdf for details.

	// clear the screen to black
	screen->Clear( 0xff );

	//  draw a sprite
	static Sprite rotatingGun( new Surface( "assets/aagun.tga" ), 36 );
	static int frame = 0;
	rotatingGun.SetFrame( ++frame % 36 );
	rotatingGun.Draw( screen, 800, 150 );

	// print something to the console window
	printf( "frame: %i, hello world!\n", frame );

	// print something to the graphics window
	screen->Print( "cpu code", 210, 365, 0xffffff );
	screen->Print( "gpu code", 510, 365, 0xffffff );

	// plot some colors
	for (int x = 0; x < 256; x++) for (int y = 0; y < 256; y++)
	{
		int red = (x & 15) << 4, green = (y & 15) << 4;
		screen->Plot( x + 200, y + 100, (red << 16) + (green << 8) );
	}

	// run some OpenCL code
	static Kernel kernel( "cl/kernels.cl", "render" );
	static Surface image( 256, 256 );
	static Buffer buffer( 256 * 256 * 4, image.pixels );
	kernel.SetArguments( &buffer, frame );
	kernel.Run( 256 * 256 );
	buffer.CopyFromDevice();
	image.CopyTo( screen, 500, 100 );

	// trace on CPU
	Mesh* mesh = scene.meshPool[0];
	for( int y = 0; y < 256; y++ ) for( int x = 0; x < 256; x++ )
	{
		BVH::Ray r( float3( x - 128.0f, 128.0f - y, -500.0f ) * 0.001f, float3( 0, 0, 1 ) );
		mesh->bvh.Intersect( r );
		screen->Plot( x + 800, y + 100, r.hit.t < 1e30f ? 0xffffff : 0 );
	}

	// some random math
	float3 position( 100, 100, 100 );
	float3 vector( 0, 0, 1 );
	mat4 matrix = mat4::RotateX( 2 /* radians */ );
	float3 new_position = (position + vector) * matrix;
	float3 vector2( 1, 0, 0 );
	float3 up = cross( vector, vector2 );

	// trace on the GPU
	static Kernel tracer( kernel.GetProgram(), "trace" );
	tracer.SetArguments( &buffer, triData, bvhData, idxData );
	tracer.Run2D( int2( 256, 256 ) );
	buffer.CopyFromDevice();
	image.CopyTo( screen, 800, 400 );
}