// Template, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

#include "precomp.h"
#include "game.h"

Actor** ActorPool::pool;
int ActorPool::actors;
ActorPool actorpool;
Surface* Actor::surface;
Sprite* Actor::m_Spark;

Starfield::Starfield()
{ 
	x = new float[STARS], y = new float[STARS]; 
	for ( short i = 0; i < STARS; i++ ) x[i] = Rand( SCRWIDTH ), y[i] = Rand( SCRHEIGHT );
}

bool Starfield::Tick()
{
	for ( short i = 0; i < STARS; i++ )
	{
		if ((x[i] -= ((float)i + 1) / STARS) < 0) x[i] = SCRWIDTH;
		int color = 55 + (int)(((float)i / STARS) * 200.0f);
		surface->Plot( (int)x[i], (int)y[i], color + (color << 8) + (color << 16) );
	}
	return true;
}

MetalBall::MetalBall()
{
	m_Sprite = new Sprite( new Surface( "assets/ball.png" ), 1 );
	while (1)
	{
		x = Rand( SCRWIDTH * 4 ) + SCRWIDTH * 1.2f, y = 10 + Rand( SCRHEIGHT - 70 );
		bool hit = false;
		for ( int i = 2; i < ActorPool::actors; i++ )
		{
			MetalBall* b2 = (MetalBall*)ActorPool::pool[i];
			float dx = b2->x - x, dy = b2->y - y;
			if (sqrtf( dx * dx + dy * dy ) < 100) hit = true;
		}
		if (!hit) break;
	}
}

bool MetalBall::Tick()
{
	if ((x -= .2f) < -50) x = SCRWIDTH * 4;
	m_Sprite->Draw( surface, (int)x, (int)y );
	for ( char u = 0; u < 50; u++ ) for ( char v = 0; v < 50; v++ )
	{
		float tx = (float)(u + x), ty = (float)(v + y);
		if ((tx < 0) || (ty < 0) || (tx >= SCRWIDTH) || (ty >= SCRHEIGHT)) continue;
		uint* src1 = m_Sprite->GetBuffer() + u + v * 50;
		if (!(*src1 & 0xffffff)) continue;
		float dx = (float)(u - 25) / 25, dy = (float)(v - 25) / 25;
		float l = sqrtf( dx * dx + dy * dy ) * .2f * PI;
		short sx = (int)(((x + 25) + (int)((160 * sin( l ) + 100) * dx) + SCRWIDTH)) % SCRWIDTH;
		short sy = (int)(((y + 25) + (int)((160 * sin( l ) + 100) * dy) + SCRHEIGHT)) % SCRHEIGHT;
		uint* src2 = surface->pixels + sx + sy * surface->width;
		uint* dst = surface->pixels + (int)tx + (int)ty * surface->width;
		*dst = AddBlend( *src1, *src2 & 0xffff00 );
	}
	return true;
}

bool MetalBall::Hit( float& a_X, float& a_Y, float& a_NX, float& a_NY )
{
	float dist, dx = a_X - (x + 25), dy = a_Y - (y + 25);
	if ((dist = sqrtf( dx * dx + dy * dy )) > 25) return false;
	a_NX = (float)(dx / sqrtf( dx * dx + dy * dy )), a_NY = (float)(dy / sqrtf( dx * dx + dy * dy ));
	a_X = (float)(x + 25 + (dx / sqrtf( dx * dx + dy * dy )) * 25.5f), a_Y = (float)(y + 25 + (dy / sqrtf( dx * dx + dy * dy )) * 25.5f);
	return true;
} 

Playership::Playership()
{
	m_Sprite = new Sprite( new Surface( "assets/playership.png" ), 9 );
	death = new Sprite( new Surface( "assets/death.png" ), 10 );
	x = 10, y = 300, vx = vy = 0, btimer = 5, dtimer = 0;
}

bool Playership::Tick()
{
	int hor = 0, ver = 0;
	if (dtimer)
	{
		death->SetFrame( 9 - (dtimer >> 4) );
		death->Draw( surface, (int)x - 25, (int)y - 20 );
		if (!--dtimer) x = 10, y = 300, vx = vy = 0;
		return true;
	}
	if (btimer) btimer--;
	if (GetAsyncKeyState( VK_UP )) vy = -1.7f, ver = 3;
	else if (GetAsyncKeyState( VK_DOWN )) vy = 1.7f, ver = 6;
	else { vy *= .97f, vy = (fabs( vy ) < .05f)?0:vy; }
	if (GetAsyncKeyState( VK_LEFT )) vx = -1.3f, hor = 0;
	else if (GetAsyncKeyState( VK_RIGHT )) vx = 1.3f, hor = 1;
	else { vx *= .97f, vx = (fabs( vx ) < .05f)?0:vx; }
	x = max( 4.0f, min( SCRWIDTH - 140.0f, x + vx ) );
	y = max( 4.0f, min( SCRHEIGHT - 40.0f, y + vy ) );
	m_Sprite->SetFrame( 2 - hor + ver );
	m_Sprite->Draw( surface, (int)x, (int)y );
	for ( unsigned short i = 2; i < ActorPool::actors; i++ )
	{
		Actor* a = ActorPool::pool[i];
		if (a->GetType() == Actor::BULLET) if (((Bullet*)a)->owner == Bullet::ENEMY)
		{
			float dx = a->x - (x + 20), dy = a->y - (y + 12);
			if (sqrtf( dx * dx + dy * dy ) < 15) dtimer = 159;
		}
		if (a->GetType() == Actor::ENEMY)
		{
			float dx = (a->x + 16) - (x + 20), dy = (a->y + 10) - (y + 12);
			if (sqrtf( dx * dx + dy * dy ) < 18) dtimer = 159;
		}
		if (a->GetType() != Actor::METALBALL) continue;
		float dx = (a->x + 25) - (x + 20), dy = (a->y + 25) - (y + 12);
		if (sqrtf( dx * dx + dy * dy ) < 35) dtimer = 159;
	}
	if ((!GetAsyncKeyState( VK_CONTROL )) || (btimer > 0)) return true;
	Bullet* newbullet = new Bullet();
	newbullet->Init( surface, x + 20, y + 18, 1, 0, Bullet::PLAYER );
	ActorPool::Add( newbullet );
	btimer = 15;
	return true;
}

Enemy::Enemy()
{
	m_Sprite = new Sprite( new Surface( "assets/enemy.png" ), 4 );
	death = new Sprite( new Surface( "assets/edeath.png" ), 4 );
	vx = -1.4f, x = SCRWIDTH * 2 + Rand( SCRWIDTH * 4 );
	vy = 0, y = SCRHEIGHT * .2f + Rand( SCRWIDTH * .6f );
	frame = 0, btimer = 5, dtimer = 0;
}

bool Enemy::Tick()
{
	if (dtimer)
	{
		death->SetFrame( 3 - (dtimer >> 3) );
		death->Draw( surface, (int)x - 1015, (int)y - 15 );
		if (!--dtimer) x = SCRWIDTH * 3, y = Rand( SCRHEIGHT ), vx = -1.4f, vy = 0;
		return true;
	}
	x += vx, y += (vy *= .99f), frame = (frame + 1) % 31;
	if (x < -50) x = SCRWIDTH * 4;
	m_Sprite->SetFrame( frame >> 3 );
	m_Sprite->Draw( surface, (int)x, (int)y );
	for ( int i = 0; i < ActorPool::actors; i++ ) 
	{
		Actor* a = ActorPool::pool[i];
		if (a->GetType() == Actor::BULLET) if (((Bullet*)a)->owner == Bullet::PLAYER)
		{
			double dx = (x + 15) - a->x, dy = (y + 11) - a->y;
			if ((dx * dx + dy * dy) < 100) 
			{
				dtimer = 31, x += 1000;
				ActorPool::Delete( a );
				delete a;
			}
		}
		double hdist = (x + 15) - (a->x + 25), vdist = (a->y + 25) - (y + 11);
		if (((hdist < 0) || (hdist > 120)) || (a->GetType() != Actor::METALBALL)) continue;
		if ((vdist > 0) && (vdist < 30)) vy -= (float)((121 - hdist) * .0015);
		if ((vdist < 0) && (vdist > -30)) vy += (float)((121 - hdist) * .0015);
	}
	if (y < 100) vy += .05f; else if (y > (SCRHEIGHT - 100)) vy -= .05f;
	Playership* p = (Playership*)ActorPool::pool[1];
	float dx = p->x - x, dy = p->y - y, dist = sqrtf( dx * dx + dy * dy );
	if ((dist > 180) || (dist < 100)) return true;
	vx += (float)((dx / 50.0) / dist), vy += (float)((dy / 50.0) / dist);
	if (--btimer) return true; else btimer = 19;
	Bullet* newbullet = new Bullet();
	newbullet->Init( surface, x + 15, y + 10, (float)((dx / 5.0f) / dist), (float)((dy / 5.0f) / dist), Bullet::ENEMY );
	ActorPool::Add( newbullet );
	return true;
}

Bullet::Bullet()
{
	player = new Sprite( new Surface( "assets/playerbullet.png" ), 1 );
	enemy = new Sprite( new Surface( "assets/enemybullet.png" ), 1 );
}

Bullet::~Bullet()
{
	delete player;
	delete enemy;
}

bool Bullet::Tick()
{
	for ( int i = 0; i < 4; i++ )
	{
		x += 1.6f * vx, y += 1.6f * vy;
		if (owner == Bullet::PLAYER) player->Draw( surface, (int)x, (int)y );
		if (owner == Bullet::ENEMY) enemy->Draw( surface, (int)x, (int)y );
		if ((!--life) || (x > SCRWIDTH) || (x < 0) || (y < 0) || (y > SCRHEIGHT))
		{
			ActorPool::Delete( this );
			return false;
		}
		float nx, ny, vx_ = vx, vy_ = vy;
		if (!ActorPool::CheckHit( x, y, nx, ny )) continue;
		m_Spark->Draw( surface, (int)x - 4, (int)y - 4 );
		x += (vx = -2 * (nx * vx_ + ny * vy_) * nx + vx_);
		y += (vy = -2 * (nx * vx_ + ny * vy_) * ny + vy_);
	}
	return true;
}

void Game::Init()
{
	ActorPool::Add( new Starfield() );
	ActorPool::Add( new Playership() );
	for ( char i = 0; i < 50; i++ ) ActorPool::Add( new MetalBall() );
	for ( char i = 0; i < 20; i++ ) ActorPool::Add( new Enemy() );
	Actor::SetSurface( screen );
	Actor::m_Spark = new Sprite( new Surface( "assets/hit.png" ), 1 );
	// Actor::m_Spark->SetFlags( Sprite::FLARE );
}

void Game::DrawBackdrop()
{
	for ( int i, y = 0; y < SCRHEIGHT; y += 4 ) for ( int x = 0; x < SCRWIDTH; x += 4 ) 
	{
		float sum1 = 0, sum2 = 0;
		for ( i = 1; i < ActorPool::actors; i++ ) 
		{
			Actor* a = ActorPool::pool[i];
			if (a->GetType() == Actor::ENEMY) break; else if (a->x > (x + 120)) continue;
			double dx = (a->x + 20) - x, dy = (a->y + 20) - y;
			sum1 += 100000.0f / (float)(dx * dx + dy * dy);
		}
		for ( ; i < ActorPool::actors; i++ ) 
		{
			Actor* a = ActorPool::pool[i];
			if (a->GetType() == Actor::BULLET) break; else if (a->x > (x + 80)) continue;
			double dx = (a->x + 15) - x, dy = (a->y + 12) - y;
			sum2 += 70000.0f / (float)(dx * dx + dy * dy);
		}
		int color = (int)min( 255.0f, sum1 ) + ((int)min( 255.0f, sum2 ) << 16), p = SCRWIDTH;
		for ( i = 0; i < 4; i++ ) screen->pixels[x + (i & 1) + (y + ((i >> 1) & 1)) * p] = color;
	}
}

void Game::Tick( float )
{
	screen->Clear( 0 );
	DrawBackdrop();
	ActorPool::Tick();
}