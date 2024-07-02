// Template, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

#include "precomp.h"
#include "game.h"

// global data (source scope)
static Game* game;
// static Font font( "assets/digital_small.png", "ABCDEFGHIJKLMNOPQRSTUVWXYZ:?!=-0123456789." );
static bool lock = false;
static Timer stopwatch;
static float duration;

// mountain peaks (push player away)
static float peakx[16] = { 496, 1074, 1390, 1734, 1774, 426, 752, 960, 1366, 1968, 728, 154, 170, 1044, 828, 1712 };
static float peaky[16] = { 398, 446, 166, 748, 1388, 1278, 938, 736, 1090, 290, 126, 82, 784, 570, 894, 704 };
static float peakh[16] = { 400, 300, 320, 510, 400, 510, 400, 600, 240, 200, 160, 160, 160, 320, 320, 320 };

// player, bullet and smoke data
static int aliveP1 = MAXP1, aliveP2 = MAXP2, frame = 0;
static Bullet bullet[MAXBULLET];

// dust particle effect tick function
void Particle::Tick()
{
	pos += vel;
	if (pos.x < 0) pos.x = 2046, pos.y = (float)((idx * 20261) % 1534); // adhoc rng
	if (pos.y < 0) pos.y = 1534;
	if (pos.x > 2046) pos.x = 0;
	if (pos.y > 1534) pos.y = 0;
	float2 force = normalize( float2( -1.0f + vel.x, -0.1f + vel.y ) ) * speed;
	uint* heights = game->heights->pixels;
	int ix = min( 1022, (int)pos.x / 2 ), iy = min( 766, (int)pos.y / 2 );
	float heightDeltaX = (float)(heights[ix + iy * 1024] & 255) - (heights[(ix + 1) + iy * 1024] & 255);
	float heightDeltaY = (float)(heights[ix + iy * 1024] & 255) - (heights[ix + (iy + 1) * 1024] & 255);
	float3 N = normalize( float3( heightDeltaX, heightDeltaY, 38 ) ) * 4.0f;
	vel.x = force.x + N.x, vel.y = force.y + N.y;
	uint* a = game->canvas->pixels + (int)pos.x + (int)pos.y * 2048;
	a[0] = AddBlend( a[0], 0x221100 ), a[2048] = AddBlend( a[2048], 0x221100 );
	a[1] = AddBlend( a[1], 0x221100 ), a[2049] = AddBlend( a[2049], 0x221100 );
}

// smoke particle effect tick function
void Smoke::Tick()
{
	unsigned int p = frame >> 3;
	if (frame < 64) if (!(frame++ & 7)) puff[p].x = xpos, puff[p].y = ypos << 8, puff[p].vy = -450, puff[p].life = 63;
	for (unsigned int i = 0; i < p; i++) if ((frame < 64) || (i & 1))
	{
		puff[i].x++, puff[i].y += puff[i].vy, puff[i].vy += 3;
		int f = (puff[i].life > 13) ? (9 - (puff[i].life - 14) / 5) : (puff[i].life / 2);
		game->smoke->SetFrame( f );
		game->smoke->Draw( game->canvas, puff[i].x - 12, (puff[i].y >> 8) - 12 );
		if (!--puff[i].life) puff[i].x = xpos, puff[i].y = ypos << 8, puff[i].vy = -450, puff[i].life = 63;
	}
}

// bullet Tick function
void Bullet::Tick()
{
	if (!(flags & Bullet::ACTIVE)) return;
	float2 prevpos = pos;
	pos += speed * 1.5f, prevpos -= pos - prevpos;
	game->canvas->Line( prevpos.x, prevpos.y, pos.x, pos.y, 0x555555 );
	if ((pos.x < 0) || (pos.x > 2047) || (pos.y < 0) || (pos.y > 1535)) flags = 0; // off-screen
	unsigned int start = 0, end = MAXP1;
	if (flags & P1) start = MAXP1, end = MAXP1 + MAXP2;
	for (unsigned int i = start; i < end; i++) // check all opponents
	{
		Tank& t = game->tankPrev[i];
		if (!((t.flags & Tank::ACTIVE) && (pos.x >( t.pos.x - 2 )) && (pos.y > (t.pos.y - 2)) && (pos.x < (t.pos.x + 2)) && (pos.y < (t.pos.y + 2)))) continue;
		game->tank[i].health = max( 0.0f, game->tank[i].health - (Rand( 0.3f ) + 0.1f) );
		if (game->tank[i].health > 0) continue;
		if (t.flags & Tank::P1) aliveP1--; else aliveP2--;	// update counters
		game->tank[i].flags &= Tank::P1 | Tank::P2;			// kill tank
		flags = 0;											// destroy bullet
		break;
	}
}

// Tank::Fire - spawns a bullet
void Tank::Fire( unsigned int party, float2& p, float2& )
{
	for (unsigned int i = 0; i < MAXBULLET; i++) if (!(bullet[i].flags & Bullet::ACTIVE))
	{
		bullet[i].flags |= Bullet::ACTIVE + party; // set owner, set active
		bullet[i].pos = p, bullet[i].speed = speed;
		break;
	}
}

// Tank::Tick - update single tank
void Tank::Tick()
{
	if (!(flags & ACTIVE)) // dead tank
	{
		smoke.xpos = (int)pos.x, smoke.ypos = (int)pos.y;
		return smoke.Tick();
	}
	float2 force = normalize( target - pos );
	// evade mountain peaks
	for (unsigned int i = 0; i < 16; i++)
	{
		float2 d( pos.x - peakx[i], pos.y - peaky[i] );
		float sd = (d.x * d.x + d.y * d.y) * 0.2f;
		if (sd < 1500)
		{
			force += d * 0.03f * (peakh[i] / sd);
			float r = sqrtf( sd );
			for (int j = 0; j < 720; j++)
			{
				float x = peakx[i] + r * sinf( (float)j * PI / 360.0f );
				float y = peaky[i] + r * cosf( (float)j * PI / 360.0f );
				game->canvas->Plot( (int)x, (int)y, 0x000500 );
			}
		}
	}
	// evade other tanks
	for (unsigned int i = 0; i < (MAXP1 + MAXP2); i++)
	{
		if (&game->tank[i] == this) continue;
		float2 d = pos - game->tankPrev[i].pos;
		if (length( d ) < 8) force += normalize( d ) * 2.0f;
		else if (length( d ) < 16) force += normalize( d ) * 0.4f;
	}
	// evade user dragged line
	if ((flags & P1) && (game->leftButton))
	{
		float x1 = (float)game->dragXStart, y1 = (float)game->dragYStart;
		float x2 = (float)game->mousex, y2 = (float)game->mousey;
		if (x1 != x2 || y1 != y2)
		{
			float2 N = normalize( float2( y2 - y1, x1 - x2 ) );
			float dist = dot( N, pos ) - dot( N, float2( x1, y1 ) );
			if (fabs( dist ) < 10) if (dist > 0) force += N * 20; else force -= N * 20;
		}
	}
	// update speed using accumulated force
	speed += force, speed = normalize( speed ), pos += speed * maxspeed * 0.5f;
	// shoot, if reloading completed
	if (--reloading >= 0) return;
	unsigned int start = 0, end = MAXP1;
	if (flags & P1) start = MAXP1, end = MAXP1 + MAXP2;
	for (unsigned int i = start; i < end; i++) if (game->tankPrev[i].flags & ACTIVE)
	{
		float2 d = game->tankPrev[i].pos - pos;
		if (length( d ) < 100 && dot( speed, normalize( d ) ) > 0.99999f)
		{
			Fire( flags & (P1 | P2), pos, speed ); // shoot
			reloading = 200; // and wait before next shot is ready
			break;
		}
	}
}

// Game::Init - Load data, setup playfield
void Game::Init()
{
	// load assets
	backdrop = new Surface( "assets/backdrop.png" );
	heights = new Surface( "assets/heightmap.png" );
	canvas = new Surface( 2048, 1536 ); // game runs on a double-sized surface
	tank = new Tank[MAXP1 + MAXP2];
	tankPrev = new Tank[MAXP1 + MAXP2];
	p1Sprite = new Sprite( new Surface( "assets/p1tank.tga" ), 1 /*, Sprite::FLARE */ );
	p2Sprite = new Sprite( new Surface( "assets/p2tank.tga" ), 1 /*, Sprite::FLARE */ );
	pXSprite = new Sprite( new Surface( "assets/deadtank.tga" ), 1 /*, Sprite::BLACKFLARE */ );
	smoke = new Sprite( new Surface( "assets/smoke.tga" ), 10 /*, Sprite::FLARE */ );
	// create blue tanks
	for (unsigned int i = 0; i < MAXP1; i++)
	{
		Tank& t = tank[i];
		t.pos = float2( (float)((i % 20) * 20) + P1STARTX, (float)((i / 20) * 20 + 50) + P1STARTY );
		t.target = float2( 2048, 1536 ); // initially move to bottom right corner
		t.speed = float2( 0, 0 ), t.flags = Tank::ACTIVE | Tank::P1, t.maxspeed = (i < (MAXP1 / 2)) ? 0.65f : 0.45f;
		t.health = 1.0f;
	}
	// create red tanks
	for (unsigned int i = 0; i < MAXP2; i++)
	{
		Tank& t = tank[i + MAXP1];
		t.pos = float2( (float)((i % 32) * 20 + P2STARTX), (float)((i / 32) * 20 + P2STARTY) );
		t.target = float2( 424, 336 ); // move to player base
		t.speed = float2( 0, 0 ), t.flags = Tank::ACTIVE | Tank::P2, t.maxspeed = 0.3f;
	}
	// create dust particles
	particle = new Particle[DUSTPARTICLES];
	srand( 0 );
	for (int i = 0; i < DUSTPARTICLES; i++)
	{
		particle[i].pos.x = Rand( 2048 );
		particle[i].pos.y = Rand( 1536 );
		particle[i].idx = i;
		particle[i].speed = Rand( 2 ) + 0.5f;
		particle[i].vel.x = particle[i].vel.y = 0;
	}
	game = this; // for global reference
	leftButton = prevButton = false;
}

// Game::DrawDeadTanks - draw dead tanks
void Game::DrawDeadTanks()
{
	for (unsigned int i = 0; i < (MAXP1 + MAXP2); i++)
	{
		Tank& t = tank[i];
		float x = t.pos.x, y = t.pos.y;
		if (!(t.flags & Tank::ACTIVE)) pXSprite->Draw( canvas, (int)x - 4, (int)y - 4);
	}
}

// Game::DrawTanks - draw the tanks
void Game::DrawTanks()
{
	for (unsigned int i = 0; i < (MAXP1 + MAXP2); i++)
	{
		Tank& t = tank[i];
		float x = t.pos.x, y = t.pos.y;
		float2 p1( x + 70 * t.speed.x + 22 * t.speed.y, y + 70 * t.speed.y - 22 * t.speed.x );
		float2 p2( x + 70 * t.speed.x - 22 * t.speed.y, y + 70 * t.speed.y + 22 * t.speed.x );
		if (!(t.flags & Tank::ACTIVE)) continue;
		else if (t.flags & Tank::P1) // draw blue tank
		{
			p1Sprite->Draw( canvas, (int)x - 4, (int)y - 4 );
			canvas->Line( (int)x, (int)y, (int)(x + 8 * t.speed.x), (int)(y + 8 * t.speed.y), 0x6666ff );
			if (tank[i].health > 0.9f) canvas->Line( (int)x - 4, (int)y - 12, (int)x + 4, (int)y - 12, 0x33ff33 );
			else if (tank[i].health > 0.6f) canvas->Line( (int)x - 4, (int)y - 12, (int)x + 2, (int)y - 12, 0x33ff33 );
			else if (tank[i].health > 0.3f) canvas->Line( (int)x - 4, (int)y - 12, (int)x - 1, (int)y - 12, 0xdddd33 );
			else if (tank[i].health > 0) canvas->Line( (int)x - 4, (int)y - 12, (int)x - 3, (int)y - 12, 0xff3333 );
		}
		else // draw red tank
		{
			p2Sprite->Draw( canvas, (int)x - 4, (int)y - 4 );
			canvas->Line( (int)x, (int)y, (int)(x + 8 * t.speed.x), (int)(y + 8 * t.speed.y), 0xff4444 );
		}
		// tread marks
		if ((x >= 0) && (x < 2048) && (y >= 0) && (y < 1536))
			backdrop->pixels[(int)x + (int)y * 2048] = SubBlend( backdrop->pixels[(int)x + (int)y * 2048], 0x030303 );
	}
}

// Game::PlayerInput - handle player input
void Game::PlayerInput()
{
	if (leftButton) // drag line to guide player tanks
	{
		if (!prevButton) dragXStart = mousex, dragYStart = mousey, dragFrames = 0; // start line
		canvas->Line( dragXStart, dragYStart, mousex, mousey, 0xffffff );
		dragFrames++;
	}
	else // select a new destination for the player tanks
	{
		if ((prevButton) && (dragFrames < 15)) for (unsigned int i = 0; i < MAXP1; i++) tank[i].target = float2( (float)mousex, (float)mousey );
		canvas->Line( 0, (float)mousey, 2047, (float)mousey, 0xffffff );
		canvas->Line( (float)mousex, 0, (float)mousex, 1535, 0xffffff );
	}
	prevButton = leftButton;
}

// Game::MeasurementStuff: functionality related to final performance measurement
void Game::MeasurementStuff()
{
#ifdef MEASURE
	char buffer[128];
	if (frame & 64) screen->Bar( 980, 20, 1010, 50, 0xff0000 );
	if (frame >= MAXFRAMES) if (!lock) 
	{
		duration = stopwatch.elapsed();
		lock = true;
	}
	else frame--;
	if (lock)
	{
		int ms = (int)duration % 1000, sec = ((int)duration / 1000) % 60, min = ((int)duration / 60000);
		sprintf( buffer, "%02i:%02i:%03i", min, sec, ms );
		// font.Centre( screen, buffer, 200 );
		sprintf( buffer, "SPEEDUP: %4.1f", REFPERF / duration );
		// font.Centre( screen, buffer, 340 );
	}
#endif
}

// Game::Tick - main game loop
void Game::Tick( float )
{
	// get current mouse position relative to full-size bitmap
	POINT p;
#ifdef MEASURE
	p.x = 512 + (int)(400 * sinf( (float)frame * 0.02f )), p.y = 384;
#else
	GetCursorPos( &p );
	ScreenToClient( FindWindow( NULL, "Tmpl8-2024" ), &p );
	leftButton = (GetAsyncKeyState( VK_LBUTTON ) != 0);
#endif
	mousex = p.x * 2, mousey = p.y * 2;
	// draw backdrop
	backdrop->CopyTo( canvas, 0, 0 );
	// update dust particles
	for (int i = 0; i < DUSTPARTICLES; i++) particle[i].Tick();
	// draw armies
	DrawDeadTanks();
	DrawTanks();
	// update armies
	memcpy( tankPrev, tank, (MAXP1 + MAXP2) * sizeof( Tank ) );
	if (!lock) for (unsigned int i = 0; i < (MAXP1 + MAXP2); i++) tank[i].Tick();
	// update bullets
	if (!lock) for (unsigned int i = 0; i < MAXBULLET; i++) bullet[i].Tick();
	// handle input
#ifndef MEASURE
	PlayerInput();
#endif
	// scale to window size
	canvas->CopyHalfSize( screen );
	// draw lens
	for (int x = p.x - 80; x < p.x + 80; x++) for (int y = p.y - 80; y < p.y + 80; y++)
	{
		if (sqrtf( (float)(x - p.x) * (x - p.x) + (y - p.y) * (y - p.y) ) > 80.0f) continue;
		int rx = mousex + x - p.x, ry = mousey + y - p.y;
		if (rx > 0 && ry > 0 && x > 0 && y > 0 && rx < 2048 && x < 1024 && ry < 1536 && y < 768)
			screen->Plot( x, y, canvas->pixels[rx + ry * 2048] );
	}
	// report on game state
	MeasurementStuff();
	char buffer[128];
	sprintf( buffer, "FRAME: %04i", frame++ );
	// font.Print( screen, buffer, 350, 580 );
	if ((aliveP1 > 0) && (aliveP2 > 0))
	{
		sprintf( buffer, "blue army: %03i  red army: %03i", aliveP1, aliveP2 );
		return screen->Print( buffer, 10, 10, 0xffff00 );
	}
	if (aliveP1 == 0)
	{
		sprintf( buffer, "sad, you lose... red left: %i", aliveP2 );
		return screen->Print( buffer, 200, 370, 0xffff00 );
	}
	sprintf( buffer, "nice, you win! blue left: %i", aliveP1 );
	screen->Print( buffer, 200, 370, 0xffff00 );
}