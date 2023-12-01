#include "precomp.h"
#include "animable_scene.h"

AnimableScene::AnimableScene()
{
	light = Quad(0, 1);
	floor = Plane(1, float3(0, 1, 0), 1);
	sphere = Sphere(2, float3(0), 0.6f);
	materials[0] = Material(MaterialType::Light);
	materials[1] = Material(MaterialType::Mirror, float3(0), true);
	materials[1].reflectivity = 0.3f;
	materials[2] = Material(MaterialType::Mirror);
	materials[2].absorption = float3(0.5f, 0, 0.5f);
	mat4 t = mat4::Translate(float3(1, -1.f, 2)) * mat4::RotateX(-PI / 2) * mat4::Scale(0.001);
	tower = Model(3, "../assets/tower.obj", t);
	tower.material.textureDiffuse = std::make_unique<Texture>("../assets/textures/tower.jpg");
	tower.AppendTriangles(sceneBVH.triangles);
	//mat4 t2 = mat4::Translate(float3(0, -0.4f, 2)) * mat4::Scale(0.5);
	//wok2 = Model(4, "../assets/wok.obj", t2);
	//wok2.material.textureDiffuse = std::make_unique<Texture>("../assets/textures/Defuse_wok.png");
	//wok2.AppendTriangles(sceneBVH.triangles);

	printf("Triangle count: %d\n", sceneBVH.GetTriangleCount());
	sceneBVH.Build();
	skydome = Texture("../assets/industrial_sunset_puresky_4k.hdr");
	SetTime(0);
}

void AnimableScene::SetTime(float t)
{
	animTime = t;
	// sphere animation: bounce
	float tm = 1 - sqrf(fmodf(animTime, 2.0f) - 1);
	sphere.pos = float3(-1.8f, 0.4f + tm, 1);

	// light source animation: swing
	mat4 M1base = mat4::Translate(float3(-1, 5, 2));
	mat4 M1 = M1base * mat4::RotateZ(sinf(animTime * 0.6f) * 0.1f) * mat4::Translate(float3(0, -0.9f, 0));
	light.T = M1, light.invT = M1.FastInvertedTransformNoScale();
}

float3 AnimableScene::GetSkyColor(const Ray& ray) const
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

float3 AnimableScene::GetLightPos() const
{
	return float3(-1, 7, 2);
}

float3 AnimableScene::GetLightColor() const
{
	return float3(24, 24, 22);
}


void AnimableScene::FindNearest(Ray& ray)
{
	light.Intersect(ray);
	floor.Intersect(ray);
	//sphere.Intersect(ray);
	//wok.Intersect(ray);
	sceneBVH.Intersect(ray);
	//sceneBVH.IntersectDebug(ray);
}

bool AnimableScene::IsOccluded(const Ray& ray)
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

HitInfo AnimableScene::GetHitInfo(const Ray& ray, const float3 I)
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
	case 3:
		hitInfo.normal = sceneBVH.GetNormal(ray.triIdx, ray.barycentric);
		hitInfo.uv = sceneBVH.GetUV(ray.triIdx, ray.barycentric);
		hitInfo.material = &tower.material;
		break;
	default:
		break;
	}

	return hitInfo;
}

float3 AnimableScene::GetAlbedo(int objIdx, float3 I) const
{
	if (objIdx == 1) return floor.GetAlbedo(I);
	return float3(0);
}

int AnimableScene::GetTriangleCount() const
{
	return sceneBVH.GetTriangleCount();
}