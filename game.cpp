// Template, IGAD version 3
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/UU - Jacco Bikker - 2006-2023

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
	screen->Clear( 0 );
	// print something to the console window
	printf( "hello world!\n" );
	// plot some colors
	for (int red = 0; red < 256; red++) for (int green = 0; green < 256; green++)
	{
		int x = red, y = green;
		screen->Plot( x + 200, y + 100, (red << 16) + (green << 8) );
	}
	// plot a white pixel in the bottom right corner
	screen->Plot( SCRWIDTH - 2, SCRHEIGHT - 2, 0xffffff );
	//  draw a sprite
	static Sprite rotatingGun( new Surface( "assets/aagun.tga" ), 36 );
	static int frame = 0;
	rotatingGun.SetFrame( frame );
	rotatingGun.Draw( screen, SCRWIDTH - 20, 1 );
	rotatingGun.Draw( screen, SCRWIDTH - 35, 50 );
	rotatingGun.Draw( screen, SCRWIDTH - 50, 100 );
	screen->Bar( SCRWIDTH - 50, 50, SCRWIDTH + 50, 300, 0xff0000 );
	if (++frame == 36) frame = 0;
	Sleep( 50 ); // otherwise it will be too fast!
}