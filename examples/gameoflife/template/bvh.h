// Template, IGAD version 3
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2023

#pragma once

#define BVHBINS				8

namespace Tmpl8
{

class Mesh;

//  +-----------------------------------------------------------------------------+
//  |  BVH class (and supporting structs)                                         |
//  |  Bounding Volume Hierarchy.                                           LH2'24|
//  +-----------------------------------------------------------------------------+
#pragma warning(disable:4201) // suppress nameless struct / union warning
struct Intersection
{
	float t, u, v;	// distance along ray & barycentric coordinates of the intersection
	uint prim;		// primitive index
};
struct Ray
{
	Ray() { O4 = D4 = rD4 = float4( 1 ); }
	Ray( float3 origin, float3 direction, float t = 1e30f )
	{
		O = origin, D = direction, rD = safercp( D );
		hit.t = t;
	}
	union { struct { float3 O; float dummy1; }; float4 O4; };
	union { struct { float3 D; float dummy2; }; float4 D4; };
	union { struct { float3 rD; float dummy3; }; float4 rD4; };
	Intersection hit; // total ray size: 64 bytes
};
class BVH
{
public:
	struct BVHNode
	{
		float3 aabbMin; uint leftFirst;
		float3 aabbMax; uint triCount;
		bool isLeaf() const { return triCount > 0; /* empty BVH leaves do not exist */ }
		float Intersect( const Ray& ray ) { return BVH::IntersectAABB( ray, aabbMin, aabbMax ); }
		float SurfaceArea() { return BVH::SA( aabbMin, aabbMax ); }
		float CalculateNodeCost() { return SurfaceArea() * triCount; }
	};
	struct Fragment
	{
		float3 bmin;
		uint primIdx;
		float3 bmax;
		uint clipped = 0;
		bool validBox() { return bmin.x < 1e30f; }
	};
	struct FragSSE { __m128 bmin4, bmax4; };
	BVH() = default;
	float SAHCost( const uint nodeIdx = 0 );
	int NodeCount( const uint nodeIdx = 0 );
	void Build( Mesh* mesh, uint cap = 999999999 );
	void Refit();
	int Intersect( Ray& ray );
private:
	void IntersectTri( Ray& ray, const uint triIdx );
	static float IntersectAABB( const Ray& ray, const float3& aabbMin, const float3& aabbMax );
	static float SA( const float3& aabbMin, const float3& aabbMax )
	{
		float3 e = aabbMax - aabbMin; // extent of the node
		return e.x * e.y + e.y * e.z + e.z * e.x;
	}
public:
	uint* triIdx = 0;
	Fragment* fragment = 0;
	uint idxCount = 0, triCount = 0, newNodePtr = 0;
	BVHNode* bvhNode = 0, * tmp = 0;
	float4* tris = 0;
	bool canRefit = true;
};

}