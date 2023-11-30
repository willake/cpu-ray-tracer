#pragma once

namespace Tmpl8
{
    inline float3 GenerateRandomUnitVector()
    {
        // Generate random angles
        const float theta = acosf(2 * RandomFloat() - 1) - (PI / 2);
        const float phi = 2 * PI * RandomFloat();

        // Calculate the corresponding unit vector components
        const float x = std::sin(phi) * std::cos(theta);
        const float y = std::sin(phi) * std::sin(theta);
        const float z = std::cos(phi);

        return float3(x, y, z);
    }
}