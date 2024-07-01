#include "template/common.h" // use SCRWIDTH, SCRHEIGHT from cpu side

// "MandelLeaves" by WAHa_06x36 - https://www.shadertoy.com/view/MttBz8

float2 cmul( float2 a, float2 b ) { return (float2)(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x); }
float fract1( float v ) { return v - (float)(int)v; }
float3 fract3( float3 V ) { return (float3)(fract1( V.x ), fract1( V.y ), fract1( V.z )); }
float3 fabs3( float3 V ) { return (float3)(fabs( V.x ), fabs( V.y ), fabs( V.z )); }

float3 hsv( float h, float s, float v )
{
	float4 K = (float4)(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	float3 p = fabs3( fract3( (float3)(h) + K.xyz ) * 6.0f - K.www );
	float3 t = p - K.xxx;
	return v * mix( K.xxx, (float3)(clamp( t.x, 0.0f, 1.0f ), clamp( t.y, 0.0f, 1.0f ), clamp( t.z, 0.0f, 1.0f )), s );
}

kernel void fractal( write_only image2d_t outimg, const float mousey, const float tm )
{
	int x = get_global_id( 0 );
	int y = get_global_id( 1 );
	float2 fragCoord = (float2)( (float)x, (float)y );
	float2 j = (float2)(SCRWIDTH, SCRHEIGHT);
	float2 surfacePosition = 0.5f * (2.0f * fragCoord - j) / min( j.x, j.y );
	float m = mousey == 0.0f ? 0.0f : mousey / j.y - 0.5f;
	float zoom = exp( m * 8.0f );
	float2 p = zoom * 0.016f * surfacePosition - (float2)(0.805f, -0.176f);
	float2 z = p, c = p, dz = (float2)(1.0f, 0.0f);
	float it = 0.0;
	for (float i = 0.0; i < 4095.0f; i += 1.0)
	{
		dz = (float2)(2.0f * (z.x * dz.x - z.y * dz.y) + 1.0f, 2.0f * (z.x * dz.y + z.y * dz.x));
		z = cmul( z, z ) + c;
		float a = sin( tm * 1.5f + i * 2.0f ) * 0.3f + i * 1.3f;
		float2 t = (float2)(cos( a ) * z.x + sin( a ) * z.y, -sin( a ) * z.x + cos( a ) * z.y);
		if (fabs( t.x ) > 2.0f && fabs( t.y ) > 2.0f) { it = i; break; }
	}
	float z2 = z.x * z.x + z.y * z.y, t = log( z2 ) * sqrt( z2 ) / length( dz ), r = sqrt( z2 );
	float q = zoom * 0.016f * (1.0f / j.x + 1.0f / j.y), d = length( j ), w = q * d / 400.0f;
	float s = q * d / 80.0f, f = 0.0f, g = 0.0f;
	if (t < q) f = t / q, g = 1.f; else f = min( s / (t + s - q) + 1.f / (r + 1.f), 1.f ), g = min( w / (t + w - q), 1.f );
	float3 h = hsv( it / 32.0f + 0.4f, 1.0f - g, f );
	write_imagef( outimg, (int2)(x, y), (float4)(h.z, h.y, h.x, 1) );
}