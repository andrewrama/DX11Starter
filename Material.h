#pragma once
#include "DXCore.h"
#include <DirectXMath.h>
#include <memory>
#include "SimpleShader.h"

class Material
{
public:
	Material(DirectX::XMFLOAT4 colorTint,
	std::shared_ptr<SimpleVertexShader> vs,
	std::shared_ptr<SimplePixelShader> ps);
	~Material();

	DirectX::XMFLOAT4 GetColorTint();
	void SetColorTint(DirectX::XMFLOAT4 _colorTint);

	std::shared_ptr<SimpleVertexShader> GetVertexShader();
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> _vs);

	std::shared_ptr<SimplePixelShader> GetPixelShader();
	void SetPixelShader(std::shared_ptr<SimplePixelShader> _ps);

private:
	DirectX::XMFLOAT4 colorTint;
	std::shared_ptr<SimpleVertexShader> vs;
	std::shared_ptr<SimplePixelShader> ps;
};

