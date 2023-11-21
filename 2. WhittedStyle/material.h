#pragma once

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
		}
		MaterialType type;
		float3 albedo{};
		bool isAlbedoOverridden;
	};
}