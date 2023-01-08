// Template, IGAD version 3
// Get the latest version from: https://github.com/jbikker/tmpl8
// IGAD/NHTV/UU - Jacco Bikker - 2006-2023

#pragma once

namespace Tmpl8
{

// basic sprite class
class Sprite
{
public:
	// structors
	Sprite( Surface* a_Surface, unsigned int a_NumFrames );
	~Sprite();
	// methods
	void Draw( Surface* a_Target, int a_X, int a_Y );
	void DrawScaled( int a_X, int a_Y, int a_Width, int a_Height, Surface* a_Target );
	void SetFlags( unsigned int a_Flags ) { flags = a_Flags; }
	void SetFrame( unsigned int a_Index ) { currentFrame = a_Index; }
	unsigned int GetFlags() const { return flags; }
	int GetWidth() { return width; }
	int GetHeight() { return height; }
	uint* GetBuffer() { return surface->pixels; }
	unsigned int Frames() { return numFrames; }
	Surface* GetSurface() { return surface; }
	void InitializeStartData();
private:
	// attributes
	int width, height;
	unsigned int numFrames;
	unsigned int currentFrame;
	unsigned int flags;
	unsigned int** start;
	Surface* surface;
};

}