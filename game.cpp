#include "precomp.h" // include (only) this in every .cpp file

void Game::Dummy()
{
	printf( "Task A\n" );	
}

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
void Game::Init()
{
	AddTask( [&](){ this->Dummy(); } );
	AddTask( [](){} );
	RunTasks();
	WaitForAll();
}

// -----------------------------------------------------------
// Close down application
// -----------------------------------------------------------
void Game::Shutdown()
{
}

static Sprite rotatingGun( new Surface( "assets/aagun.tga" ), 36 );
static int frame = 0;

// -----------------------------------------------------------
// Main application tick function
// -----------------------------------------------------------
void Game::Tick( float deltaTime )
{
	// clear the graphics window
	screen->Clear( 0 );
	// print something in the graphics window
	screen->Print( "hello world", 2, 2, 0xffffff );
	// test colors
	screen->Bar( 50, 200, 100, 250, 0xff0000 ); // red
	screen->Bar( 125, 200, 175, 250, 0x00ff00 ); // green
	screen->Bar( 200, 200, 250, 250, 0x0000ff ); // blue
	// draw a sprite
	rotatingGun.SetFrame( frame );
	rotatingGun.Draw( screen, 100, 100 );
	if (++frame == 36) frame = 0;
}
