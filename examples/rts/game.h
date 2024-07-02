// Template, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

#pragma once

namespace Tmpl8
{

// final measurement defines
// #define MEASURE										// ENABLE THIS FOR FINAL PERFORMANCE MEASUREMENT
#define REFPERF			(922+52000+3*60000)				// reference performance Jacco's Razer laptop (i7)
// #define REFPERF		(680+31*1000+10*60*1000)		// uncomment and put your reference time here

// modify these values for testing purposes (used when MEASURE is disabled)
#define MAXP1			200			
#define MAXP2			(4 * MAXP1)	// because the player is smarter than the AI
#define MAXBULLET		200
#define P1STARTX		500
#define P1STARTY		300
#define P2STARTX		1300
#define P2STARTY		700
#define DUSTPARTICLES	20000

// standard values for final measurement, do not modify
#define MAXFRAMES		4000
#ifdef MEASURE
#undef MAXP1
#define MAXP1			750				
#undef MAXP2
#define MAXP2			(4 * MAXP1)
#undef P1STARTX
#define P1STARTX		200
#undef P1STARTY
#define P1STARTY		100
#undef P2STARTX
#define P2STARTX		1300
#undef P2STARTY
#define P2STARTY		700
#undef SCRWIDTH
#define SCRWIDTH		1024
#undef SCRHEIGHT
#define SCRHEIGHT		768
#undef DUSTPARTICLES
#define DUSTPARTICLES	10000
#endif

class Particle
{
public:
	void Tick();
	float2 pos, vel, speed;
	int idx;
};

class Smoke
{
public:
	struct Puff { int x, y, vy, life; };
	Smoke() : active( false ), frame( 0 ) {};
	void Tick();
	Puff puff[8];
	bool active;
	int frame, xpos, ypos;
};

class Tank
{
public:
	enum { ACTIVE = 1, P1 = 2, P2 = 4 };
	Tank() : pos( float2( 0, 0 ) ), speed( float2( 0, 0 ) ), target( float2( 0, 0 ) ), reloading( 0 ) {};
	void Fire( unsigned int party, float2& pos, float2& dir );
	void Tick();
	float2 pos, speed, target;
	float maxspeed, health;
	int flags, reloading;
	Smoke smoke;
};

class Bullet
{
public:
	enum { ACTIVE = 1, P1 = 2, P2 = 4 };
	Bullet() : flags( 0 ) {};
	void Tick();
	float2 pos, speed;
	int flags;
};

class Surface;
class Surface8;
class Sprite;
class Game : public TheApp
{
public:
	// game flow methods
	void Init();
	void Tick( float deltaTime );
	void DrawTanks();
	void DrawDeadTanks();
	void PlayerInput();
	void MeasurementStuff();
	void Shutdown() { /* implement if you want to do something on exit */ }
	// input handling
	void MouseUp( int ) { /* implement if you want to detect mouse button presses */ }
	void MouseDown( int ) { /* implement if you want to detect mouse button presses */ }
	void MouseMove( int x, int y ) { mousePos.x = x, mousePos.y = y; }
	void MouseWheel( float ) { /* implement if you want to handle the mouse wheel */ }
	void KeyUp( int ) { /* implement if you want to handle keys */ }
	void KeyDown( int ) { /* implement if you want to handle keys */ }
	// data members
	int2 mousePos;
	Surface* canvas, *backdrop, *heights; 
	Sprite* p1Sprite, *p2Sprite, *pXSprite, *smoke;
	int mousex, mousey, dragXStart, dragYStart, dragFrames;
	bool leftButton, prevButton;
	Tank* tank, *tankPrev;
	Particle* particle;
};

} // namespace Tmpl8