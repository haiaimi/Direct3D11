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
//������ͼ
Texture2D gDiffuseMap;

//������ʽ
SamplerState samAnisotropic
{
	Filter = ANISOTROPIC;      //��������ȡ����ʽ��Ч������
	MaxAnisotropy = 16;

	AddressU = WRAP;         //wrap�������и�ʽ�������������羵��border��clamp�ȵ�
	AddressV = WRAP;
};

//HLSL�ж�������벼�֣�һ��Ҫ��C++�ж����һ��
struct VertexIn 
{
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
	float2 Tex     : TEXCOORD;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;      //ϵͳֵ�����������ڽ�׶����λ�ã���ʾ����Ļ�ϵ�λ�ã�
	float3 PosW    : POSITION;
	float3 NormalW : NORMAL;
	float2 Tex     : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// ת��������ռ�1
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
		// ����ȡ��
		texColor = gDiffuseMap.Sample(samAnisotropic, pin.Tex);
	}

	//��ʼ����������ֵ����ʼΪ0
	float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

	float4 A, D, S;

	//�������ÿ�Դ���������Ӱ��,LightHelperΪ��Դ
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

	// ��������alphaͨ��
	litColor.a = gMaterial.Diffuse.a * texColor.a;

	return litColor;
}

technique11 LiftTech
{
	pass P0
	{
		//��Ⱦ�ܵ���ִ�в��裬����û��ʹ�ü�����ɫ��
		SetVertexShader(CompileShader(vs_5_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, PS(true)));
	}
}