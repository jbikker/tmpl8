#include "template/common.h"
#include "cl/tools.cl"

kernel void render( global uint* pixels, const int offset )
{
	// plot a simple pixel to a buffer
	const int p = get_global_id( 0 );
	pixels[p] = (p + offset) << 8;
}

// EOF