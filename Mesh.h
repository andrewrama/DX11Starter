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
		int numOfIndices;

		void CreateBuffers(Vertex* _vertices,
			int numOfVertices,
			unsigned int* _indices,
			int _numOfIndices,
			Microsoft::WRL::ComPtr<ID3D11Device> _device);

		void CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices);

	public:
		Mesh(Vertex* _vertices,
			int numOfVertices,
			unsigned int* _indices,
			int _numOfIndices,
			Microsoft::WRL::ComPtr<ID3D11Device> _device);
		Mesh(const std::wstring& objFile, Microsoft::WRL::ComPtr<ID3D11Device> device);
		~Mesh();

		Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
		Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();
		int GetIndexCount();
		void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
};

