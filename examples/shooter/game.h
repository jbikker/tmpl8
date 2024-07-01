// Template, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

#pragma once

#define STARS		500
#define MAXACTORS	1000

namespace Tmpl8
{

class Surface;
class Sprite;

class Actor
{
public:
	enum
	{
		UNDEFINED = 0,
		METALBALL = 1,
		PLAYER = 2,
		ENEMY = 3,
		BULLET = 4
	};
	virtual bool Tick() = 0;
	virtual bool Hit( float&, float&, float&, float& ) { return false; }
	virtual int GetType() { return Actor::UNDEFINED; }
	static void SetSurface( Surface* a_Surface ) { surface = a_Surface; }
	static Surface* surface;
	Sprite* m_Sprite;
	static Sprite* m_Spark;
	float x, y;
};

class MetalBall;
class ActorPool
{
public:
	ActorPool() { pool = new Actor*[MAXACTORS]; actors = 0; }
	static void Tick() 
	{ 
		for ( int i = 0; i < actors; i++ ) 
		{
			Actor* actor = pool[i];
			if (!actor->Tick()) delete actor;
		}
	}
	static void Add( Actor* a_Actor ) { pool[actors++] = a_Actor; }
	static void Delete( Actor* actor )
	{
		for ( int i = 0; i < actors; i++ ) if (pool[i] == actor)
		{
			for ( int j = i + 1; j < actors; j++ ) pool[j - 1] = pool[j];
			actors--;
			break;
		}
	}
	static bool CheckHit( float& a_X, float& a_Y, float& a_NX, float& a_NY )
	{
		for ( int i = 0; i < actors; i++ )
			if (pool[i]->Hit( a_X, a_Y, a_NX, a_NY )) return true;
		return false;
	}
	static int GetActiveActors() { return actors; }
	static Actor** pool;
	static int actors;
};

class Surface;
class Starfield : public Actor
{
public:
	Starfield();
	bool Tick();
private:
	float* x, *y;
};

class Bullet : public Actor
{
public:
	Bullet();
	~Bullet();
	enum
	{
		PLAYER = 0,
		ENEMY = 1
	};
	void Init( Surface* a_Surface, float a_X, float a_Y, float a_VX, float a_VY, int a_Owner )
	{
		x = a_X, y = a_Y;
		vx = a_VX, vy = a_VY;
		surface = a_Surface;
		life = 1200;
		owner = a_Owner;
	}
	bool Tick();
	int GetType() { return Actor::BULLET; }
	float vx, vy;
	int life, owner;
	Sprite* player, *enemy;
};

class MetalBall : public Actor
{
public:
	MetalBall();
	bool Tick();
	bool Hit( float& a_X, float& a_Y, float& a_NX, float& a_NY );
	int GetType() { return Actor::METALBALL; }
};

class Playership : public Actor
{
public:
	Playership();
	bool Tick();
	int GetType() { return Actor::PLAYER; }
private:
	float vx, vy;
	int btimer, dtimer;
	Sprite* death;
};

class Enemy : public Actor
{
public:
	Enemy();
	bool Tick();
	int GetType() { return Actor::ENEMY; }
private:
	float vx, vy;
	int frame, btimer, dtimer;
	Sprite* death;
};

class Game : public TheApp
{
public:
	// game flow methods
	void Init();
	void Tick( float deltaTime );
	void Shutdown() { /* implement if you want to do something on exit */ }
	void DrawBackdrop();
	void HandleKeys();
	// input handling
	void MouseUp( int ) { /* implement if you want to detect mouse button presses */ }
	void MouseDown( int ) { /* implement if you want to detect mouse button presses */ }
	void MouseMove( int x, int y ) { mousePos.x = x, mousePos.y = y; }
	void MouseWheel( float ) { /* implement if you want to handle the mouse wheel */ }
	void KeyUp( int ) { /* implement if you want to handle keys */ }
	void KeyDown( int ) { /* implement if you want to handle keys */ }
	// data members
	int2 mousePos;
	Sprite* ship;
	ActorPool* actorPool;
	int timer;
};

} // namespace Tmpl8