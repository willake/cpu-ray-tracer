#include "precomp.h"

#include "helper.h"

// -----------------------------------------------------------
// Initialize the renderer
// -----------------------------------------------------------
void Renderer::Init()
{
	// create fp32 rgb pixel buffer to render to
	accumulator = (float4*)MALLOC64( SCRWIDTH * SCRHEIGHT * 16 );
	memset( accumulator, 0, SCRWIDTH * SCRHEIGHT * 16 );
}

// -----------------------------------------------------------
// Evaluate light transport
// -----------------------------------------------------------
float3 Renderer::Trace( Ray& ray , int depth)
{
	scene.FindNearest( ray );
	if (ray.objIdx == -1) return 0; // or a fancy sky color
	float3 I = ray.O + ray.t * ray.D;
	float3 N = scene.GetNormal( ray.objIdx, I, ray.D );
	Material* material = scene.GetMaterial(ray.objIdx);
	float3 albedo = material->isAlbedoOverridden ? scene.GetAlbedo( ray.objIdx, I ) : material->albedo;

	/* visualize normal */ // return (N + 1) * 0.5f;
	/* visualize distance */ // return 0.1f * float3( ray.t, ray.t, ray.t );

	if (depth >= depthLimit) return float3(0);

	if (material->type == MaterialType::Light) return scene.GetLightColor();
	
	if (material->type == MaterialType::Glass)
	{
		/* calculate k for refreaction air index = 1, glass index = 1.52*/
		float n1 = ray.inside ? 1.52f : 1;
		float n2 = ray.inside ? 1 : 1.52f;
		float n1Divn2 = n1 / n2;
		float cos1 = dot(N, -ray.D);
		float k = 1 - (n1Divn2 * n1Divn2) * (1 - cos1 * cos1);
		
		if (k < 0) // k < 0 is total internal reflection while k >= 0 have refraction
		{
			float3 RD = reflect(ray.D, N); // reflect direction
			auto reflectRay = Ray(I + (RD * 0.0001f), RD);
			return albedo *
				((material->reflectivity * Trace(reflectRay, depth + 1)) +
				(1 - material->reflectivity) * DirectIllumination(I, N));
		}
		else
		{
			float theta1 = acos(cos1);
			float sin1 = sin(theta1);
			float cos2 = sqrt(1 - (n1Divn2 * sin1) * (n1Divn2 * sin1));
			// un-squared reflectance for s-polarized light
			float uRs = (n1 * cos1 - n2 * cos2) / (n1 * cos1 + n2 * cos2);
			// un-squared reflectance for p-polarized light
			float uRp = (n1 * cos2 - n2 * cos1) / (n1 * cos2 - n2 * cos1);
			float Fr = ((uRs * uRs) + (uRp * uRp)) * 0.5f;
			float Ft = 1 - Fr;
			
			float3 RD = reflect(ray.D, N); // reflect direction
			auto reflectRay = Ray(I + (RD * 0.0001f), RD);
			float3 reflection = albedo *
				((material->reflectivity * Trace(reflectRay, depth + 1)) +
				(1 - material->reflectivity) * DirectIllumination(I, N));

			float3 RfrD = (n1Divn2 * ray.D) + N * (n1Divn2 * cos1 - sqrt(k)); // refract direction 
			auto refractRay = Ray(I + (RfrD * 0.0001f), RfrD);
			refractRay.inside = !refractRay.inside;
			float3 refraction = albedo * Trace(refractRay, depth + 1);

			return Fr * reflection + Ft * refraction;
		}
	}

	if (material->type == MaterialType::Mirror)
	{
		float3 RD = reflect(ray.D, N); // reflect direction
		auto reflectRay = Ray(I + (RD * 0.0001f), RD);
		return albedo *
			((material->reflectivity * Trace(reflectRay, depth + 1)) +
			(1 - material->reflectivity) * DirectIllumination(I, N));
	}

	/* visualize albedo */ return albedo * DirectIllumination(I, N);
}

float3 Renderer::DirectIllumination(float3 I, float3 N)
{
	float3 lightColor = scene.GetLightColor() / 24 * 3; // adjust intensity manually
	float3 lightPos = scene.GetLightPos();
	float3 L = normalize(lightPos - I);
	auto shadowRay = Ray(I + (L * 0.0001f), L);

	scene.quad.Intersect(shadowRay);

	if (scene.IsOccluded(shadowRay)) return float3(0);

	float d = length(lightPos - I);
	float distFactor = 1 / (d * d);
	float angleFactor = max(0.0f, dot(N, L));

	return lightColor * angleFactor * distFactor;
}

// -----------------------------------------------------------
// Main application tick function - Executed once per frame
// -----------------------------------------------------------
void Renderer::Tick( float deltaTime )
{
	// animation
	if (animating) scene.SetTime( anim_time += deltaTime * 0.002f );
	// pixel loop
	Timer t;
	// lines are executed as OpenMP parallel tasks (disabled in DEBUG)
#pragma omp parallel for schedule(dynamic)
	for (int y = 0; y < SCRHEIGHT; y++)
	{
		// trace a primary ray for each pixel on the line
		for (int x = 0; x < SCRWIDTH; x++)
		{
			float4 pixel = float4( Trace( camera.GetPrimaryRay( (float)x, (float)y ), 0), 0 );
			// translate accumulator contents to rgb32 pixels
			screen->pixels[x + y * SCRWIDTH] = RGBF32_to_RGB8( &pixel );
			accumulator[x + y * SCRWIDTH] = pixel;
		}
	}
	// performance report - running average - ms, MRays/s
	static float avg = 10, alpha = 1;
	avg = (1 - alpha) * avg + alpha * t.elapsed() * 1000;
	if (alpha > 0.05f) alpha *= 0.5f;
	float fps = 1000.0f / avg, rps = (SCRWIDTH * SCRHEIGHT) / avg;
	printf( "%5.2fms (%.1ffps) - %.1fMrays/s\n", avg, fps, rps / 1000 );
	// handle user input
	camera.HandleInput( deltaTime );
}

// -----------------------------------------------------------
// Update user interface (imgui)
// -----------------------------------------------------------
void Renderer::UI()
{
	// animation toggle
	ImGui::Checkbox( "Animate scene", &animating );
	// ray query on mouse
	Ray r = camera.GetPrimaryRay( (float)mousePos.x, (float)mousePos.y );
	scene.FindNearest( r );
	ImGui::Text( "Object id: %i", r.objIdx );
}