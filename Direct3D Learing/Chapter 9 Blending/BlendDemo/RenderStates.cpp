//***************************************************************************************
// RenderStates.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "RenderStates.h"

ID3D11RasterizerState* RenderStates::WireframeRS = 0;
ID3D11RasterizerState* RenderStates::NoCullRS    = 0;
	 
ID3D11BlendState*      RenderStates::AlphaToCoverageBS = 0;
ID3D11BlendState*      RenderStates::TransparentBS     = 0;
ID3D11BlendState*      RenderStates::BlotBS            = 0;

ID3D11DepthStencilState*   RenderStates::BlotDSS       = 0;
ID3D11DepthStencilState*   RenderStates::GlobalDDS     = 0;

void RenderStates::InitAll(ID3D11Device* device)
{
	//
	// WireframeRS
	//
	D3D11_RASTERIZER_DESC wireframeDesc;
	ZeroMemory(&wireframeDesc, sizeof(D3D11_RASTERIZER_DESC));
	wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
	wireframeDesc.CullMode = D3D11_CULL_BACK;      //背面剔除
	wireframeDesc.FrontCounterClockwise = false;
	wireframeDesc.DepthClipEnable = true;

	HR(device->CreateRasterizerState(&wireframeDesc, &WireframeRS));

	//
	// NoCullRS
	//
	D3D11_RASTERIZER_DESC noCullDesc;
	ZeroMemory(&noCullDesc, sizeof(D3D11_RASTERIZER_DESC));
	noCullDesc.FillMode = D3D11_FILL_SOLID;
	noCullDesc.CullMode = D3D11_CULL_NONE;        //背面不剔除，因为网格纹理由透明部分，需要看到背面
	noCullDesc.FrontCounterClockwise = false;
	noCullDesc.DepthClipEnable = true;

	HR(device->CreateRasterizerState(&noCullDesc, &NoCullRS));

	//
	// AlphaToCoverageBS
	//

	D3D11_BLEND_DESC alphaToCoverageDesc = {0};
	alphaToCoverageDesc.AlphaToCoverageEnable = true;
	alphaToCoverageDesc.IndependentBlendEnable = false;
	alphaToCoverageDesc.RenderTarget[0].BlendEnable = false;
	alphaToCoverageDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	HR(device->CreateBlendState(&alphaToCoverageDesc, &AlphaToCoverageBS));

	//
	// TransparentBS
	//

	D3D11_BLEND_DESC transparentDesc = {0};
	transparentDesc.AlphaToCoverageEnable = false;
	transparentDesc.IndependentBlendEnable = false;

	transparentDesc.RenderTarget[0].BlendEnable = true;
	transparentDesc.RenderTarget[0].SrcBlend       = D3D11_BLEND_SRC_ALPHA;
	transparentDesc.RenderTarget[0].DestBlend      = D3D11_BLEND_ONE;
	transparentDesc.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
	transparentDesc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
	transparentDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	transparentDesc.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
	transparentDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;      //从BackBuffer写入所有通道
	//transparentDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALPHA | D3D11_COLOR_WRITE_ENABLE_BLUE;       //只允许蓝色

	HR(device->CreateBlendState(&transparentDesc, &TransparentBS));

	D3D11_BLEND_DESC BlotDesc = { 0 };
	BlotDesc.AlphaToCoverageEnable = false;
	BlotDesc.IndependentBlendEnable = false;

	BlotDesc.RenderTarget[0].BlendEnable     = true;
	BlotDesc.RenderTarget[0].SrcBlend        = D3D11_BLEND_ONE;
	BlotDesc.RenderTarget[0].DestBlend       = D3D11_BLEND_ONE;
	BlotDesc.RenderTarget[0].BlendOp         = D3D11_BLEND_OP_ADD;
	BlotDesc.RenderTarget[0].SrcBlendAlpha   = D3D11_BLEND_ZERO;
	BlotDesc.RenderTarget[0].DestBlendAlpha  = D3D11_BLEND_ZERO;
	BlotDesc.RenderTarget[0].BlendOpAlpha    = D3D11_BLEND_OP_ADD;
	BlotDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	device->CreateBlendState(&BlotDesc, &BlotBS);

	D3D11_DEPTH_STENCIL_DESC GlobalDSSDesc;
	GlobalDSSDesc.DepthEnable = true;
	GlobalDSSDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	GlobalDSSDesc.DepthFunc = D3D11_COMPARISON_LESS;
	GlobalDSSDesc.StencilEnable = false;
	device->CreateDepthStencilState(&GlobalDSSDesc, &GlobalDDS);

	D3D11_DEPTH_STENCIL_DESC BlotDSSDesc;
	memset(&BlotDSSDesc, 0, sizeof(D3D11_DEPTH_STENCIL_DESC));
	BlotDSSDesc.DepthEnable     = true;
	BlotDSSDesc.DepthWriteMask  = D3D11_DEPTH_WRITE_MASK_ZERO;
	BlotDSSDesc.DepthFunc       = D3D11_COMPARISON_LESS;
	BlotDSSDesc.StencilEnable   = false;

	device->CreateDepthStencilState(&BlotDSSDesc, &BlotDSS);
}

void RenderStates::DestroyAll()
{
	ReleaseCOM(WireframeRS);
	ReleaseCOM(NoCullRS);
	ReleaseCOM(AlphaToCoverageBS);
	ReleaseCOM(TransparentBS);
}