// Template, IGAD version 3
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2023

#include "precomp.h"
#include "game.h"

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
void Game::Init()
{
	// anything that happens only once at application start goes here
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
}