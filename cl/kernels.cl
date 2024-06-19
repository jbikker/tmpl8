#include "template/common.h"
#include "cl/tools.cl"

__kernel void render( __global uint* pixels, const int offset )
{
	// plot a pixel to a buffer
	const int p = get_global_id( 0 );
	pixels[p] = (p + offset) << 8;
}

// EOF