#include "Material.h"

Material::Material(DirectX::XMFLOAT4 colorTint,
    std::shared_ptr<SimpleVertexShader> vs,
    std::shared_ptr<SimplePixelShader> ps) :
    colorTint(colorTint),
    vs(vs),
    ps(ps)
{
}

Material::~Material()
{
}

DirectX::XMFLOAT4 Material::GetColorTint()
{
    return colorTint;
}

void Material::SetColorTint(DirectX::XMFLOAT4 _colorTint)
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
