#include "Material.h"

Material::Material(DirectX::XMFLOAT3 colorTint,
    std::shared_ptr<SimpleVertexShader> vs,
    std::shared_ptr<SimplePixelShader> ps,
    float roughness) :
    colorTint(colorTint),
    vs(vs),
    ps(ps),
    roughness(roughness)
{
}

Material::~Material()
{
}

DirectX::XMFLOAT3 Material::GetColorTint()
{
    return colorTint;
}

void Material::SetColorTint(DirectX::XMFLOAT3 _colorTint)
{
    colorTint = _colorTint;
}

std::shared_ptr<SimpleVertexShader> Material::GetVertexShader()
{
    return vs;
}

void Material::SetVertexShader(std::shared_ptr<SimpleVertexShader> _vs)
{
    vs = _vs;
}

std::shared_ptr<SimplePixelShader> Material::GetPixelShader()
{
    return ps;
}

void Material::SetPixelShader(std::shared_ptr<SimplePixelShader> _ps)
{
    ps = _ps;
}


void Material::AddTextureSRV(std::string shaderName, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
{
    textureSRVs.insert({ shaderName, srv });
}


void Material::AddSampler(std::string shaderName, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler)
{
    samplers.insert({ shaderName, sampler });
}

void Material::PrepareMaterial()
{
    for (auto& t : textureSRVs) { ps->SetShaderResourceView(t.first.c_str(), t.second); }
    for (auto& s : samplers) { ps->SetSamplerState(s.first.c_str(), s.second); }
}

float Material::GetRoughness()
{
    return roughness;
}

void Material::SetRoughness(float _roughness)
{
    roughness = _roughness;
}
