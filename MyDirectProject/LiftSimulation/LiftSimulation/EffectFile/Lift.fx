#include "LightHelper.fx"


cbuffer cbPerFrame
{
	DirectionalLight gDirLight;
	PointLight gPointLight;
	SpotLight gSpotLight;
	float3 gEyePosW;
};

cbuffer cbPerObject
{
	float4x4 gWorld;
	float4x4 gWorldInvTranspose;
	float4x4 gWorldViewProj;
	float4x4 gTexTransform;
	Material gMaterial;
};
//纹理贴图
Texture2D gDiffuseMap;

//采样格式
SamplerState samAnisotropic
{
	Filter = ANISOTROPIC;      //各向异性取样方式，效果更好
	MaxAnisotropy = 16;

	AddressU = WRAP;         //wrap纹理排列格式，还有其他的如镜像，border、clamp等等
	AddressV = WRAP;
};

//HLSL中定义的输入布局，一定要与C++中定义的一致
struct VertexIn 
{
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
	float2 Tex     : TEXCOORD;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;      //系统值，顶点最终在截锥体中位置（显示在屏幕上的位置）
	float3 PosW    : POSITION;
	float3 NormalW : NORMAL;
	float2 Tex     : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// 转换到世界空间1
	vout.PosW = mul(float4(vin.PosL, 1.0f), gWorld).xyz;

	vout.NormalW = mul(vin.NormalL, (float3x3)gWorldInvTranspose);

	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);

	vout.Tex = mul(float4(vin.Tex, 0.0f, 1.0f), gTexTransform).xy;

	return vout;
}

float4 PS(VertexOut pin, uniform bool gUseTexture) : SV_Target
{
	pin.NormalW = normalize(pin.NormalW);

	float3 toEyeW = normalize(gEyePosW - pin.PosW);

	float4 texColor = float4(1, 1, 1, 1);
	if (gUseTexture)
	{
		// 纹理取样
		texColor = gDiffuseMap.Sample(samAnisotropic, pin.Tex);
	}

	//初始化三个反射值，初始为0
	float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

	float4 A, D, S;

	//下面是用开源代码求光照影响,LightHelper为开源
	ComputeDirectionalLight(gMaterial, gDirLight, pin.NormalW, toEyeW, A, D, S);
	ambient += A;
	diffuse += D;
	spec += S;

	ComputePointLight(gMaterial, gPointLight, pin.PosW, pin.NormalW, toEyeW, A, D, S);
	ambient += A;
	diffuse += D;
	spec += S;

	ComputeSpotLight(gMaterial, gSpotLight, pin.PosW, pin.NormalW, toEyeW, A, D, S);
	ambient += A;
	diffuse += D;
	spec += S;

	float4 litColor = texColor * (ambient + diffuse) + spec;

	// 结合纹理的alpha通道
	litColor.a = gMaterial.Diffuse.a * texColor.a;

	return litColor;
}

technique11 LiftTech
{
	pass P0
	{
		//渲染管道中执行步骤，这里没有使用几何着色器
		SetVertexShader(CompileShader(vs_5_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, PS(true)));
	}
}