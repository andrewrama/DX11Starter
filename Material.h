#pragma once
#include "DXCore.h"
#include <DirectXMath.h>
#include <memory>
#include "SimpleShader.h"

class Material
{
public:
	Material(DirectX::XMFLOAT3 colorTint,
		std::shared_ptr<SimpleVertexShader> vs,
		std::shared_ptr<SimplePixelShader> ps,
		float roughness);

	~Material();

	DirectX::XMFLOAT3 GetColorTint();
	void SetColorTint(DirectX::XMFLOAT3 _colorTint);

	std::shared_ptr<SimpleVertexShader> GetVertexShader();
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> _vs);

	std::shared_ptr<SimplePixelShader> GetPixelShader();
	void SetPixelShader(std::shared_ptr<SimplePixelShader> _ps);

	float GetRoughness();
	void SetRoughness(float _roughness);

private:
	DirectX::XMFLOAT3 colorTint;
	std::shared_ptr<SimpleVertexShader> vs;
	std::shared_ptr<SimplePixelShader> ps;
	float roughness;
};

