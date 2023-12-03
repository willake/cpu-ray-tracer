#include "precomp.h"
#include "scene_1.h"

Scene1::Scene1()
{
	errorMaterial.albedo = float3(255, 192, 203) / 255.f;
	light = Quad(0, 1);
	floor = Plane(1, float3(0, 1, 0), 1);
	sphere = Sphere(2, float3(0), 0.6f);
	materials[0].isLight = true;
	materials[1].reflectivity = 0.3f;
	materials[2].refractivity = 1.0f;
	materials[2].absorption = float3(0.5f, 0, 0.5f);
	mat4 t = mat4::Translate(float3(1, -0.4f, 0)) * mat4::Scale(1);
	//mat4 t2 = mat4::Translate(float3(0, -0.4f, 2)) * mat4::Scale(0.5);
	//wok2 = Model(4, "../assets/wok.obj", t2);
	//wok2.material.textureDiffuse = std::make_unique<Texture>("../assets/textures/Defuse_wok.png");
	//wok2.AppendTriangles(sceneBVH.triangles);

	printf("Triangle count: %d\n", sceneBVH.GetTriangleCount());
	sceneBVH.Build();
	skydome = Texture("../assets/industrial_sunset_puresky_4k.hdr");
	SetTime(0);
}

void Scene1::SetTime(float t)
{
	animTime = t;
	// sphere animation: bounce
	float tm = 1 - sqrf(fmodf(animTime, 2.0f) - 1);
	sphere.pos = float3(-1.8f, 0.4f + tm, 1);

	// light source animation: swing
	mat4 M1base = mat4::Translate(float3(0, 2.6f, 2));
	mat4 M1 = M1base * mat4::RotateZ(sinf(animTime * 0.6f) * 0.1f) * mat4::Translate(float3(0, -0.9f, 0));
	light.T = M1, light.invT = M1.FastInvertedTransformNoScale();
}

float3 Scene1::GetSkyColor(const Ray& ray) const
{
	// Convert ray direction to texture coordinates, assuming a spherical skydome
	float phi = atan2(-ray.D.z, ray.D.x) + PI;
	float theta = acos(-ray.D.y);
	float u = phi * INV2PI;
	float v = theta * INVPI;

	//// Sample the HDR skydome texture
	float3 color = skydome.Sample(u, v);

	return color;
}

float3 Scene1::GetLightPos() const
{
	return float3(0, 2, 0);
}

float3 Scene1::GetLightColor() const
{
	return float3(24, 24, 22);
}


void Scene1::FindNearest(Ray& ray)
{
	light.Intersect(ray);
	floor.Intersect(ray);
	//sphere.Intersect(ray);
	//wok.Intersect(ray);
	sceneBVH.Intersect(ray);
	//sceneBVH.IntersectDebug(ray);
}

bool Scene1::IsOccluded(const Ray& ray)
{
	// from tmpl8rt_IGAD
	//if (sphere.IsOccluded(ray)) return true;
	if (light.IsOccluded(ray)) return true;
	Ray shadow = Ray(ray);
	shadow.t = 1e34f;
	sceneBVH.Intersect(shadow);
	//sceneBVH.IntersectDebug(shadow);
	if (shadow.objIdx > -1) return true;
	//if (wok.IsOccluded(ray)) return true;
	// skip planes
	return false;
}

HitInfo Scene1::GetHitInfo(const Ray& ray, const float3 I)
{
	HitInfo hitInfo = HitInfo(float3(0), float2(0), &errorMaterial);
	switch (ray.objIdx)
	{
	case 0:
		hitInfo.normal = light.GetNormal(I);
		hitInfo.uv = float2(0);
		hitInfo.material = &materials[0];
		break;
	case 1:
		hitInfo.normal = floor.GetNormal(I);
		hitInfo.uv = float2(0);
		hitInfo.material = &materials[1];
		break;
	case 2:
		hitInfo.normal = sphere.GetNormal(I);
		hitInfo.uv = float2(0);
		hitInfo.material = &materials[2];
		break;
		break;
	default:
		break;
	}

	return hitInfo;
}

float3 Scene1::GetAlbedo(int objIdx, float3 I) const
{
	if (objIdx == 1) return floor.GetAlbedo(I);
	return float3(0);
}

int Scene1::GetTriangleCount() const
{
	return sceneBVH.GetTriangleCount();
}