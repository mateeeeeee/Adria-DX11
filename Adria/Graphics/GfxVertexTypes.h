#pragma once
#include <d3d11.h>
#include <vector>

namespace adria
{
	constexpr uint16 NUMBER_OF_LAYOUTS = 7U;
	static std::vector<D3D11_INPUT_ELEMENT_DESC> Layouts[NUMBER_OF_LAYOUTS] =
	{
		/*1.*/ {{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }},
		/*2.*/ {{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },{ "TEX", 0, DXGI_FORMAT_R32G32_FLOAT,0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }},
		/*3.*/ {{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },{ "TEX", 0, DXGI_FORMAT_R32G32_FLOAT,0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }},
		/*4.*/ {{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }},
		/*5.*/ {{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 }},
		/*6.*/ {{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }},
		/*7.*/ {{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },{ "TEX", 0, DXGI_FORMAT_R32G32_FLOAT,0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT,0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }, { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT,0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 }}
	};

	struct SimpleVertex
	{
		Vector3 position;

		static std::vector<D3D11_INPUT_ELEMENT_DESC> GetLayout()
		{
			return Layouts[0u];
		}
	};

	struct TexturedVertex
	{
		Vector3 position;
		Vector2 uv;

		static std::vector<D3D11_INPUT_ELEMENT_DESC> GetLayout()
		{
			return Layouts[1u];
		}
	};

	struct TexturedNormalVertex
	{
		Vector3 position;
		Vector2 uv;
		Vector3 normal;

		static std::vector<D3D11_INPUT_ELEMENT_DESC> GetLayout()
		{
			return Layouts[2u];
		}
	};

	struct ColoredVertex
	{
		Vector3 position;
		Vector4 color;

		static std::vector<D3D11_INPUT_ELEMENT_DESC> GetLayout()
		{
			return Layouts[3u];
		}
	};

	struct ColoredNormalVertex
	{
		Vector3 position;
		Vector4 color;
		Vector3 normal;

		static std::vector<D3D11_INPUT_ELEMENT_DESC> GetLayout()
		{
			return Layouts[4u];
		}
	};

	struct NormalVertex
	{
		Vector3 position;
		Vector3 normal;

		static std::vector<D3D11_INPUT_ELEMENT_DESC> GetLayout()
		{
			return Layouts[5u];
		}
	};

	struct CompleteVertex
	{
		Vector3 position;
		Vector2 uv;
		Vector3 normal;
		Vector3 tangent;
		Vector3 bitangent;

		static std::vector<D3D11_INPUT_ELEMENT_DESC> GetLayout()
		{
			return Layouts[6u];
		}
	};

}

