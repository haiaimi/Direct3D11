#include "LiftSimulation.h"
#include <d3d11.h>
#include "MathHelper.h"

LiftApp* gLiftApp = nullptr;

//回调函数，用于窗口初始化
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM IParam)
{
	return gLiftApp->MsgProc(hwnd, msg, wParam, IParam);
}

LiftApp::LiftApp(HINSTANCE hInstance) :
	mhAppInst(hInstance),
	mWndCaption(L"电梯模拟"),
	md3dDriveType(D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE),
	mClientHeight(800),
	mClientWidth(1200),
	mhMainWnd(0),

	md3dDevice(nullptr),
	md3dImmediateContext(nullptr),
	mSwapChain(nullptr),
	mDepthStencilBuffer(nullptr),
	mRenderTargetView(nullptr),
	mDepthStencilView(nullptr),
	mBoxIB(nullptr),
	mBoxVB(nullptr),
	mFX(nullptr),
	mTech(nullptr),
	mInputLayout(nullptr),
	mEyePosW(0.f,0.f,0.f)
{
	//初始化Viewport结构
	memset(&mScreenViewport, 0, sizeof(D3D11_VIEWPORT));
	gLiftApp = this;

	//初始化世界、观察、投影矩阵
	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&mWorld, I);
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);

	liftHeight = 0.f;
	moveSpeed = 10.f;
	bCanMove = true;
}

LiftApp::~LiftApp()
{
	ReleaseCOM(mSwapChain);
	ReleaseCOM(mDepthStencilBuffer);
	ReleaseCOM(mRenderTargetView);
	ReleaseCOM(mDepthStencilView);

	if (md3dImmediateContext)
		md3dImmediateContext->ClearState();

	ReleaseCOM(md3dDevice);
	ReleaseCOM(md3dImmediateContext);
	ReleaseCOM(mFX);
	ReleaseCOM(mBoxIB);
	ReleaseCOM(mBoxVB);
	ReleaseCOM(mInputLayout);
}

int LiftApp::Run()
{
	MSG msg{ 0 };

	mTimer.Reset();

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			mTimer.Tick();
			UpdateScene(mTimer.DeltaTime());
			DrawScene();
		}
	}

	return (int)msg.wParam;
}

bool LiftApp::Init()
{
	if (!InitMainWindow())
		return false;

	if (!InitDirect3D())
		return false;

	// 直射光
	mDirLight.Ambient = XMFLOAT4(0.5f, 0.5f, 0.f, 1.0f);
	mDirLight.Diffuse = XMFLOAT4(0.7f, 0.8f, 0.4f, 1.0f);
	mDirLight.Specular = XMFLOAT4(0.4f, 0.1f, 0.f, 1.0f);
	mDirLight.Direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	// 点光源
	mPointLight.Ambient = XMFLOAT4(0.f, 1.f, 0.f, 1.0f);
	mPointLight.Diffuse = XMFLOAT4(0.f, 1.f, 0.f, 1.0f);
	mPointLight.Specular = XMFLOAT4(0.f, 1.f, 0.f, 1.0f);
	mPointLight.Att = XMFLOAT3(0.0f, 0.1f, 0.0f);
	mPointLight.Range = 25.0f;

	// 聚光灯（类似手电筒）
	mSpotLight.Ambient = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	mSpotLight.Diffuse = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	mSpotLight.Specular = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	mSpotLight.Att = XMFLOAT3(0.5f, 0.0f, 0.0f);
	mSpotLight.Spot = 96.0f;
	mSpotLight.Range = 10000.0f;

	//方块的材质
	mBoxMat.Ambient = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	mBoxMat.Diffuse = XMFLOAT4(0.7f, 0.77f, 0.46f, 1.0f);
	mBoxMat.Specular = XMFLOAT4(1.f, 0.2f, 0.2f, 26.0f);

	BuildBoxBuffer();
	BuildFX();
	SetInputLayout();

	//初始化成功
	return true;
}

LRESULT LiftApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			mTimer.Stop();
		}
		else
		{
			mTimer.Start();
		}
		return 0;

	//关闭窗口
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool LiftApp::InitMainWindow()
{
	//初始化窗口类
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = mhAppInst;
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"LiftWndClassName";

	//注册窗口
	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"窗口注册失败", 0, 0);
		return false;
	}

	RECT R = { 0, 0, mClientWidth, mClientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	mhMainWnd = CreateWindow(L"LiftWndClassName", mWndCaption.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);

	if (!mhMainWnd)
	{
		MessageBox(0, L"创建窗口失败", 0, 0);
	}

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);

	return true;
}

bool LiftApp::InitDirect3D()
{
	D3D_FEATURE_LEVEL featureLevel;
	HRESULT hr = D3D11CreateDevice(
		nullptr,
		md3dDriveType,
		0,
		0,
		0,
		0,
		D3D11_SDK_VERSION,
		&md3dDevice,
		&featureLevel,
		&md3dImmediateContext);

	if (FAILED(hr))
	{
		MessageBox(0, L"D3D设备创建失败", 0, 0);
		return false;
	}
	
	if (featureLevel != D3D_FEATURE_LEVEL_11_0)
	{
		MessageBox(0, L"该设备不支持D3D11特性", 0, 0);
		return false;
	}

	//交换链初始化结构
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = mClientWidth;
	sd.BufferDesc.Height = mClientHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60; //刷新率
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1;   //不使用多重采样抗锯齿
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT;    //用于渲染目标
	sd.BufferCount = 1;    //缓冲区数目，正常指定一个
	sd.OutputWindow = mhMainWnd;    //指定输出窗口
	sd.Windowed     = true;
	sd.SwapEffect   = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags        = 0;

	//下面是就是开始创建交换链
	IDXGIDevice* dxgiDevice = 0;
	md3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);

	IDXGIAdapter* dxgiAdapter = 0;
	dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);


	IDXGIFactory* dxgiFactory = 0;
	dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);

	dxgiFactory->CreateSwapChain(md3dDevice, &sd, &mSwapChain);   //创建交换链

	ReleaseCOM(dxgiDevice);
	ReleaseCOM(dxgiAdapter);
	ReleaseCOM(dxgiFactory);

	ID3D11Texture2D* backBuffer;       //缓冲区
	mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
	md3dDevice->CreateRenderTargetView(backBuffer, nullptr, &mRenderTargetView);   //创建RenderTarget
	ReleaseCOM(backBuffer);

	D3D11_TEXTURE2D_DESC depthStencilDesc;

	depthStencilDesc.Width     = mClientWidth;
	depthStencilDesc.Height    = mClientHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;

	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	md3dDevice->CreateTexture2D(&depthStencilDesc, 0, &mDepthStencilBuffer);
	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer, 0, &mDepthStencilView);

	//绑定RenderTarget和深度模板缓冲到渲染管道
	md3dImmediateContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);

	//设置视口
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	md3dImmediateContext->RSSetViewports(1, &mScreenViewport);

	return true;
}

void LiftApp::UpdateScene(float dt)
{
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, static_cast<float>(mClientWidth / mClientHeight), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);

	mEyePosW = XMFLOAT3(20.f, 0.f, 20.f);
	XMVECTOR pos = XMVectorSet(50.f, 10.f, 20.f, 1.f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, V);       //获取观察矩阵

	liftHeight += dt * moveSpeed;
	if (liftHeight > 40.f)
	{
		liftHeight = 40.f;
		moveSpeed = -moveSpeed;
	}
	else if (liftHeight < 0.f)
	{
		liftHeight = 0.f;
		moveSpeed = -moveSpeed;
	}

}

void LiftApp::DrawScene()
{
	XMVECTORF32 Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const FLOAT*>(&Cyan));
	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);      //清理RenderTarget和深度模板缓冲进行绘制下一帧
	
	md3dImmediateContext->IASetInputLayout(mInputLayout);
	md3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);   //绘制的基本图元为三角形

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	md3dImmediateContext->IASetVertexBuffers(0, 1, &mBoxVB, &stride, &offset);
	md3dImmediateContext->IASetIndexBuffer(mBoxIB, DXGI_FORMAT_R32_UINT, 0);

	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	mfxDirLight->SetRawValue(&mDirLight, 0, sizeof(mDirLight));
	mfxPointLight->SetRawValue(&mPointLight, 0, sizeof(mPointLight));
	mfxSpotLight->SetRawValue(&mSpotLight, 0, sizeof(mSpotLight));
	
	D3DX11_TECHNIQUE_DESC techDesc;
	mTech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		XMMATRIX scale = XMMatrixScaling(10.f, 2.f, 10.f);      //方块尺寸
		mfxEyePosW->SetRawValue(&mEyePosW, 0, sizeof(mEyePosW));
		XMMATRIX offset = XMMatrixTranslation(0.f, liftHeight-20.f, 0.f);

		XMMATRIX worldViewProj = world * scale * offset * view * proj;     //获取当前世界观察矩阵， 这里一定要注意相乘的顺序，scale一定要在offset前面
		
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);   //保持网格因为Scale变化造成发现异常

		mfxWorld->SetMatrix(reinterpret_cast<float*>(&world));
		mfxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
		mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
		mfxMaterial->SetRawValue(&mBoxMat, 0, sizeof(mBoxMat));

		mTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);

		md3dImmediateContext->DrawIndexed(36, 0, 0);

		offset = XMMatrixTranslation(0.f, - 20.f, 0.f);
		worldViewProj = world * scale * offset * view * proj;
		mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
		mTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);    //这里一定要Apply这样才能绘制新的图形
		md3dImmediateContext->DrawIndexed(36, 0, 0);

		scale = XMMatrixScaling(10.f, 40.f, 2.f);
		offset = XMMatrixTranslation(0.f, 0.f, -5.f);
		worldViewProj = world * scale * offset * view * proj;
		mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
		mTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);    //这里一定要Apply这样才能绘制新的图形
		md3dImmediateContext->DrawIndexed(36, 0, 0);
	}


	mSwapChain->Present(0, 0);     //显示BackBuffer
}

void LiftApp::BuildBoxBuffer()
{
	Vertex v[24];

	std::vector<Vertex> Vertices;
	std::vector<UINT> Indices;

	float w2 = 0.5f;
	float h2 = 0.5f;
	float d2 = 0.5f;

	//下面依次填充六个面的顶点
	v[0] = Vertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[1] = Vertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[2] = Vertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[3] = Vertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	v[4] = Vertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
	v[5] = Vertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	v[6] = Vertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	v[7] = Vertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);

	v[8] = Vertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	v[9] = Vertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	v[10] = Vertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
	v[11] = Vertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f);

	v[12] = Vertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f);
	v[13] = Vertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f);
	v[14] = Vertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);
	v[15] = Vertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);

	v[16] = Vertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[17] = Vertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[18] = Vertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[19] = Vertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	v[20] = Vertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[21] = Vertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[22] = Vertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[23] = Vertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	Vertices.assign(&v[0], &v[24]);

	//下面填充索引
	UINT i[36];

	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;

	i[6] = 4; i[7] = 5; i[8] = 6;
	i[9] = 4; i[10] = 6; i[11] = 7;

	i[12] = 8; i[13] = 9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;

	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;

	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;

	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;

	Indices.assign(&i[0], &i[36]);

	//创建顶点缓冲
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * 24;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &Vertices[0];
	md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB);

	//创建索引缓冲
	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * 36;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &Indices[0];
	md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB);
}

void LiftApp::BuildFX()
{
	std::ifstream fin(L"EffectFile/Lift.fxo", std::ios::binary);

	fin.seekg(0, std::ios_base::end);
	int size = (int)fin.tellg();     //获取该文件的大小
	fin.seekg(0, std::ios_base::beg);
	std::vector<char> compileShader(size);

	fin.read(&compileShader[0], size);
	fin.close();

	//创建效果
	D3DX11CreateEffectFromMemory(&compileShader[0], size, 0, md3dDevice, &mFX);

	mTech = mFX->GetTechniqueByName("LiftTech");
	mfxWorldViewProj = mFX->GetVariableByName("gWorldViewProj")->AsMatrix(); //从效果文件中获取矩阵变量
	mfxWorld = mFX->GetVariableByName("gWorld")->AsMatrix();     //获取world变量
	mfxWorldInvTranspose = mFX->GetVariableByName("gWorldInvTranspose")->AsMatrix(); 
	//获取光线变量
	mfxEyePosW = mFX->GetVariableByName("gEyePosW")->AsVector();
	mfxDirLight = mFX->GetVariableByName("gDirLight");
	mfxPointLight = mFX->GetVariableByName("gPointLight");
	mfxSpotLight = mFX->GetVariableByName("gSpotLight");
	mfxMaterial = mFX->GetVariableByName("gMaterial");
}

void LiftApp::SetInputLayout()
{
	D3D11_INPUT_ELEMENT_DESC descs[3]=
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	//D3D效果相关库，微软已经开源
	D3DX11_PASS_DESC passDesc;
	mTech->GetPassByIndex(0)->GetDesc(&passDesc);
	md3dDevice->CreateInputLayout(descs, 3, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &mInputLayout);
}