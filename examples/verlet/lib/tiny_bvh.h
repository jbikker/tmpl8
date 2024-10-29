/*
The MIT License (MIT)

Copyright (c) 2024, Jacco Bikker / Breda University of Applied Sciences.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

//
// Use this in *one* .c or .cpp
//   #define TINYBVH_IMPLEMENTATION
//   #include "tiny_bvh.h"
//

#ifndef TINY_BVH_H_
#define TINY_BVH_H_

// binned BVH building: bin count
#define BVHBINS 8

// aligned memory allocation
#ifdef _MSC_VER
#define ALIGNED_MALLOC( x ) ( ( x ) == 0 ? 0 : _aligned_malloc( ( x ), 64 ) )
#define ALIGNED_FREE( x ) _aligned_free( x )
#else
#define ALIGNED_MALLOC( x ) ( ( x ) == 0 ? 0 : aligned_alloc( 64, ( x ) ) )
#define ALIGNED_FREE( x ) free( x )
#endif

namespace tinybvh {

struct Intersection
{
	float t, u, v;	// distance along ray & barycentric coordinates of the intersection
	uint prim;		// primitive index
};

struct Ray
{
	Ray() = default;
	Ray( float3 origin, float3 direction, float t = 1e30f )
	{
		O = origin, D = direction, rD = safercp( D );
		hit.t = t;
	}
	float3 O, D, rD;
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
		float Intersect( const Ray& ray ) const { return BVH::IntersectAABB( ray, aabbMin, aabbMax ); }
		float SurfaceArea() const { return BVH::SA( aabbMin, aabbMax ); }
		float CalculateNodeCost() const { return SurfaceArea() * triCount; }
	};
	struct Fragment
	{
		float3 bmin;
		uint primIdx;
		float3 bmax;
		uint clipped = 0;
		bool validBox() { return bmin.x < 1e30f; }
	};
	BVH() = default;
	~BVH()
	{
		ALIGNED_FREE( bvhNode );
		delete triIdx;
		delete fragment;
		bvhNode = 0, triIdx = 0, fragment = 0;
	}
	float SAHCost( const uint nodeIdx = 0 ) const
	{
		const BVHNode& n = bvhNode[nodeIdx];
		if (n.isLeaf()) return 2.0f * n.SurfaceArea() * n.triCount;
		float cost = 3.0f * n.SurfaceArea() + SAHCost( n.leftFirst ) + SAHCost( n.leftFirst + 1 );
		return nodeIdx == 0 ? (cost / n.SurfaceArea()) : cost;
	}
	int NodeCount( const uint nodeIdx = 0 ) const
	{
		const BVHNode& n = bvhNode[nodeIdx];
		uint retVal = 1;
		if (!n.isLeaf()) retVal += NodeCount( n.leftFirst ) + NodeCount( n.leftFirst + 1 );
		return retVal;
	}
	void Build( const float4* vertices, const uint primCount );
	void Refit();
	int Intersect( Ray& ray ) const;
private:
	void IntersectTri( Ray& ray, const uint triIdx ) const;
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
};

#ifdef TINYBVH_IMPLEMENTATION

#include <assert.h>

void BVH::Build( const float4* vertices, const uint primCount )
{
	// reset node pool
	newNodePtr = 2, triCount = primCount;
	if (!bvhNode)
	{
		bvhNode = (BVHNode*)ALIGNED_MALLOC( triCount * 2 * sizeof( BVHNode ) );
		memset( &bvhNode[1], 0, 32 ); // node 1 remains unused, for cache line alignment.
		triIdx = new uint[triCount];
		tris = (float4*)vertices;
		fragment = new Fragment[triCount];
		idxCount = triCount;
	}
	else
	{
		assert( triCount == primCount ); // don't change triangle count between builds.
	}
	// assign all triangles to the root node
	BVHNode& root = bvhNode[0];
	root.leftFirst = 0, root.triCount = triCount, root.aabbMin = float3( 1e30f ), root.aabbMax = float3( -1e30f );
	// initialize fragments and update root bounds
	for (uint i = 0; i < triCount; i++)
	{
		fragment[i].bmin = fminf( fminf( tris[i * 3], tris[i * 3 + 1] ), tris[i * 3 + 2] );
		fragment[i].bmax = fmaxf( fmaxf( tris[i * 3], tris[i * 3 + 1] ), tris[i * 3 + 2] );
		root.aabbMin = fminf( root.aabbMin, fragment[i].bmin );
		root.aabbMax = fmaxf( root.aabbMax, fragment[i].bmax ), triIdx[i] = i;
	}
	// subdivide recursively
	uint task[256], taskCount = 0, nodeIdx = 0;
	float3 minDim = (root.aabbMax - root.aabbMin) * 1e-20f, bestLMin = 0, bestLMax = 0, bestRMin = 0, bestRMax = 0;
	while (1)
	{
		while (1)
		{
			BVHNode& node = bvhNode[nodeIdx];
			// find optimal object split
			float3 binMin[3][BVHBINS], binMax[3][BVHBINS];
			for (uint a = 0; a < 3; a++) for (uint i = 0; i < BVHBINS; i++) binMin[a][i] = 1e30f, binMax[a][i] = -1e30f;
			uint count[3][BVHBINS] = { 0 };
			const float3 rpd3 = float3( BVHBINS / (node.aabbMax - node.aabbMin) ), nmin3 = node.aabbMin;
			for (uint i = 0; i < node.triCount; i++) // process all tris for x,y and z at once
			{
				const uint fi = triIdx[node.leftFirst + i];
				int3 bi = int3( ((fragment[fi].bmin + fragment[fi].bmax) * 0.5f - nmin3) * rpd3 );
				bi.x = clamp( bi.x, 0, BVHBINS - 1 ), bi.y = clamp( bi.y, 0, BVHBINS - 1 ), bi.z = clamp( bi.z, 0, BVHBINS - 1 );
				binMin[0][bi.x] = fminf( binMin[0][bi.x], fragment[fi].bmin );
				binMax[0][bi.x] = fmaxf( binMax[0][bi.x], fragment[fi].bmax ), count[0][bi.x]++;
				binMin[1][bi.y] = fminf( binMin[1][bi.y], fragment[fi].bmin );
				binMax[1][bi.y] = fmaxf( binMax[1][bi.y], fragment[fi].bmax ), count[1][bi.y]++;
				binMin[2][bi.z] = fminf( binMin[2][bi.z], fragment[fi].bmin );
				binMax[2][bi.z] = fmaxf( binMax[2][bi.z], fragment[fi].bmax ), count[2][bi.z]++;
			}
			// calculate per-split totals
			float splitCost = 1e30f;
			uint bestAxis = 0, bestPos = 0;
			for (int a = 0; a < 3; a++) if ((node.aabbMax[a] - node.aabbMin[a]) > minDim[a])
			{
				float3 lBMin[BVHBINS - 1], rBMin[BVHBINS - 1], l1 = 1e30f, l2 = -1e30f;
				float3 lBMax[BVHBINS - 1], rBMax[BVHBINS - 1], r1 = 1e30f, r2 = -1e30f;
				float ANL[BVHBINS - 1], ANR[BVHBINS - 1];
				for (uint lN = 0, rN = 0, i = 0; i < BVHBINS - 1; i++)
				{
					lBMin[i] = l1 = fminf( l1, binMin[a][i] ), rBMin[BVHBINS - 2 - i] = r1 = fminf( r1, binMin[a][BVHBINS - 1 - i] );
					lBMax[i] = l2 = fmaxf( l2, binMax[a][i] ), rBMax[BVHBINS - 2 - i] = r2 = fmaxf( r2, binMax[a][BVHBINS - 1 - i] );
					lN += count[a][i], rN += count[a][BVHBINS - 1 - i];
					ANL[i] = lN == 0 ? 1e30f : ((l2 - l1).halfArea() * (float)lN);
					ANR[BVHBINS - 2 - i] = rN == 0 ? 1e30f : ((r2 - r1).halfArea() * (float)rN);
				}
				// evaluate bin totals to find best position for object split
				for (uint i = 0; i < BVHBINS - 1; i++)
				{
					const float C = ANL[i] + ANR[i];
					if (C < splitCost)
					{
						splitCost = C, bestAxis = a, bestPos = i;
						bestLMin = lBMin[i], bestRMin = rBMin[i], bestLMax = lBMax[i], bestRMax = rBMax[i];
					}
				}
			}
			if (splitCost >= node.CalculateNodeCost()) break; // not splitting is better.
			// in-place partition
			uint j = node.leftFirst + node.triCount, src = node.leftFirst;
			const float rpd = rpd3.cell[bestAxis], nmin = nmin3.cell[bestAxis];
			for (uint i = 0; i < node.triCount; i++)
			{
				const uint fi = triIdx[src];
				int bi = (uint)(((fragment[fi].bmin[bestAxis] + fragment[fi].bmax[bestAxis]) * 0.5f - nmin) * rpd);
				bi = clamp( bi, 0, BVHBINS - 1 );
				if ((uint)bi <= bestPos) src++; else swap( triIdx[src], triIdx[--j] );
			}
			// create child nodes
			uint leftCount = src - node.leftFirst, rightCount = node.triCount - leftCount;
			if (leftCount == 0 || rightCount == 0) break; // should not happen.
			const int lci = newNodePtr++, rci = newNodePtr++;
			bvhNode[lci].aabbMin = bestLMin, bvhNode[lci].aabbMax = bestLMax;
			bvhNode[lci].leftFirst = node.leftFirst, bvhNode[lci].triCount = leftCount;
			bvhNode[rci].aabbMin = bestRMin, bvhNode[rci].aabbMax = bestRMax;
			bvhNode[rci].leftFirst = j, bvhNode[rci].triCount = rightCount;
			node.leftFirst = lci, node.triCount = 0;
			// recurse
			task[taskCount++] = rci, nodeIdx = lci;
		}
		// fetch subdivision task from stack
		if (taskCount == 0) break; else nodeIdx = task[--taskCount];
	}
}

void BVH::Refit()
{
	for (int i = newNodePtr - 1; i >= 0; i--)
	{
		BVHNode& node = bvhNode[i];
		if (node.isLeaf()) // leaf: adjust to current triangle vertex positions
		{
			float4 aabbMin( 1e30f ), aabbMax( -1e30f );
			for (uint first = node.leftFirst, j = 0; j < node.triCount; j++)
			{
				const uint vertIdx = triIdx[first + j] * 3;
				aabbMin = fminf( aabbMin, tris[vertIdx] ), aabbMax = fmaxf( aabbMax, tris[vertIdx] );
				aabbMin = fminf( aabbMin, tris[vertIdx + 1] ), aabbMax = fmaxf( aabbMax, tris[vertIdx + 1] );
				aabbMin = fminf( aabbMin, tris[vertIdx + 2] ), aabbMax = fmaxf( aabbMax, tris[vertIdx + 2] );
			}
			node.aabbMin = aabbMin, node.aabbMax = aabbMax;
			continue;
		}
		// interior node: adjust to child bounds
		const BVHNode& left = bvhNode[node.leftFirst], & right = bvhNode[node.leftFirst + 1];
		node.aabbMin = fminf( left.aabbMin, right.aabbMin );
		node.aabbMax = fmaxf( left.aabbMax, right.aabbMax );
	}
}

int BVH::Intersect( Ray& ray ) const
{
	// traverse bvh
	BVHNode* node = &bvhNode[0], * stack[64];
	uint stackPtr = 0, steps = 0;
	while (1)
	{
		steps++;
		if (node->isLeaf())
		{
			for (uint i = 0; i < node->triCount; i++) IntersectTri( ray, triIdx[node->leftFirst + i] );
			if (stackPtr == 0) break; else node = stack[--stackPtr];
			continue;
		}
		BVHNode* child1 = &bvhNode[node->leftFirst], * child2 = &bvhNode[node->leftFirst + 1];
		float dist1 = child1->Intersect( ray ), dist2 = child2->Intersect( ray );
		if (dist1 > dist2) { swap( dist1, dist2 ); swap( child1, child2 ); }
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
	return steps;
}

void BVH::IntersectTri( Ray& ray, const uint idx ) const
{
	// Moeller-Trumbore ray/triangle intersection algorithm
	const uint vertIdx = idx * 3;
	const float3 edge1 = tris[vertIdx + 1] - tris[vertIdx];
	const float3 edge2 = tris[vertIdx + 2] - tris[vertIdx];
	const float3 h = cross( ray.D, edge2 );
	const float a = dot( edge1, h );
	if (fabs( a ) < 0.0000001f) return; // ray parallel to triangle
	const float f = 1 / a;
	const float3 s = ray.O - float3( tris[vertIdx] );
	const float u = f * dot( s, h );
	if (u < 0 || u > 1) return;
	const float3 q = cross( s, edge1 );
	const float v = f * dot( ray.D, q );
	if (v < 0 || u + v > 1) return;
	const float t = f * dot( edge2, q );
	if (t > 0 && t < ray.hit.t) ray.hit.t = t, ray.hit.u = u, ray.hit.v = v, ray.hit.prim = idx;
}

float BVH::IntersectAABB( const Ray& ray, const float3& aabbMin, const float3& aabbMax )
{
	// "slab test" ray/AABB intersection
	float tx1 = (aabbMin.x - ray.O.x) * ray.rD.x, tx2 = (aabbMax.x - ray.O.x) * ray.rD.x;
	float tmin = min( tx1, tx2 ), tmax = max( tx1, tx2 );
	float ty1 = (aabbMin.y - ray.O.y) * ray.rD.y, ty2 = (aabbMax.y - ray.O.y) * ray.rD.y;
	tmin = max( tmin, min( ty1, ty2 ) ), tmax = min( tmax, max( ty1, ty2 ) );
	float tz1 = (aabbMin.z - ray.O.z) * ray.rD.z, tz2 = (aabbMax.z - ray.O.z) * ray.rD.z;
	tmin = max( tmin, min( tz1, tz2 ) ), tmax = min( tmax, max( tz1, tz2 ) );
	if (tmax >= tmin && tmin < ray.hit.t && tmax >= 0) return tmin; else return 1e30f;
}

#endif

}

#endif