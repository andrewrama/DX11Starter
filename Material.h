#pragma once
#include "DXCore.h"
#include <DirectXMath.h>
#include <memory>
#include "SimpleShader.h"
#include <unordered_map>

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


	void AddTextureSRV(std::string shaderName, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
	void AddSampler(std::string shaderName, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler);
	void PrepareMaterial();

	float GetRoughness();
	void SetRoughness(float _roughness);

private:
	DirectX::XMFLOAT3 colorTint;
	std::shared_ptr<SimpleVertexShader> vs;
	std::shared_ptr<SimplePixelShader> ps;
	float roughness;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> textureSRVs;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11SamplerState>> samplers;
};

