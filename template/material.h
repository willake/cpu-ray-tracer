#pragma once
#include <memory>

namespace Tmpl8
{
	enum MaterialType
	{
		Diffuse,
		Mirror,
		Glass,
		Light
	};

	class Material
	{
	public:
		Material(const MaterialType type = MaterialType::Diffuse, const float3 albedo = float3(1.0f), const bool isAlbedoOverridden = false)
		{
			this->type = type;
			this->albedo = albedo;
			this->isAlbedoOverridden = isAlbedoOverridden;
			this->reflectivity = 1.0f;
			this->refractivity = 1.0f;
			this->absorption = float3(0);
		}
		float3 GetAlbedo(float2 uv)
		{
			if (textureDiffuse.get() == nullptr)
			{
				return albedo;
			}
			return textureDiffuse->Sample(uv.x, uv.y);
		}
	public:
		MaterialType type;
		float3 albedo{};
		bool isAlbedoOverridden;
		float reflectivity;
		float refractivity;
		float3 absorption;
		std::unique_ptr<Texture> textureDiffuse;
		/*Texture textureMetallic;
		Texture textuteRoughness;*/
	};
}