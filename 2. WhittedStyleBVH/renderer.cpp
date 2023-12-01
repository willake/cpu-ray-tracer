#include "precomp.h"
#include "helper.h"
#include "renderer.h"

// -----------------------------------------------------------
// Initialize the renderer
// -----------------------------------------------------------
void Renderer::Init()
{
	// create fp32 rgb pixel buffer to render to
	accumulator = (float4*)MALLOC64(SCRWIDTH * SCRHEIGHT * 16);
	memset(accumulator, 0, SCRWIDTH * SCRHEIGHT * 16);
}

// -----------------------------------------------------------
// Evaluate light transport
// -----------------------------------------------------------
float3 Renderer::Trace(Ray& ray, int depth)
{
	scene.FindNearest(ray);
	//if (ray.objIdx == -1) return float3(0);
	if (ray.objIdx == -1) return scene.GetSkyColor(ray); // or a fancy sky color
	if (depth >= depthLimit) return float3(0);
	float3 I = ray.O + ray.t * ray.D;
	HitInfo hitInfo = scene.GetHitInfo(ray, I);
	float3 N = hitInfo.normal;
	float2 uv = hitInfo.uv;
	Material* material = hitInfo.material;
	float3 albedo = material->isAlbedoOverridden ? scene.GetAlbedo(ray.objIdx, I) : material->GetAlbedo(uv);

	/* visualize edges */ // return GetEdgeDebugColor(ray.barycentric);
	/* visualize normal */ // return (N + 1) * 0.5f;
	/* visualize distance */ // return 0.1f * float3( ray.t, ray.t, ray.t );
	/* visualize albedo */ // return albedo;

	if (material->type == MaterialType::Light) return scene.GetLightColor();

	float3 out_radiance(0);
	float reflectivity = material->type == MaterialType::Mirror ? material->reflectivity : 0;
	float refractivity = material->type == MaterialType::Glass ? 1 : 0;
	float diffuseness = 1 - (reflectivity + refractivity);

	if (material->type == MaterialType::Mirror)
	{
		float3 R = reflect(ray.D , N);
		Ray r(I + R * EPSILON, R);
		out_radiance += reflectivity * albedo * Trace(r, depth + 1);
	}
	else if (material->type == MaterialType::Glass)
	{
		float3 R = reflect(ray.D, N);
		Ray r(I + R * EPSILON, R);
		float n1 = ray.inside ? 1.2f : 1, n2 = ray.inside ? 1 : 1.2f;
		float eta = n1 / n2, cosi = dot(-ray.D, N);
		float cost2 = 1.0f - eta * eta * (1 - cosi * cosi);
		float Fr = 1;
		if (cost2 > 0)
		{
			float a = n1 - n2, b = n1 + n2, R0 = (a * a) / (b * b), c = 1 - cosi;
			Fr = R0 + (1 - R0) * (c * c * c * c * c);
			float3 T = eta * ray.D + ((eta * cosi - sqrtf(fabs(cost2))) * N);
			Ray t(I + T * EPSILON, T);
			t.inside = !ray.inside;
			out_radiance += albedo * (1 - Fr) * Trace(t, depth + 1);
		}
		out_radiance += albedo * Fr * Trace(r, depth + 1);
	}

	if (diffuseness > 0)
	{
		float3 irradiance = DirectIllumination(I, N);
		float3 ambient = float3(0.2f, 0.2f, 0.2f);
		float3 brdf = albedo * INVPI;
		out_radiance += diffuseness * brdf * (irradiance + ambient);
	}
	float3 medium_scale(1);
	if (ray.inside)
	{
		float3 absorption = material->absorption;
		medium_scale.x = expf(absorption.x * -ray.t);
		medium_scale.y = expf(absorption.y * -ray.t);
		medium_scale.z = expf(absorption.z * -ray.t);
	}

	return medium_scale * out_radiance;
}

float3 Renderer::GetEdgeDebugColor(float2 uv)
{
	if (abs(uv.x) < 0.03f || abs(uv.x - 1) < 0.03f || abs(uv.y) < 0.03f || abs(uv.y - 1) < 0.03f)
	{
		return float3(0, 0, 0);
	}
	else
	{
		return float3(1);
	}
}

float3 Renderer::DirectIllumination(float3 I, float3 N)
{
	// sum irradiance from light sources
	float3 irradiance(0);
	// query the (only) scene light
	float3 pointOnLight = scene.GetLightPos();
	float3 L = pointOnLight - I;
	float distance = length(L);
	L *= 1 / distance;
	float ndotl = dot(N, L);
	if (ndotl < EPSILON) /* we don't face the light */ return 0;
	// cast a shadow ray
	Ray s(I + L * EPSILON, L, distance - 2 * EPSILON);
	if (!scene.IsOccluded(s))
	{
		// light is visible; calculate irradiance (= projected radiance)
		float attenuation = 1 / (distance * distance);
		float3 in_radiance = scene.GetLightColor() * attenuation;
		irradiance = in_radiance * dot(N, L);
	}
	return irradiance;
}

// -----------------------------------------------------------
// Main application tick function - Executed once per frame
// -----------------------------------------------------------
void Renderer::Tick(float deltaTime)
{
	// animation
	if (animating) scene.SetTime(anim_time += deltaTime * 0.002f);
	// pixel loop
	Timer t;
	// lines are executed as OpenMP parallel tasks (disabled in DEBUG)
#pragma omp parallel for schedule(dynamic)
	for (int y = 0; y < SCRHEIGHT; y++)
	{
		// trace a primary ray for each pixel on the line
		for (int x = 0; x < SCRWIDTH; x++)
		{
			float4 pixel = float4(Trace(camera.GetPrimaryRay((float)x, (float)y), 0), 0);
			// translate accumulator contents to rgb32 pixels
			screen->pixels[x + y * SCRWIDTH] = RGBF32_to_RGB8(&pixel);
			accumulator[x + y * SCRWIDTH] = pixel;
		}
	}
	// performance report - running average - ms, MRays/s
	/*static float avg = 10, alpha = 1;
	avg = (1 - alpha) * avg + alpha * t.elapsed() * 1000;
	if (alpha > 0.05f) alpha *= 0.5f;
	float fps = 1000.0f / avg, rps = (SCRWIDTH * SCRHEIGHT) / avg;
	printf("%5.2fms (%.1ffps) - %.1fMrays/s\n", avg, fps, rps / 1000);*/
	m_avg = (1 - m_alpha) * m_avg + m_alpha * t.elapsed() * 1000;
	if (m_alpha > 0.05f) m_alpha *= 0.5f;
	m_fps = 1000.0f / m_avg, m_rps = (SCRWIDTH * SCRHEIGHT) / m_avg;
	printf("%5.2fms (%.1ffps) - %.1fMrays/s\n", m_avg, m_fps, m_rps / 1000);
	// handle user input
	camera.HandleInput(deltaTime);
}

// -----------------------------------------------------------
// Update user interface (imgui)
// -----------------------------------------------------------
void Renderer::UI()
{
	// animation toggle
	ImGui::Checkbox("Animate scene", &animating);
	ImGui::SliderFloat("Camera move speed", &camera.moveSpeed, 1.0f, 10.0f, "%.2f");
	ImGui::SliderFloat("Camera turn speed", &camera.turnSpeed, 1.0f, 10.0f, "%.2f");
	// ray query on mouse
	Ray r = camera.GetPrimaryRay((float)mousePos.x, (float)mousePos.y);
	scene.FindNearest(r);
	ImGui::Text("Object id: %i", r.objIdx);
	ImGui::Text("Triangle count: %i", scene.GetTriangleCount());
	ImGui::Text("Frame: %5.2f ms (%.1ffps)", m_avg, m_fps);
	//ImGui::Text("FPS: %.1ffps", m_fps);
	ImGui::Text("RPS: %.1f Mrays/s", m_rps);
}