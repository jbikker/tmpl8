// Template, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

#include "precomp.h"
#include "game.h"

#define GRIDSIZE 128

struct Point { float3 pos, opos, fix; bool fixed; float restlength[4]; };
Point grid[GRIDSIZE][GRIDSIZE];
int xoffset[4] = { 1, -1, 0, 0 }, yoffset[4] = { 0, 0, 1, -1 };

void Game::Init()
{
	for ( int idx, y = 0; y < GRIDSIZE; y++ ) for ( int x = 0; x < GRIDSIZE; x++ )
		idx = x + y * GRIDSIZE,
		grid[y][x].pos.x = 10 + (float)x * ((SCRWIDTH - 100) / GRIDSIZE) + y * 0.9f + Rand( 2 ),
		grid[y][x].pos.y = 10 + (float)y * ((SCRHEIGHT - 120) / GRIDSIZE) + Rand( 2 ),
		grid[y][x].opos = grid[y][x].fix = grid[y][x].pos,
		grid[y][x].fixed = (y == 0);
	for ( int y = 0; y < GRIDSIZE; y++ ) for ( int x = 0; x < GRIDSIZE; x++ ) for ( int c = 0; c < 4; c++ )
		grid[y][x].restlength[c] = length(grid[y][x].pos - grid[y + yoffset[c]][x + xoffset[c]].pos ) * 1.2f;
}

void Game::DrawGrid()
{
	// draw the grid
	screen->Clear( 0 );
	for ( int y = 0; y < (GRIDSIZE - 1); y++ ) for ( int x = 1; x < (GRIDSIZE - 2); x++ )
	{
		float3 p1 = grid[y][x].pos, p2 = grid[y][x + 1].pos, p3 = grid[y + 1][x].pos;
		screen->Line( p1.x, p1.y, p2.x, p2.y, 0xffffff );
		screen->Line( p1.x, p1.y, p3.x, p3.y, 0xffffff );
	}
	for ( int y = 0; y < (GRIDSIZE - 1); y++ )
	{
		float3 p1 = grid[y][GRIDSIZE - 2].pos, p2 = grid[y + 1][GRIDSIZE - 2].pos;
		screen->Line( p1.x, p1.y, p2.x, p2.y, 0xffffff );
	}
}

float magic = 0.92f;
void Game::Simulation()
{
	// verlet integration; apply gravity
	for ( int y = 0; y < GRIDSIZE; y++ ) for ( int x = 0; x < GRIDSIZE; x++ )
	{
		float3 curpos = grid[y][x].pos, prevpos = grid[y][x].opos;
		grid[y][x].pos += (curpos - prevpos) + float3( 0, 0.009f, 0 ); // gravity
		grid[y][x].opos = curpos;
		if (Rand( 10 ) < 0.05f) grid[y][x].pos += float3( Rand( 1.80f + magic ), Rand( 0.19f ), 0 );
	}
	magic += 0.0002f;
	// apply constraints; 4 simulation steps
	for ( int i = 0; i < 6; i++ ) 
	{
		for ( int y = 1; y < GRIDSIZE - 1; y++ ) for ( int x = 1; x < GRIDSIZE - 1; x++ )
		{
			float3 pointpos = grid[y][x].pos;
			// use springs to four neighbouring points
			for ( int linknr = 0; linknr < 4; linknr++ )
			{
				Point* neighbour = &grid[y + yoffset[linknr]][x + xoffset[linknr]];
				float distance = length(neighbour->pos - pointpos);
				if (distance > grid[y][x].restlength[linknr])
				{
					float extra = distance / (grid[y][x].restlength[linknr]) - 1;
					float3 dir = neighbour->pos - pointpos;
					pointpos += extra * dir * 0.5f;
					neighbour->pos -= extra * dir * 0.5f;
				}
			}
			grid[y][x].pos = pointpos;
		}
		for ( int x = 0; x < GRIDSIZE; x++ ) grid[0][x].pos = grid[0][x].fix;
	}
}

void Game::Tick( float )
{
	// update the simulation
	Timer tm;
	tm.reset();
	for( int i = 0; i < 3; i++ ) Simulation();
	float elapsed1 = tm.elapsed();

	// draw the grid
	tm.reset();
	DrawGrid();
	float elapsed2 = tm.elapsed();

	// display statistics
	char t[128];
	sprintf( t, "ye olde ruggeth cloth simulation: %5.1f ms", elapsed1 );
	screen->Print( t, 2, SCRHEIGHT - 24, 0xffffff );
	sprintf( t, "                       rendering: %5.1f ms", elapsed2 );
	screen->Print( t, 2, SCRHEIGHT - 14, 0xffffff );
}