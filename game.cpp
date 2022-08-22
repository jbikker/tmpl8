#include "precomp.h"
#include "game.h"

TheApp* CreateApp() { return new Game(); }

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
void Game::Tick( float deltaTime )
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
	// draw a sprite
	static Sprite rotatingGun( new Surface( "assets/aagun.tga" ), 36 );
	static int frame = 0;
	rotatingGun.SetFrame( frame );
	rotatingGun.Draw( screen, 100, 100 );
	if (++frame == 36) frame = 0;
	Sleep( 50 ); // otherwise it will be too fast!
}