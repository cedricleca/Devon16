#pragma once

#include <d3d11.h>

extern ID3D11Device*				g_pd3dDevice;
extern ID3D11DeviceContext*			g_pd3dDeviceContext;
extern IDXGISwapChain*				g_pSwapChain;
extern ID3D11RenderTargetView*		g_mainRenderTargetView;
extern ID3D10Blob *					g_pDEVONVertexShaderBlob;
extern ID3D11VertexShader*			g_pDEVONVertexShader;
extern ID3D11InputLayout*			g_pDEVONInputLayout;
extern ID3D11Buffer*				g_pDEVONVertexConstantBuffer;
extern ID3D10Blob *					g_pDEVONPixelShaderBlob;
extern ID3D11PixelShader*			g_pDEVONPixelShader;
extern ID3D11Buffer*				g_pDEVONVB;
extern ID3D11Buffer*				g_pDEVONIB;
extern ID3D11Texture2D*				g_pDEVONTexture0;
extern ID3D11SamplerState*			g_pDEVONSamplerLinear;
extern ID3D11ShaderResourceView*	g_pDEVONTexView;

namespace DXTools
{
	void			CreateRenderTarget();
	void			CleanupRenderTarget();
	bool			CreateDeviceD3D(HWND hWnd);
	void			CleanupDeviceD3D();
	unsigned char *	MapEmulationTexture();
	void			UnmapEmulationTexture();
	void			UpdateConstants(float InRoundness = 0.15f, float InScanline = 0.05);
	void			Render(float * ClearColor, float ViewportW, float ViewportH);
};


