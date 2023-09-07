#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include "Vertex.h"
#include "DXCore.h"
#include <memory>
class Mesh
{
	private:
		Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceCtx;
		int numOfIndices;

	public:
		Mesh(Vertex* _vertices,
			int numOfVertices,
			unsigned int* _indices,
			int _numOfIndices,
			Microsoft::WRL::ComPtr<ID3D11Device> _device,
			Microsoft::WRL::ComPtr<ID3D11DeviceContext> ctx);
		~Mesh();

		Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
		Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();
		int GetIndexCount();
		void Draw();
};

