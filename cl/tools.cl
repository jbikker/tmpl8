// random numbers: 
// seed using WangHash((threadidx+1)*17), then use RandomInt / RandomFloat

uint WangHash( uint s ) 
{ 
	s = (s ^ 61) ^ (s >> 16), s *= 9, s = s ^ (s >> 4);
	s *= 0x27d4eb2d, s = s ^ (s >> 15); return s; 
}

uint RandomInt( uint* s ) 
{ 
	*s ^= *s << 13, * s ^= *s >> 17, * s ^= *s << 5; 
	return *s; 
}

float RandomFloat( uint* s ) 
{ 
	return RandomInt( s ) * 2.3283064365387e-10f;
}

float safercp( const float x ) 
{ 
	return x > 1e-10f ? (1.0f / x) : (x < -1e-10f ? (1.0f / x) : 1e30f); 
}

float3 safercp3( const float3 v ) 
{ 
	return (float3)( safercp( v.x ), safercp( v.y ), safercp( v.z ) );
}

float3 transformPoint( const float3 P, const float4* T )
{
	const float4 p = (float4)( P, 1 );
	return (float3)( dot( T[0], p ), dot( T[1], p ), dot( T[2], p ) ); 
}

float3 transformVector( const float3 P, const float4* T )
{
	const float4 p = (float4)( P, 0 );
	return (float3)( dot( T[0], p ), dot( T[1], p ), dot( T[2], p ) ); 
}

uint float3ToUint( const float3 v )
{
	const int r = (int)min( 255.0f, v.x * 256.0f);
	const int g = (int)min( 255.0f, v.y * 256.0f);
	const int b = (int)min( 255.0f, v.z * 256.0f);
	return (r << 16) + (g << 8) + b;
}

// EOF