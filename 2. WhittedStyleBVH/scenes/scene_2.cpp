#include "precomp.h"
#include "scene_1.h"

Scene2::Scene2()
{
	light = Quad(0, 1);
	floor = Plane(1, float3(0, 1, 0), 1);
	sphere = Sphere(2, float3(0), 0.6f);
	materials[0] = Material(MaterialType::Light);
	materials[1] = Material(MaterialType::Mirror, float3(0), true);
	materials[1].reflectivity = 0.3f;
	materials[2] = Material(MaterialType::Mirror);
	materials[2].absorption = float3(0.5f, 0, 0.5f);
	mat4 t = mat4::Translate(float3(1, -0.4f, 1));
	mat4 s = mat4::Scale(0.5f);
	bvhModels.push_back(BVH(3, "../assets/sphere.obj", t, s));
	bvhModels[0].material.type = MaterialType::Mirror;
	//bvhModels[0].material.textureDiffuse = std::make_unique<Texture>("../assets/textures/Defuse_wok.png");
	//gridModels.push_back(Grid(3, "../assets/wok.obj", t, s));
	//gridModels[0].material.reflectivity = 0.4;
	//gridModels[0].material.type = MaterialType::Mirror;
	//gridModels[0].material.textureDiffuse = std::make_unique<Texture>("../assets/textures/Defuse_wok.png");
	//mat4 t2 = mat4::Translate(float3(0, -0.4f, 2)) * mat4::Scale(0.5);
	//models.push_back(BVHModel(4, "../assets/wok.obj", t2));
	//models[1].material.textureDiffuse = std::make_unique<Texture>("../assets/textures/Defuse_wok.png");

	//printf("Triangle count: %d\n", sceneBVH.GetTriangleCount());
	//sceneBVH.Build();
	skydome = Texture("../assets/industrial_sunset_puresky_4k.hdr");
	SetTime(0);
}

void Scene2::SetTime(float t)
{
	animTime = t;
	// sphere animation: bounce
	float tm = 1 - sqrf(fmodf(animTime, 2.0f) - 1);
	sphere.pos = float3(-1.8f, 0.4f + tm, 1);

	mat4 modelT = mat4::Translate(float3(1, -0.4f + tm, 1));
	//bvhModels[0].SetTransform(modelT);
	//gridModels[0].SetTransform(modelT);
	// light source animation: swing
	mat4 M1base = mat4::Translate(float3(0, 2.6f, 2));
	mat4 M1 = M1base * mat4::RotateZ(sinf(animTime * 0.6f) * 0.1f) * mat4::Translate(float3(0, -0.9f, 0));
	light.T = M1, light.invT = M1.FastInvertedTransformNoScale();
}

float3 Scene2::GetSkyColor(const Ray& ray) const
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

float3 Scene2::GetLightPos() const
{
	return float3(0, 2, 0);
}

float3 Scene2::GetLightColor() const
{
	return float3(24, 24, 22);
}


void Scene2::FindNearest(Ray& ray)
{
	light.Intersect(ray);
	//floor.Intersect(ray);
	//sphere.Intersect(ray);
	for (int i = 0; i < bvhModels.size(); i++)
	{
		bvhModels[i].Intersect(ray);
	}
	for (int i = 0; i < gridModels.size(); i++)
	{
		gridModels[i].Intersect(ray);
	}
}

bool Scene2::IsOccluded(const Ray& ray)
{
	// from tmpl8rt_IGAD
	//if (sphere.IsOccluded(ray)) return true;
	//if (light.IsOccluded(ray)) return true;
	Ray shadow = Ray(ray);
	shadow.t = 1e34f;
	for (int i = 0; i < bvhModels.size(); i++)
	{
		bvhModels[i].Intersect(shadow);
	}
	for (int i = 0; i < gridModels.size(); i++)
	{
		gridModels[i].Intersect(shadow);
	}
	if (shadow.objIdx > -1) return true;
	// skip planes
	return false;
}

HitInfo Scene2::GetHitInfo(const Ray& ray, const float3 I)
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
		hitInfo.normal = bvhModels[0].GetNormal(ray.triIdx, ray.barycentric);
		hitInfo.uv = bvhModels[0].GetUV(ray.triIdx, ray.barycentric);
		hitInfo.material = &bvhModels[0].material;
		/*hitInfo.normal = gridModels[0].GetNormal(ray.triIdx, ray.barycentric);
		hitInfo.uv = gridModels[0].GetUV(ray.triIdx, ray.barycentric);
		hitInfo.material = &gridModels[0].material;*/
		break;
	case 4:
		hitInfo.normal = bvhModels[1].GetNormal(ray.triIdx, ray.barycentric);
		hitInfo.uv = bvhModels[1].GetUV(ray.triIdx, ray.barycentric);
		hitInfo.material = &bvhModels[1].material;
		/*hitInfo.normal = gridModels[1].GetNormal(ray.triIdx, ray.barycentric);
		hitInfo.uv = gridModels[1].GetUV(ray.triIdx, ray.barycentric);
		hitInfo.material = &gridModels[1].material;*/
		break;
	default:
		break;
	}

	return hitInfo;
}

float3 Scene2::GetAlbedo(int objIdx, float3 I) const
{
	if (objIdx == 1) return floor.GetAlbedo(I);
	return float3(0);
}

int Scene2::GetTriangleCount() const
{
	int count = 0;
	for (int i = 0; i < bvhModels.size(); i++)
	{
		count += bvhModels[i].GetTriangleCount();
	}
	for (int i = 0; i < gridModels.size(); i++)
	{
		count += gridModels[i].GetTriangleCount();
	}
	return count;
}