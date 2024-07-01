#include "template/common.h"
#include "cl/tools.cl"

kernel void render( global uint* pixels, const int offset )
{
	// plot a simple pixel to a buffer
	const int p = get_global_id( 0 );
	pixels[p] = (p + offset) << 8;
}

float intersectAABB( const float3 O, const float3 rD, const float t, const float3 bmin, const float3 bmax )
{
	float tx1 = (bmin.x - O.x) * rD.x, tx2 = (bmax.x - O.x) * rD.x;
	float tmin = min( tx1, tx2 ), tmax = max( tx1, tx2 );
	float ty1 = (bmin.y - O.y) * rD.y, ty2 = (bmax.y - O.y) * rD.y;
	tmin = max( tmin, min( ty1, ty2 ) ), tmax = min( tmax, max( ty1, ty2 ) );
	float tz1 = (bmin.z - O.z) * rD.z, tz2 = (bmax.z - O.z) * rD.z;
	tmin = max( tmin, min( tz1, tz2 ) ), tmax = min( tmax, max( tz1, tz2 ) );
	if (tmax >= tmin && tmin < t && tmax >= 0) return tmin; else return 1e30f;
}

void intersectTri( const float3 O, const float3 D, global float4* tri, const uint triIdx, float* t )
{
	// Moeller-Trumbore ray/triangle intersection algorithm
	const uint vertIdx = triIdx * 3;
	const float3 edge1 = (tri[vertIdx + 1] - tri[vertIdx]).xyz;
	const float3 edge2 = (tri[vertIdx + 2] - tri[vertIdx]).xyz;
	const float3 h = cross( D, edge2 );
	const float a = dot( edge1, h );
	if (fabs( a ) < 0.00001f) return; // ray parallel to triangle
	const float f = 1 / a;
	const float3 s = O - tri[vertIdx].xyz;
	const float u = f * dot( s, h );
	if (u < 0 || u > 1) return;
	const float3 q = cross( s, edge1 );
	const float v = f * dot( D, q );
	if (v < 0 || u + v > 1) return;
	const float d = f * dot( edge2, q );
	if (d > 0.0001f && d < *t) *t = d;
}

kernel void trace( global uint* pixels, global float4* tri, global float4* bvh, global uint* idx )
{
	// trace a ray on the GPU
	const int x = get_global_id( 0 ), y = get_global_id( 1 );
	// define ray
	float3 O = (float3)( x - 128, 128 - y, -500 ) * 0.001f;
	float3 D = (float3)( 0, 0, 1 ), rD = (float3)( 1e30f, 1e30f, 1 );
	float t = 1e30f;
	// traverse bvh
	uint node = 0; // root
	uint stack[32], stackPtr = 0, steps = 0;
	while (1)
	{
		steps++;
		// fetch the node
		float4 bmin = bvh[node * 2 + 0];
		float4 bmax = bvh[node * 2 + 1];
		uint leftFirst = as_uint( bmin.w );
		uint triCount = as_uint( bmax.w );
		if (triCount > 0)
		{
			// process leaf node
			for (uint i = 0; i < triCount; i++) intersectTri( O, D, tri, idx[leftFirst + i], &t );
			if (stackPtr == 0) break;
			node = stack[--stackPtr];
			continue;
		}
		uint child1 = leftFirst, child2 = leftFirst + 1;
		float dist1 = intersectAABB( O, rD, t, bmin.xyz, bmax.xyz );
		float dist2 = intersectAABB( O, rD, t, bmin.xyz, bmax.xyz );
		if (dist1 > dist2) 
		{ 
			float h = dist1; dist1 = dist2; dist2 = h;
			uint t = child1; child1 = child2; child2 = t;
		}
		if (dist1 == 1e30f)
		{
			if (stackPtr == 0) break; else node = stack[--stackPtr];
		}
		else
		{
			node = child1;
			if (dist2 != 1e30f) stack[stackPtr++] = child2;
		}
	}
	// pixels[x + y * 256] = t < 1e30f ? 0xffffff : 0;
	pixels[x + y * 256] = steps << 17;
}

// EOF