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

float Material::GetRoughness()
{
    return roughness;
}

void Material::SetRoughness(float _roughness)
{
    roughness = _roughness;
}
