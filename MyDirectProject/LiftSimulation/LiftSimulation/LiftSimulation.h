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

#define ReleaseCOM(x) { if(x){ x->Release(); x = 0; } }        //COM����ͷ�

using namespace DirectX;
using namespace DirectX::PackedVector;

struct Vertex
{
	Vertex() {};

	XMFLOAT3 Pos;       //����λ��
	XMFLOAT3 Normal;    //���㷨��
	XMFLOAT2 TexCoord;  //��ͼ����

	Vertex(
		float px, float py, float pz,     //����
		float nx, float ny, float nz,     //���߷���
		float u, float v             //uv����
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

	/**��Ⱦ����,����ִ��֡ѭ��*/
	int Run();

	/**��ʼ��D3D�豸�ȵ�*/
	bool Init();

	/**���»���*/
	void UpdateScene(float dt);
	void DrawScene();

	/**
	 * wParam��IParam�ڲ�ͬ��Message���в�ͬ�����ã��� WM_SIZE�¾��д��ڳߴ���Ϣ
	 * @Param wParam ��Ӧ32λ���޷���������
	 */
	LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM IParam);

	/**���������建��*/
	void BuildBoxBuffer();

	/**����Ч���ļ�*/
	void BuildFX();

	/**�������벼��*/
	void SetInputLayout();

protected:
	bool InitMainWindow();
	bool InitDirect3D();

protected:
	HINSTANCE mhAppInst;
	HWND mhMainWnd;

	GameTimer mTimer; //��Ϸ��ʱ����ʹ���˿�Դ���룬��GameTimer

	ID3D11Device* md3dDevice;     //�豸ָ��
	ID3D11DeviceContext* md3dImmediateContext;    //�豸������
	IDXGISwapChain* mSwapChain;    //������
	ID3D11Texture2D* mDepthStencilBuffer;     //���ģ�建��
	ID3D11RenderTargetView* mRenderTargetView;     //RenderTarget
	ID3D11DepthStencilView* mDepthStencilView;
	D3D11_VIEWPORT mScreenViewport;
	ID3DX11Effect* mFX;

	ID3D11Buffer* mBoxVB;   //���㻺��
	ID3D11Buffer* mBoxIB;   //��������

	ID3DX11EffectTechnique* mTech;
	ID3DX11EffectMatrixVariable* mfxWorldViewProj;   //����Shader�е�����۲�ͶӰ������
	ID3DX11EffectMatrixVariable* mfxWorld;         //�������
	ID3DX11EffectMatrixVariable* mfxWorldInvTranspose;     //���ڷ���ת��
	ID3DX11EffectVectorVariable* mfxEyePosW;          //���������λ��
	ID3DX11EffectMatrixVariable* mfxTexTransform;      //����任����Ҫ��������ת
	ID3DX11EffectShaderResourceVariable* mfxDiffuseMap;   //������Դ
	ID3DX11EffectVariable* mfxDirLight;
	ID3DX11EffectVariable* mfxPointLight;
	ID3DX11EffectVariable* mfxSpotLight;
	ID3DX11EffectVariable* mfxMaterial;
	ID3D11ShaderResourceView* mliftSRV;
	ID3D11ShaderResourceView* mWallSRV;

	ID3D11InputLayout* mInputLayout;       //���벼��

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


//��ں���
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	LiftApp theApp(hInstance);

	if (!theApp.Init())
		return 0;

	return theApp.Run();
}