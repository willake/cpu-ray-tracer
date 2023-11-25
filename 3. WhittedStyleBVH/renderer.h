#pragma once

#include "texture.h"
#include "material.h"
#include "model.h"
#include "bvh.h"
#include "bvh_scene.h"

#define EPSILON	0.0001f

namespace Tmpl8
{
	class Renderer : public TheApp
	{
	public:
		// game flow methods
		void Init();
		float3 Trace( Ray& ray , int depth);
		float3 DirectIllumination(float3 I, float3 N);
		void Tick( float deltaTime );
		void UI();
		void Shutdown() { /* implement if you want to do things on shutdown */ }
		// input handling
		void MouseUp( int button ) { /* implement if you want to detect mouse button presses */ }
		void MouseDown( int button ) { /* implement if you want to detect mouse button presses */ }
		void MouseMove( int x, int y ) { mousePos.x = x, mousePos.y = y; }
		void MouseWheel( float y ) { /* implement if you want to handle the mouse wheel */ }
		void KeyUp( int key ) { /* implement if you want to handle keys */ }
		void KeyDown( int key ) { /* implement if you want to handle keys */ }
		// data members
		int2 mousePos;
		float4* accumulator;
		BVHScene scene;
		Camera camera;
		bool animating = false;
		float anim_time = 0;
		int depthLimit = 4;
	};
} // namespace Tmpl8