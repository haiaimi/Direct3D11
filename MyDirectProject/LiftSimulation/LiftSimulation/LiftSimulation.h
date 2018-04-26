#pragma once

#include "d3d11.h"
#include "DirectXMath.h"
#include <algorithm>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <DirectXPackedVector.h>
#include "GameTimer.h"
#include "d3dx11effect.h"
#include "LightHelper.h"

#define ReleaseCOM(x) { if(x){ x->Release(); x = 0; } }        //COM组件释放

using namespace DirectX;
using namespace DirectX::PackedVector;

struct Vertex
{
	Vertex() {};

	XMFLOAT3 Pos;       //顶点位置
	XMFLOAT3 Normal;    //顶点法线
	XMFLOAT2 TexCoord;  //贴图坐标

	Vertex(
		float px, float py, float pz,     //顶点
		float nx, float ny, float nz,     //法线方向
		float u, float v             //uv坐标
	) :
		Pos(px, py, pz),
		Normal(nx, ny, nz),
		TexCoord(u, v)
	{}
};

class LiftApp
{
public:
	LiftApp(HINSTANCE hInstance);
	virtual ~LiftApp();

	/**渲染运行,里面执行帧循环*/
	int Run();

	/**初始化D3D设备等等*/
	bool Init();

	/**更新画面*/
	void UpdateScene(float dt);
	void DrawScene();

	/**
	 * wParam和IParam在不同的Message下有不同的作用，如 WM_SIZE下就有窗口尺寸信息
	 * @Param wParam 对应32位的无符号整型数
	 */
	LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM IParam);

	/**创建正方体缓冲*/
	void BuildBoxBuffer();

	/**创建效果文件*/
	void BuildFX();

	/**设置输入布局*/
	void SetInputLayout();

protected:
	bool InitMainWindow();
	bool InitDirect3D();

protected:
	HINSTANCE mhAppInst;
	HWND mhMainWnd;

	GameTimer mTimer; //游戏计时器，使用了开源代码，见GameTimer

	ID3D11Device* md3dDevice;     //设备指针
	ID3D11DeviceContext* md3dImmediateContext;    //设备上下文
	IDXGISwapChain* mSwapChain;    //交换链
	ID3D11Texture2D* mDepthStencilBuffer;     //深度模板缓冲
	ID3D11RenderTargetView* mRenderTargetView;     //RenderTarget
	ID3D11DepthStencilView* mDepthStencilView;
	D3D11_VIEWPORT mScreenViewport;
	ID3DX11Effect* mFX;

	ID3D11Buffer* mBoxVB;   //顶点缓冲
	ID3D11Buffer* mBoxIB;   //索引缓冲

	ID3DX11EffectTechnique* mTech;
	ID3DX11EffectMatrixVariable* mfxWorldViewProj;   //用于Shader中的世界观察投影机矩阵
	ID3DX11EffectMatrixVariable* mfxWorld;         //世界矩阵
	ID3DX11EffectMatrixVariable* mfxWorldInvTranspose;     //用于法线转换
	ID3DX11EffectVectorVariable* mfxEyePosW;          //摄像机所在位置
	ID3DX11EffectMatrixVariable* mfxTexTransform;      //纹理变换，如要让纹理旋转
	ID3DX11EffectShaderResourceVariable* mfxDiffuseMap;   //纹理资源
	ID3DX11EffectVariable* mfxDirLight;
	ID3DX11EffectVariable* mfxPointLight;
	ID3DX11EffectVariable* mfxSpotLight;
	ID3DX11EffectVariable* mfxMaterial;
	ID3D11ShaderResourceView* mliftSRV;
	ID3D11ShaderResourceView* mWallSRV;

	ID3D11InputLayout* mInputLayout;       //输入布局

	XMFLOAT4X4 mWorld;
	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;
	XMFLOAT4X4 mTexTransform;
	XMFLOAT3 mEyePosW;

	DirectionalLight mDirLight;
	PointLight mPointLight;
	SpotLight mSpotLight;
	Material mBoxMat;

	std::wstring mWndCaption;
	D3D_DRIVER_TYPE md3dDriveType;
	UINT m4xMsaaQuality;
	int mClientWidth;
	int mClientHeight;

	float liftHeight;
	float dstHeight;
	float moveSpeed;
	bool bCanMove;
};


//入口函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	LiftApp theApp(hInstance);

	if (!theApp.Init())
		return 0;

	return theApp.Run();
}