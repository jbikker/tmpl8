// Template, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

#include "precomp.h"
#include "game.h"

// -----------------------------------------------------------
// Double-buffered bit pattern definition and access
// -----------------------------------------------------------
uint* pattern = 0, *second, pw = 0, ph = 0;
void BitSet( uint x, uint y ) { pattern[y * pw + (x >> 5)] |= 1 << (x & 31); }
int GetBit( uint x, uint y ) { return (second[y * pw + (x >> 5)] >> (x & 31)) & 1; }

// -----------------------------------------------------------
// Minimal loader for Golly .rle files
// -----------------------------------------------------------
void RLELoader( char* fileName )
{
	FILE* f = fopen( fileName, "r" );
	if (!f) return;
	char buffer[1024], *p, c;
	int state = 0, n = 0, x = 0, y = 0;
	while (!feof( f ))
	{
		buffer[0] = 0;
		fgets( buffer, 1023, f );
		if (*(p = buffer) == '#') continue; else if (*p == 'x')
		{
			if (sscanf( buffer, "x = %i, y = %i", &pw, &ph ) != 2) return; // bad format
			pw = (pw + 31) / 32, pattern = (uint*)calloc( pw * ph, 4 );
			continue;
		}
		while (1)
		{
			if ((c = *p++) < 32) break; // load next line
			if (state == 0) if (c < '0' || c > '9') state = 1, n = max( n, 1 ); else n = n * 10 + (c - '0');
			if (state == 1) // expect other character
			{
				if (c == '$') y += n, x = 0; // newline
				else if (c == 'o') for( int i = 0; i < n; i++ ) BitSet( x++, y ); else if (c == 'b') x += n;
				state = n = 0;
			}
		}
	}
}

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
void Game::Init()
{
	// load the pattern
	RLELoader( "assets/turing_js_r.rle" );				// 1,728 x 1,647
	second = (uint*)calloc( pw * ph, 4 );
}

// -----------------------------------------------------------
// Advance the pattern
// -----------------------------------------------------------
static uint result[8] = { 0, 0, 0xffffff, 0xffffff, 0, 0, 0, 0 };
void Game::Simulate()
{
	// swap source and target pattern image
	uint* tmp = pattern; pattern = second; second = tmp;
	memset( pattern, 0, pw * ph * 4 );
	// process all pixels, skipping one pixel boundary
	const int w = pw * 32, h = ph;
	for( int y = 1; y < h - 1; y++ ) for( int x = 1; x < w - 1; x++ )
	{
		// count active neighbors
		int n = GetBit( x - 1, y - 1 ) + GetBit( x, y - 1 ) + GetBit( x + 1, y - 1 ) + GetBit( x - 1, y ) + 
				GetBit( x + 1, y ) + GetBit( x - 1, y + 1 ) + GetBit( x, y + 1 ) + GetBit( x + 1, y + 1 );
		if ((GetBit( x, y ) && n ==2) || n == 3) BitSet( x, y );
	}
}

// -----------------------------------------------------------
// Handle mouse dragging
// -----------------------------------------------------------
void Game::HandleInput()
{
	if (!GetAsyncKeyState( VK_LBUTTON )) mouseDown = false; else
	{
		POINT p;
		GetCursorPos( &p );
		if (mouseDown)
		{
			offsetx = offset0x + (dragx - p.x), offsety = offset0y + (dragy - p.y);
			offsetx = min( (int)pw * 32 - SCRWIDTH, max( 0, offsetx ) );
			offsety = min( (int)ph - SCRHEIGHT, max( 0, offsety ) );
			return;
		}
		offset0x = offsetx, offset0y = offsety, dragx = p.x, dragy = p.y;
		mouseDown = true;
	}
}

// -----------------------------------------------------------
// Main application tick function
// -----------------------------------------------------------
void Game::Tick( float deltaTime )
{
	// advance the simulation
	Simulate();
	// handle input
	HandleInput();
	// display the current state
	uint* a = screen->pixels;
	for( int y = 0; y < SCRHEIGHT; y++ ) for( int x = 0; x < SCRWIDTH; x++ ) 
		*a++ = GetBit( x + offsetx, y + offsety ) ? 0xffffff : 0;
}