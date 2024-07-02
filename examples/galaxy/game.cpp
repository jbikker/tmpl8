// Template, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

#include "precomp.h"
#include "game.h"

#define DOTS 50000 // target: 50000 @ 30fps.

float rx = 0, ry = 0, rz = 0;
float elapsed1, elapsed2, elapsed3, elapsed4;
Timer timer;

//-----------------------------------------------------------
// Game::Sort
// Sort the dots according to depth (QuickSort)
//-----------------------------------------------------------
void Swap( float3& a, float3& b ) { float3 t = a; a = b; b = t; }
void Game::Sort( float3 a[], int first, int last )
{
	if (first >= last) return;
	int p = first;
	float3 e = a[first];
	for (int i = first + 1; i <= last; i++) if (a[i].z > e.z) Swap( a[i], a[++p] );
	Swap( a[p], a[first] );
	Sort( a, first, p - 1 );
	Sort( a, p + 1, last );
}

//-----------------------------------------------------------
// Game::Init
// Initialize the application
//-----------------------------------------------------------
void Game::Init()
{
	m_Dot = new Sprite( new Surface( "assets/dot.png" ), 4 );
	// m_Dot->SetFlags( Sprite::FLARE );
	m_Points = new float3[DOTS];
	m_Rotated = new float3[DOTS];
	for (int i = 0; i < DOTS; i++)
	{
		float a = (floor( Rand( 36 ) ) + powf( Rand( 1 ), 8.0f )) * 10.0f, // arm
			d = powf( Rand( 1 ), 4.0f ), // distance along arm
			u = d * sinf( a * PI / 180 ), // point on arm
			v = d * cosf( a * PI / 180 ),
			x = u * cosf( d * -2.0f ) + v * sinf( d * -2.0f ), // swirl
			z = u * sinf( d * -2.0f ) - v * cosf( d * -2.0f ),
			y = (Rand( 2 ) - 1.0f) * 0.1f * (1 - d); // thickness
		m_Points[i] = float3( x, y, z );
	}
}

//-----------------------------------------------------------
// Game::Transform
// Rotate the dots around the origin using matrix m
//-----------------------------------------------------------
void Game::Transform()
{
	mat4 m;
	m = mat4::RotateY( -ry ) * mat4::RotateX( 0.3f );
	ry += 0.00035f, rz += 0.001f;
	if (rx > 360) rx -= 360;
	if (ry > 360) ry -= 360;
	if (rz > 360) rz -= 360;
	for (int i = 0; i < DOTS; i++)
	{
		m_Rotated[i] = float4( m_Points[i], 1 ) * m;
		// HACK: encode dot index in rotated z coordinate
		uint* hack = (uint*)&m_Rotated[i].z;
		*hack = (*hack & -4) + i;
	}
}

//-----------------------------------------------------------
// Game::Render
// Draw the dots to the window surface
//-----------------------------------------------------------
void Game::Render()
{
	timer.reset(); screen->Clear( 0 ); elapsed4 = timer.elapsed();
	int i = 0;
	for (; i < DOTS / 10; i++)
	{
		// extract dot index from sorted z-coordinate
		uint dotIdx = *(uint*)&m_Rotated[i].z & 3;
		// draw scaled sprite
		float sx = (m_Rotated[i].x * 3000.0f / (m_Rotated[i].z + 5.0f)) + (SCRWIDTH / 2);
		float sy = (m_Rotated[i].y * 3000.0f / (m_Rotated[i].z + 5.0f)) + (SCRHEIGHT / 2);
		float size = 170.0f / (m_Rotated[i].z + 5.0f);
		m_Dot->SetFrame( dotIdx );
		m_Dot->DrawScaledAdditiveSubpixel( sx - size / 2, sy - size / 2, size, size, screen );
	}
	for(; i < DOTS; i++)
	{
		// draw subpixel particle
		float sx = (m_Rotated[i].x * 3000.0f / (m_Rotated[i].z + 5.0f)) + (SCRWIDTH / 2);
		float sy = (m_Rotated[i].y * 3000.0f / (m_Rotated[i].z + 5.0f)) + (SCRHEIGHT / 2);
		screen->PlotSubpixel( sx, sy, 0xffffff );
	}
}
//-----------------------------------------------------------
// Game::Tick
// Main function of application
//-----------------------------------------------------------
void Game::Tick( float )
{

	timer.reset(); Transform(); elapsed1 = timer.elapsed();
	timer.reset(); Sort( m_Rotated, 0, DOTS / 10 - 1 ); Sort( m_Rotated, DOTS / 10, DOTS - 1 ); elapsed2 = timer.elapsed();
	timer.reset(); Render(); elapsed3 = timer.elapsed();

	char t[200];
	sprintf( t, "transform: %7.4f", elapsed1 );
	screen->Print( t, 2, 2, 0x11ffffff );
	sprintf( t, "sort:      %7.4f", elapsed2 );
	screen->Print( t, 2, 12, 0x11ffffff );
	sprintf( t, "render:    %7.4f", elapsed3 );
	screen->Print( t, 2, 22, 0x11ffffff );
	sprintf( t, "clear:     %7.4f", elapsed4 );
	screen->Print( t, 2, 32, 0x11ffffff );
}