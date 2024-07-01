// Template, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

#include "precomp.h"
#include "game.h"

#define HOLES			8
#define SPAWNPERFRAME	32
#define PARTICLES		655360

static bool started = false;
static int particles = 0, delay = 1;
static float r = 2, fps = 0;
static Timer fpstimer, functimer;

BlackHole::BlackHole( float px, float py, float pg )
{
	ox = px * SCRWIDTH, oy = py * SCRHEIGHT, g = pg;
}

void Game::UpdateBlackHoles()
{
	float hw = SCRWIDTH / 2, hh = SCRHEIGHT / 2;
	for ( unsigned int i = 0; i < HOLES; i++ )
	{
		float hcx = m_Hole[i]->ox - hw, hcy = m_Hole[i]->oy - hh;
		m_Hole[i]->x = hcx * sin( r ) + hcy * cos( r ) + hw;
		m_Hole[i]->y = hcx * cos( r ) - hcy * sin( r ) + hh;
		r += 0.0001f;
		if (r > (PI * 2)) r -= PI * 2;
	}
}

void Game::BuildBackdrop()
{
	uint* dst = screen->pixels;
	float fy = 0;
	for ( unsigned int y = 0; y < SCRHEIGHT; y++ )
	{
		float fx = 0;
		for ( unsigned int x = 0; x < SCRWIDTH; x++ )
		{
			float g = 0;
			for ( unsigned int i = 0; i < HOLES; i++ )
			{
				float dx = m_Hole[i]->x - fx, dy = m_Hole[i]->y - fy;
				float squareddist = ( dx * dx + dy * dy );
				g += (250.0f * m_Hole[i]->g) / squareddist;
			}	
			if (g > 1) g = 0;
			*dst++ = (int)(g * 255.0f);
			fx++;
		}
		fy++;
	}
}

void Game::Init()
{
	m_Particle = new Particle*[PARTICLES];
	m_Hole = new BlackHole*[HOLES];
	m_Hole[0] = new BlackHole( 0.2f, 0.3f, 1.0f );
	m_Hole[1] = new BlackHole( 0.5f, 0.5f, 3.0f );
	m_Hole[2] = new BlackHole( 0.7f, 0.6f, 0.5f );
	m_Hole[3] = new BlackHole( 0.4f, 0.9f, 1.8f );
	m_Hole[4] = new BlackHole( 0.25f, 0.8f, 0.8f );
	m_Hole[5] = new BlackHole( 0.9f, 0.3f, 0.5f );
	m_Hole[6] = new BlackHole( 0.7f, 0.4f, 0.8f );
	m_Hole[7] = new BlackHole( 0.8f, 0.1f, 0.9f );
	for ( unsigned int i = 0; i < PARTICLES; i++ )
	{
		m_Particle[i] = new Particle();
		if (i & 1)
		{
			m_Particle[i]->m = 0.7f;
			m_Particle[i]->c = (255 << 16) + (255 << 8);
		}
		else
		{
			m_Particle[i]->m = 1.5f;
			m_Particle[i]->c = (255 << 16) + 50 + (50 << 8);
		}
	}
	fpstimer.reset();
}

void Game::SpawnParticle( int n )
{
	m_Particle[n]->x = 0;
	m_Particle[n]->y = SCRHEIGHT * (0.8f + Rand( 0.02f ));
	m_Particle[n]->vx = 0.95f + Rand( 0.1f );
	m_Particle[n]->vy = -0.05f + Rand( 0.1f );
}

void Game::UpdateParticles()
{
	for ( unsigned int i = 0; i < PARTICLES; i++ ) if (m_Particle[i]->alive)
	{
		m_Particle[i]->x += m_Particle[i]->vx;
		m_Particle[i]->y += m_Particle[i]->vy;
		if (!((m_Particle[i]->x < (2 * SCRWIDTH)) && (m_Particle[i]->x > -SCRWIDTH) &&
			  (m_Particle[i]->y < (2 * SCRHEIGHT)) && (m_Particle[i]->y > -SCRHEIGHT)))
		{
			SpawnParticle( i );
			continue;
		}
		for ( unsigned int h = 0; h < HOLES; h++ )
		{
			float dx = m_Hole[h]->x - m_Particle[i]->x;
			float dy = m_Hole[h]->y - m_Particle[i]->y;
			float sd = dx * dx + dy * dy;
			float dist = 1.0f / sqrtf( sd );
			dx *= dist, dy *= dist;
			float g = (250.0f * m_Hole[h]->g * m_Particle[i]->m) / sd;
			if (g >= 1) 
			{
				SpawnParticle( i );
				break;
			}
			m_Particle[i]->vx += 0.5f * g * dx;
			m_Particle[i]->vy += 0.5f * g * dy;
		}				
		int x = (int)m_Particle[i]->x, y = (int)m_Particle[i]->y;
		if ((x >= 0) && (x < SCRWIDTH) && (y >= 0) && (y < SCRHEIGHT))
		{
			uint* dst = screen->pixels;
			dst[x + y * SCRWIDTH] = m_Particle[i]->c;
		}
	}
}

void Game::Tick( float /* deltaTime */ )
{
	// spawn particles
	if (started) for ( unsigned int i = 0; i < SPAWNPERFRAME; i++ )
	{
		if (particles < PARTICLES) 
		{
			SpawnParticle( particles );
			m_Particle[particles++]->alive = true;
		}
	}
	if (GetAsyncKeyState( 32 )) started = true;
	
	// get accurate frame rate
	if (!--delay)
	{
		delay = 8;
		float elapsed = fpstimer.elapsed();
		fpstimer.reset();
		fps = 8000.0f / elapsed;
	}
	
	// spin black holes
	functimer.reset();
	UpdateBlackHoles();
	float elapsed1 = functimer.elapsed();
	
	// draw the gravity field
	functimer.reset();
	BuildBackdrop();
	float elapsed2 = functimer.elapsed();

	// draw the particles	
	functimer.reset();
	UpdateParticles();	
	float elapsed3 = functimer.elapsed();
	
	// statistics
	char t[128];
	sprintf( t, "updateblackholes  %06.3f ms", elapsed1 );
	screen->Print( t, 2, 2, 0xffffff );
	sprintf( t, "buildbackdrop     %06.3f ms", elapsed2 );
	screen->Print( t, 2, 12, 0xffffff );
	sprintf( t, "updateparticles   %06.3f ms", elapsed3 );
	screen->Print( t, 2, 22, 0xffffff );
	sprintf( t, "%i particles", particles );
	screen->Print( t, 2, 32, 0xffffff );
	sprintf( t, "fps %6.2f", fps );
	screen->Print( t, 2, 42, 0xff9999 );
}