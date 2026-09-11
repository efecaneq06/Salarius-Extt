#pragma once
#include <d3d11.h>
#include <unordered_map>
#include <string>
#include "../ext/nanosvg.h"
#include "../ext/nanosvgrast.h"
#include "embedded_weapons.h"

namespace weapon_icons {

    static std::unordered_map<int, ID3D11ShaderResourceView*> textures;
    static std::unordered_map<int, float> aspectRatios;
    static bool loaded = false;

    static bool CreateTextureFromRGBA(unsigned char* data, int width, int height, ID3D11Device* device,
        ID3D11ShaderResourceView** out_srv)
    {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA subResource = {};
        subResource.pSysMem = data;
        subResource.SysMemPitch = width * 4;

        ID3D11Texture2D* pTexture = nullptr;
        HRESULT hr = device->CreateTexture2D(&desc, &subResource, &pTexture);
        if (FAILED(hr)) return false;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        hr = device->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
        pTexture->Release();
        return SUCCEEDED(hr);
    }

    static bool LoadSVGFromMemory(const char* svgData, ID3D11Device* device, ID3D11ShaderResourceView** out_srv,
        int targetHeight, float* outAspect)
    {
        size_t len = strlen(svgData);
        char* copy = (char*)malloc(len + 1);
        if (!copy) return false;
        memcpy(copy, svgData, len + 1);

        NSVGimage* image = nsvgParse(copy, "px", 96.0f);
        free(copy);
        if (!image) return false;

        float scale = (float)targetHeight / image->height;
        int w = (int)(image->width * scale);
        int h = targetHeight;

        if (outAspect) *outAspect = (float)w / (float)h;

        NSVGrasterizer* rast = nsvgCreateRasterizer();
        if (!rast) { nsvgDelete(image); return false; }

        unsigned char* pixels = (unsigned char*)malloc(w * h * 4);
        if (!pixels) { nsvgDeleteRasterizer(rast); nsvgDelete(image); return false; }

        nsvgRasterize(rast, image, 0, 0, scale, pixels, w, h, w * 4);

        bool ok = CreateTextureFromRGBA(pixels, w, h, device, out_srv);

        free(pixels);
        nsvgDeleteRasterizer(rast);
        nsvgDelete(image);
        return ok;
    }

    static void LoadAll(ID3D11Device* device, const char* = nullptr)
    {
        if (loaded) return;
        loaded = true;

        for (int i = 0; i < embedded_weapons::weaponCount; i++) {
            const auto& w = embedded_weapons::weapons[i];
            ID3D11ShaderResourceView* srv = nullptr;
            float aspect = 2.5f;

            if (LoadSVGFromMemory(w.data, device, &srv, 32, &aspect)) {
                textures[w.id] = srv;
                aspectRatios[w.id] = aspect;
            }
        }
    }

    static ID3D11ShaderResourceView* Get(int weaponId)
    {
        auto it = textures.find(weaponId);
        if (it != textures.end()) return it->second;
        return nullptr;
    }

    static float GetAspect(int weaponId)
    {
        auto it = aspectRatios.find(weaponId);
        if (it != aspectRatios.end()) return it->second;
        return 2.5f;
    }

    static void Cleanup()
    {
        for (auto& p : textures) {
            if (p.second) p.second->Release();
        }
        textures.clear();
        aspectRatios.clear();
        loaded = false;
    }
}
