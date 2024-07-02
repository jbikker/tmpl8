// Template, 2024 IGAD Edition
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/BUAS/UU - Jacco Bikker - 2006-2024

#include "precomp.h"

using namespace Tmpl8;

// constructor
Sprite::Sprite( Surface* surface, unsigned int frameCount ) :
	width( surface->width / frameCount ),
	height( surface->height ),
	numFrames( frameCount ),
	currentFrame( 0 ),
	flags( 0 ),
	start( new unsigned int* [frameCount] ),
	surface( surface )
{
	InitializeStartData();
}

// destructor
Sprite::~Sprite()
{
	delete surface;
	for (unsigned int i = 0; i < numFrames; i++) delete start[i];
	delete start;
}

// draw sprite to target surface
void Sprite::Draw( Surface* target, int x, int y )
{
	if (x < -width || x >( target->width + width )) return;
	if (y < -height || y >( target->height + height )) return;
	int x1 = x, x2 = x + width;
	int y1 = y, y2 = y + height;
	uint* src = GetBuffer() + currentFrame * width;
	if (x1 < 0) src += -x1, x1 = 0;
	if (x2 > target->width) x2 = target->width;
	if (y1 < 0) src += -y1 * width * numFrames, y1 = 0;
	if (y2 > target->height) y2 = target->height;
	uint* dest = target->pixels;
	int xs;
	if (x2 > x1 && y2 > y1)
	{
		unsigned int addr = y1 * target->width + x1;
		const int w = x2 - x1;
		const int h = y2 - y1;
		for (int j = 0; j < h; j++)
		{
			const int line = j + (y1 - y);
			const int lsx = start[currentFrame][line] + x;
			xs = (lsx > x1) ? lsx - x1 : 0;
			for (int i = xs; i < w; i++)
			{
				const uint c1 = *(src + i);
				if (c1 & 0xffffff) *(dest + addr + i) = c1;
			}
			addr += target->width;
			src += width * numFrames;
		}
	}
}

// draw scaled sprite
void Sprite::DrawScaled( int x1, int y1, int w, int h, Surface* target )
{
	if (width == 0 || height == 0) return;
	for (int x = 0; x < w; x++) for (int y = 0; y < h; y++)
	{
		int u = (int)((float)x * ((float)width / (float)w));
		int v = (int)((float)y * ((float)height / (float)h));
		uint color = GetBuffer()[u + v * width * numFrames];
		if (color & 0xffffff) target->pixels[x1 + x + ((y1 + y) * target->width)] = color;
	}
}

// special draw function for galaxy
void Sprite::DrawScaledAdditiveSubpixel( float x, float y, float w, float h, Surface* target )
{
	// calculate address of source image
	uint* src = surface->pixels + currentFrame * width;
	// calculate screen space bounding box of the scaled image
	int x1 = (int)(x - w / 2), y1 = (int)(y - h / 2);
	int x2 = (int)(x + 1 + w / 2), y2 = (int)(y + 1 + h / 2);
	// loop over target box; do a filtered read from source for each pixel
	for (int i = x1; i < x2; i++) for (int j = y1; j < y2; j++)
	{
		// determine floating point (sub-pixel) source coordinates
		float sx = max( 0.0f, ((float)i - (x - w / 2)) * (width / w) );
		float sy = max( 0.0f, ((float)j - (y - h / 2)) * (height / h) );
		// use bilinear interpolation to read from source image
		float x_in_pixel = sx - (int)sx;
		float y_in_pixel = sy - (int)sy;
		float w0 = (1 - x_in_pixel) * (1 - y_in_pixel);
		float w1 = x_in_pixel * (1 - y_in_pixel);
		float w2 = (1 - x_in_pixel) * y_in_pixel;
		float w3 = 1 - (w0 + w1 + w2);
		int x0 = min( width - 1, max( 0, (int)sx ) );
		int y0 = min( height - 1, max( 0, (int)sy ) );
		int x3 = min( width - 1, max( 0, (int)(sx + 1) ) );
		int y3 = min( height - 1, max( 0, (int)(sy + 1) ) );
		uint p0 = src[x0 + y0 * width * numFrames];
		uint p1 = src[x3 + y0 * width * numFrames];
		uint p2 = src[x0 + y3 * width * numFrames];
		uint p3 = src[x3 + y3 * width * numFrames];
		uint scaledp0 = ScaleColor( p0, (int)(w0 * 255.9f) );
		uint scaledp1 = ScaleColor( p1, (int)(w1 * 255.9f) );
		uint scaledp2 = ScaleColor( p2, (int)(w2 * 255.9f) );
		uint scaledp3 = ScaleColor( p3, (int)(w3 * 255.9f) );
		uint color = scaledp0 + scaledp1 + scaledp2 + scaledp3;
		target->pixels[i + j * target->width] = AddBlend( target->pixels[i + j * target->width], color );
	}
}

// prepare sprite outline data for faster rendering
void Sprite::InitializeStartData()
{
	for (unsigned int f = 0; f < numFrames; ++f)
	{
		start[f] = new unsigned int[height];
		for (int y = 0; y < height; ++y)
		{
			start[f][y] = width;
			uint* addr = GetBuffer() + f * width + y * width * numFrames;
			for (int x = 0; x < width; ++x) if (addr[x])
			{
				start[f][y] = x;
				break;
			}
		}
	}
}