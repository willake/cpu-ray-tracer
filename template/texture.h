#pragma once

//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"

// some of codes copy from surface.h

#include <string>

namespace Tmpl8 
{
    class Texture
    {
    private:
        void LoadFromFile(const string file)
        {
            int n;
            unsigned char* data = stbi_load(file.c_str(), &width, &height, &n, 0);
            if (data)
            {
                //pixels = (uint*)MALLOC64(width * height * sizeof(uint));
                pixels.resize(width * height);
                ownBuffer = true; // needs to be deleted in destructor
                const int s = width * height;
                if (n == 1) // greyscale
                {
                    for (int i = 0; i < s; i++)
                    {
                        const unsigned char p = data[i];
                        pixels[i] = p + (p << 8) + (p << 16);
                    }
                }
                else
                {
                    for (int i = 0; i < s; i++) pixels[i] = (data[i * n + 0] << 16) + (data[i * n + 1] << 8) + data[i * n + 2];
                }
            }
            stbi_image_free(data);
        }
    public:
        Texture() = default;
        Texture(const string file) : pixels(0), width(0), height(0)
        {
            FILE* f = fopen(file.c_str(), "rb");
            if (!f) FatalError("File not found: %s", file);
            fclose(f);
            LoadFromFile(file);
        }
        ~Texture()
        {
            //if (ownBuffer) FREE64(pixels); // free only if we allocated the buffer ourselves
        }

        float3 Sample(float u, float v) const
        {
            //if (!pixels)
            if (pixels.size() == 0)
            {
                // throw exception (texture not loaded)
                return float3(0);
            }
            // Clamp texture coordinates to [0, 1]
            u = fmod(u, 1.0f);
            v = fmod(v, 1.0f);

            // Calculate pixel coordinates
            int x = static_cast<int>(u * width);
            int y = static_cast<int>(v * height);

            // Ensure coordinates are within bounds
            x = clamp(x, 0, width - 1);
            y = clamp(y, 0, height - 1);

            // Calculate index in the image data array
            int index = x + y * width;

            uint pixel = pixels[index];

            // Sample color from the texture
            float r = ((pixel >> 16) & 0xFF) / 255.f;
            float g = ((pixel >> 8) & 0xFF) / 255.f;
            float b = (pixel & 0xFF) / 255.f;

            return float3(r, g, b);
        }

    private:
        //uint* pixels = 0;
        std::vector<uint> pixels;
        int width = 0, height = 0;
        bool ownBuffer = false;
    };
}