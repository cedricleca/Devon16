#include "DXTools.h"
#include <d3dcompiler.h>
#include <array>

ID3D11Device*				g_pd3dDevice = nullptr;
ID3D11DeviceContext*		g_pd3dDeviceContext = nullptr;
IDXGISwapChain*				g_pSwapChain = nullptr;
ID3D11RenderTargetView*		g_mainRenderTargetView = nullptr;
ID3D10Blob *				g_pDEVONVertexShaderBlob = nullptr;
ID3D11VertexShader*			g_pDEVONVertexShader = nullptr;
ID3D11InputLayout*			g_pDEVONInputLayout = nullptr;
ID3D11Buffer*				g_pDEVONVertexConstantBuffer = nullptr;
ID3D10Blob *				g_pDEVONPixelShaderBlob = nullptr;
ID3D11PixelShader*			g_pDEVONPixelShader = nullptr;
ID3D11Buffer*				g_pDEVONVB = nullptr;
ID3D11Buffer*				g_pDEVONIB = nullptr;
ID3D11Buffer*				g_pDEVONCB = nullptr;
ID3D11Texture2D*			g_pDEVONTexture0 = nullptr;
ID3D11SamplerState*			g_pDEVONSamplerLinear = nullptr;
ID3D11ShaderResourceView*	g_pDEVONTexView = nullptr;

ID3D11RenderTargetView*		g_BloomTargetView = nullptr;

namespace DXTools
{
	struct Float2
	{
		float x;
		float y;

		Float2() {};
		Float2(float _x, float _y) : x(_x), y(_y) {};
	};

	struct Float3 : public Float2
	{
		float z;

		Float3() {};
		Float3(float _x, float _y, float _z) : Float2(_x, _y), z(_z) {};
	};

	struct SimpleVertex
	{
		Float2 Pos;
		Float3 Col;
		Float2 UV;

		SimpleVertex(float x, float y, float r, float g, float b, float u, float v) : Pos(x, y), Col(r, g, b), UV(u, v) {};
	};

	struct ConstantBuffer
	{
		float CRTRoundness;
		float Scanline;
		float y;	// unused
		float z;	// unused
	};

	struct BufferDesc : public D3D11_BUFFER_DESC
	{
		BufferDesc()
		{
			ZeroMemory(this, sizeof(BufferDesc));
		}
	};

	struct SubresourceData : public D3D11_SUBRESOURCE_DATA
	{
		SubresourceData()
		{
			ZeroMemory(this, sizeof(SubresourceData));
		}
	};

	void CreateRenderTarget()
	{
		DXGI_SWAP_CHAIN_DESC sd;
		g_pSwapChain->GetDesc(&sd);

		// Create the render target
		ID3D11Texture2D* pBackBuffer;
		D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
		ZeroMemory(&render_target_view_desc, sizeof(render_target_view_desc));
		render_target_view_desc.Format = sd.BufferDesc.Format;
		render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
		g_pd3dDevice->CreateRenderTargetView(pBackBuffer, &render_target_view_desc, &g_mainRenderTargetView);
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);

//		g_pd3dDevice->CreateRenderTargetView(pBackBuffer, &render_target_view_desc, &g_mainRenderTargetView);

		pBackBuffer->Release();
	}

	void CleanupRenderTarget()
	{
		if (g_mainRenderTargetView) 
		{ 
			g_mainRenderTargetView->Release(); 
			g_mainRenderTargetView = nullptr; 
		}
	}

	bool CreateDeviceD3D(HWND hWnd)
	{
		// Setup swap chain
		DXGI_SWAP_CHAIN_DESC sd;
		{
			ZeroMemory(&sd, sizeof(sd));
			sd.BufferCount = 2;
			sd.BufferDesc.Width = 0;
			sd.BufferDesc.Height = 0;
			sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			sd.BufferDesc.RefreshRate.Numerator = 60;
			sd.BufferDesc.RefreshRate.Denominator = 1;
			sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sd.OutputWindow = hWnd;
			sd.SampleDesc.Count = 1;
			sd.SampleDesc.Quality = 0;
			sd.Windowed = TRUE;
			sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		}

		UINT createDeviceFlags = 0;
		D3D_FEATURE_LEVEL featureLevel;
		const D3D_FEATURE_LEVEL featureLevelArray[] = { D3D_FEATURE_LEVEL_11_0, };
		if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 1, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
			return false;

		// Create the vertex shader
		static const char* vertexShader = "\
											struct VS_INPUT\
											{\
											float2 pos : POSITION;\
											float4 col : COLOR0;\
											float2 uv : TEXCOORD0;\
											};\
											\
											struct PS_INPUT\
											{\
											float4 pos : SV_POSITION;\
											float4 col : COLOR0;\
											float4 uv : TEXCOORD0;\
											};\
											\
											PS_INPUT VS(VS_INPUT input)\
											{\
											PS_INPUT output;\
											output.pos = float4(input.pos.xy, 0.f, 1.f);\
											output.col = input.col;\
											output.uv = float4(input.uv.xy, 0.f, 1.f);\
											return output;\
											}";

		D3DCompile(vertexShader, strlen(vertexShader), nullptr, nullptr, nullptr, "VS", "vs_4_0", 0, 0, &g_pDEVONVertexShaderBlob, nullptr);
		if (g_pDEVONVertexShaderBlob == nullptr) // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
			return false;

		if (g_pd3dDevice->CreateVertexShader((DWORD*)g_pDEVONVertexShaderBlob->GetBufferPointer(), g_pDEVONVertexShaderBlob->GetBufferSize(), nullptr, &g_pDEVONVertexShader) != S_OK)
			return false;

		// Define the input layout
		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, (size_t)(&((SimpleVertex*)0)->Col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT, 0, (size_t)(&((SimpleVertex*)0)->UV), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		// Create the input layout
		g_pd3dDevice->CreateInputLayout( layout, ARRAYSIZE(layout), g_pDEVONVertexShaderBlob->GetBufferPointer(),
											  g_pDEVONVertexShaderBlob->GetBufferSize(), &g_pDEVONInputLayout );
		g_pDEVONVertexShaderBlob->Release();

		// Compile the pixel shader
		static const char* pixelShader = "\
					cbuffer constants : register( b0 )\
					{\
						float CRTRoundness;\
						float CRTScanline;\
						float d1;\
						float d2;\
					};\
					Texture2D txDiffuse : register( t0 );\
					SamplerState samLinear : register( s0 );\
					struct PS_INPUT\
					{\
						float4 pos : SV_POSITION;\
						float4 col : COLOR0;\
						float4 uv : TEXCOORD0;\
					};\
					float4 Warp(float4 pos){\
						pos = float4((pos.x-(416.0f/1024.0f))*2.0f,(pos.y-(242.0f/1024.0f))*2.0f,0,0);\
						pos *= float4(1.0f+(pos.y*pos.y)*CRTRoundness,1.0f+(pos.x*pos.x)*CRTRoundness,0,0);\
						pos = float4((pos.x*0.5f+(416.0f/1024.0f)),(pos.y*0.5+(242.0f/1024.0f)),0,0);\
					return pos;}\
					float4 PS( PS_INPUT input ) : SV_Target\
					{\
						float4 uv = Warp(input.uv);\
						float scanline = abs(sin(uv.y * 512.0f * 3.14159f)) * CRTScanline + (1 - CRTScanline);\
						float4 ColLeft = txDiffuse.Sample( samLinear, uv+float4(-1.0f/512.0f,0,0,0) );\
						float4 Col = txDiffuse.Sample( samLinear, uv );\
						if(length(ColLeft) > length(Col))\
						{\
							float4 Dif = ColLeft-Col;\
							return  (Col + float4(0.5f*Dif.x,0.3f*Dif.y,0.2f*Dif.z,1.0f)) * scanline * (1+0.5*CRTScanline);\
						}\
						return  (Col * scanline) * (1+0.5*CRTScanline);\
					}";


		D3DCompile(pixelShader, strlen(pixelShader), nullptr, nullptr, nullptr, "PS", "ps_4_0", 0, 0, &g_pDEVONPixelShaderBlob, nullptr);
		if (g_pDEVONPixelShaderBlob == nullptr)  // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
			return false;

		if (g_pd3dDevice->CreatePixelShader((DWORD*)g_pDEVONPixelShaderBlob->GetBufferPointer(), g_pDEVONPixelShaderBlob->GetBufferSize(), nullptr, &g_pDEVONPixelShader) != S_OK)
			return false;

		g_pDEVONPixelShaderBlob->Release();

		// Create vertex buffer
		const float CorticoFieldW = 416.0f / 512.0f;
		const float CorticoFieldH = 242.0f / 512.0f;
		const SimpleVertex vertices[] =
		{
			{ -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f },
			{ 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, CorticoFieldW, 0.0f },
			{ -1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, CorticoFieldH },
			{ 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, CorticoFieldW, CorticoFieldH },
		};
    
		BufferDesc bd;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof( SimpleVertex ) * ARRAYSIZE(vertices);
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		SubresourceData InitData;
		InitData.pSysMem = vertices;
		if(g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pDEVONVB ) != S_OK)
			return false;

		ConstantBuffer Constants;
		Constants.CRTRoundness = 0.15f;
		Constants.Scanline = 0.05f;
		
		SubresourceData InitData_Constants;
		InitData_Constants.pSysMem = &Constants;

		BufferDesc cbDesc;
		cbDesc.ByteWidth = sizeof(ConstantBuffer);
		cbDesc.Usage = D3D11_USAGE_DYNAMIC;
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		if(g_pd3dDevice->CreateBuffer(&cbDesc, &InitData_Constants, &g_pDEVONCB) != S_OK)
			return false;

		g_pd3dDeviceContext->PSSetConstantBuffers(0, 1, &g_pDEVONCB);

		const int OutputSize = 512;
		unsigned char * Buf = new unsigned char[OutputSize*OutputSize*4];
		SubresourceData TexSubResource;
		TexSubResource.pSysMem = Buf;
		TexSubResource.SysMemPitch = OutputSize*4;
		TexSubResource.SysMemSlicePitch = 0;

		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory( &desc, sizeof(desc) );
		desc.Width = desc.Height = OutputSize;
		desc.MipLevels = desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		g_pd3dDevice->CreateTexture2D( &desc, &TexSubResource, &g_pDEVONTexture0 );

		D3D11_SAMPLER_DESC sampDesc;
		ZeroMemory( &sampDesc, sizeof(sampDesc) );
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampDesc.AddressU = sampDesc.AddressV = sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		g_pd3dDevice->CreateSamplerState( &sampDesc, &g_pDEVONSamplerLinear );

		D3D11_SHADER_RESOURCE_VIEW_DESC TexView;
		ZeroMemory( &TexView, sizeof(TexView) );
		TexView.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		TexView.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		TexView.Buffer.NumElements = 1;
		g_pd3dDevice->CreateShaderResourceView(g_pDEVONTexture0, &TexView, &g_pDEVONTexView);

		return true;
	}

	void CleanupDeviceD3D()
	{
		CleanupRenderTarget();
		if( g_pSwapChain ) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
		if( g_pd3dDeviceContext ) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
		if( g_pd3dDevice ) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
		    
		if( g_pDEVONVB ) g_pDEVONVB->Release();
		if (g_pDEVONCB ) g_pDEVONCB->Release();
		if (g_pDEVONIB ) g_pDEVONIB->Release();
		
		if( g_pDEVONInputLayout ) g_pDEVONInputLayout->Release();
		if( g_pDEVONVertexShader ) g_pDEVONVertexShader->Release();
		if( g_pDEVONPixelShader ) g_pDEVONPixelShader->Release();
		if( g_pDEVONTexture0 ) g_pDEVONTexture0->Release();
		if( g_pDEVONSamplerLinear ) g_pDEVONSamplerLinear->Release();
		if( g_pDEVONTexView ) g_pDEVONTexView->Release();
	}

	unsigned char * MapEmulationTexture()
	{
		D3D11_MAPPED_SUBRESOURCE MappedTex;
		g_pd3dDeviceContext->Map(g_pDEVONTexture0, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedTex);
		return (unsigned char *)MappedTex.pData;
	}

	void UnmapEmulationTexture()
	{
		g_pd3dDeviceContext->Unmap(g_pDEVONTexture0, 0);
	}

	void UpdateConstants(float InRoundness, float InScanline)
	{
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		g_pd3dDeviceContext->Map(g_pDEVONCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource);
		ConstantBuffer * Constants = (ConstantBuffer*)mappedSubResource.pData;
		Constants->CRTRoundness = InRoundness;
		Constants->Scanline = InScanline;
		g_pd3dDeviceContext->Unmap(g_pDEVONCB, 0);
	}

	void Render(float * ClearColor, float ViewportW, float ViewportH)
	{
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, ClearColor);

		D3D11_VIEWPORT vp;
		memset(&vp, 0, sizeof(D3D11_VIEWPORT));

		if(ViewportW / ViewportH > (4.0f/3.0f))
		{
			vp.Width = ViewportH * (4.0f/3.0f);
			vp.Height = ViewportH;
			vp.TopLeftX = 0.5f * (ViewportW - vp.Width);
			vp.TopLeftY = 0.0f;
		}
		else
		{
			vp.Width = ViewportW;
			vp.Height = ViewportW * (3.0f/4.0f);
			vp.TopLeftX = 0.0f;
			vp.TopLeftY = 0.5f * (ViewportH - vp.Height);
		}

		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;

		g_pd3dDeviceContext->RSSetViewports(1, &vp);
	
		g_pd3dDeviceContext->IASetInputLayout( g_pDEVONInputLayout );
		// Set vertex buffer
		UINT stride = sizeof( SimpleVertex );
		UINT offset = 0;
		g_pd3dDeviceContext->IASetVertexBuffers( 0, 1, &g_pDEVONVB, &stride, &offset );
		g_pd3dDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );
		
		// Render a quad
		g_pd3dDeviceContext->VSSetShader( g_pDEVONVertexShader, nullptr, 0 );
		g_pd3dDeviceContext->PSSetShader( g_pDEVONPixelShader, nullptr, 0 );
		g_pd3dDeviceContext->PSSetShaderResources( 0, 1, &g_pDEVONTexView );
		g_pd3dDeviceContext->PSSetSamplers( 0, 1, &g_pDEVONSamplerLinear );
		g_pd3dDeviceContext->PSSetConstantBuffers(0, 1, &g_pDEVONCB);
		g_pd3dDeviceContext->Draw( 4, 0 );
	}
};