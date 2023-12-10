#include "precomp.h"
#include "file_scene.h"

FileScene::FileScene(const string& filePath)
{
	errorMaterial.albedo = float3(255, 192, 203) / 255.f;
	
	light = Quad(0, 1);
	floor = Plane(1, float3(0, 1, 0), 1);
	materials[0].isLight = true;
	materials[1].reflectivity = 0.3f;
	objIdUsed = 2;

	SceneData sceneData = LoadSceneFile(filePath);

	sceneName = sceneData.name;
	skydome = Texture(sceneData.skydomeLocation);

	objCount = sceneData.objects.size();

	std::vector<BVH*> bvhs;
	bvhs.resize(objCount);
	for (int i = 0; i < objCount; i++)
	{
		ObjectData& objectData = sceneData.objects[i];
		mat4 T = mat4::Translate(objectData.position)
			* mat4::RotateX(objectData.rotation.x)
			* mat4::RotateY(objectData.rotation.y)
			* mat4::RotateZ(objectData.rotation.z);
		mat4 S = mat4::Scale(objectData.scale);
		bvhs[i] = new BVH(objIdUsed, objectData.modelLocation, T, S);
		bvhs[i]->material.textureDiffuse = std::make_unique<Texture>(objectData.textureLocation);
		objIdUsed++;
	}

	tlasBVH = TLASBVH(bvhs);

	SetTime(0);
}

SceneData FileScene::LoadSceneFile(const string& filePath)
{
	SceneData sceneData;
	// Read the XML file into a string
	std::ifstream file(filePath.c_str());
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	// Parse the XML content using RapidXML
	rapidxml::xml_document<> doc;
	doc.parse<0>(&content[0]);

	// Get the root node
	rapidxml::xml_node<>* root = doc.first_node("scene");

	// Extract scene information
	sceneData.name = root->first_node("scene_name")->value();
	sceneData.skydomeLocation = root->first_node("skydome_location")->value();

	// Extract object information
	for (rapidxml::xml_node<>* objNode = root->first_node("objects")->first_node("object"); objNode; objNode = objNode->next_sibling()) 
	{
		ObjectData obj;
		obj.modelLocation = objNode->first_node("model_location")->value();
		obj.textureLocation = objNode->first_node("texture_location")->value();

		for (rapidxml::xml_node<>* posNode = objNode->first_node("position")->first_node(); posNode; posNode = posNode->next_sibling()) 
		{
			int index = posNode->name()[0] - 'x'; // 'x', 'y', 'z' map to 0, 1, 2
			obj.position[index] = std::stof(posNode->value());
		}

		for (rapidxml::xml_node<>* rotNode = objNode->first_node("rotation")->first_node(); rotNode; rotNode = rotNode->next_sibling()) 
		{
			int index = rotNode->name()[0] - 'x'; // 'x', 'y', 'z' map to 0, 1, 2
			obj.rotation[index] = std::stof(rotNode->value());
		}

		for (rapidxml::xml_node<>* scaleNode = objNode->first_node("scale")->first_node(); scaleNode; scaleNode = scaleNode->next_sibling())
		{
			int index = scaleNode->name()[0] - 'x'; // 'x', 'y', 'z' map to 0, 1, 2
			obj.scale[index] = std::stof(scaleNode->value());
		}

		sceneData.objects.push_back(obj);
	}

	return sceneData;
}

void FileScene::SetTime(float t)
{
	animTime = t;
	// sphere animation: bounce
	float tm = 1 - sqrf(fmodf(animTime, 2.0f) - 1);
	sphere.pos = float3(0.f, 0.5f + tm, 1.f);

	// light source animation: swing
	mat4 M1base = mat4::Translate(float3(0, 2.6f, 2));
	mat4 M1 = M1base * mat4::RotateZ(sinf(animTime * 0.6f) * 0.1f) * mat4::Translate(float3(0, -0.9f, 0));
	light.T = M1, light.invT = M1.FastInvertedTransformNoScale();
}

float3 FileScene::GetSkyColor(const Ray& ray) const
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

float3 FileScene::GetLightPos() const
{
	return float3(0, 2, 0);
}

float3 FileScene::GetLightColor() const
{
	return float3(24, 24, 22);
}


void FileScene::FindNearest(Ray& ray)
{
	light.Intersect(ray);
	floor.Intersect(ray);
	sphere.Intersect(ray);

	tlasBVH.Intersect(ray);
}

bool FileScene::IsOccluded(const Ray& ray)
{
	// from tmpl8rt_IGAD
	if (sphere.IsOccluded(ray)) return true;
	if (light.IsOccluded(ray)) return true;
	Ray shadow = Ray(ray);
	shadow.t = 1e34f;
	tlasBVH.Intersect(shadow);
	if (shadow.objIdx > -1) return true;
	// skip planes
	return false;
}

HitInfo FileScene::GetHitInfo(const Ray& ray, const float3 I)
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
	default:
		BVH* bvh = tlasBVH.blas[ray.objIdx - 2];
		hitInfo.normal = bvh->GetNormal(ray.triIdx, ray.barycentric);
		hitInfo.uv = bvh->GetUV(ray.triIdx, ray.barycentric);
		hitInfo.material = &bvh->material;
		break;
	}

	if (dot(hitInfo.normal, ray.D) > 0) hitInfo.normal = -hitInfo.normal;

	return hitInfo;
}

float3 FileScene::GetAlbedo(int objIdx, float3 I) const
{
	if (objIdx == 1) return floor.GetAlbedo(I);
	return float3(0);
}

int FileScene::GetTriangleCount() const
{
	int count = 0;
	for (int i = 0; i < objCount; i++)
	{
		count += tlasBVH.blas[i]->GetTriangleCount();
	}
	return count;
}