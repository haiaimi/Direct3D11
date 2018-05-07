 
#include "LightHelper.fx"
 
cbuffer cbPerFrame
{
	DirectionalLight gDirLights[3];
	float3 gEyePosW;

	float  gFogStart;
	float  gFogRange;
	float4 gFogColor;
};

cbuffer cbPerObject
{
	float4x4 gWorld;
	float4x4 gWorldInvTranspose;
	float4x4 gWorldViewProj;
	float4x4 gTexTransform;
	Material gMaterial;
};

// Nonnumeric values cannot be added to a cbuffer.
Texture2D gDiffuseMap;

SamplerState samAnisotropic
{
	Filter = ANISOTROPIC;
	MaxAnisotropy = 4;

	AddressU = WRAP;
	AddressV = WRAP;
};

struct VertexIn
{
	float3 PosL    : POSITION;
};

struct VertexOut
{
	float3 PosL    : POSITION;
};

// In Tessellation pipeline, VS is mostly used for animation to animate a coarse mesh
// before tessellating it.
VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	vout.PosL = vin.PosL;

	return vout;
}
 
struct PatchTess
{
	float EdgeTess[4]   : SV_TessFactor;
	float InsideTess[2] : SV_InsideTessFactor;
};

PatchTess ConstantHS(InputPatch<VertexOut, 16> patch, uint patchID : SV_PrimitiveID)
{
	PatchTess pt;
	
	// Uniform tessellation for this demo.

	pt.EdgeTess[0] = 25;
	pt.EdgeTess[1] = 25;
	pt.EdgeTess[2] = 25;
	pt.EdgeTess[3] = 25;
	
	pt.InsideTess[0] = 25;
	pt.InsideTess[1] = 25;

	return pt;
}

struct HullOut
{
	float3 PosL : POSITION;
};

// This Hull Shader part is commonly used for a coordinate basis change, 
// for example changing from a quad to a Bezier bi-cubic.
[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(16)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VertexOut, 16> p, 
           uint i : SV_OutputControlPointID,
           uint patchId : SV_PrimitiveID)
{
	HullOut hout;
	
	hout.PosL = p[i].PosL;
	
	hout.PosL.y = 1.5f*hout.PosL.y;
	return hout;
}

struct DomainOut
{
	float4 PosH : SV_POSITION;
	float3 PosW : POSITION;
	float3 NormalW : NORMAL;
};

float4 BernsteinBasis(float t)
{
    float invT = 1.0f - t;

    return float4( invT * invT * invT,
                   3.0f * t * invT * invT,
                   3.0f * t * t * invT,
                   t * t * t );
}

float3 CubicBezierSum(const OutputPatch<HullOut, 16> bezpatch, float4 basisU, float4 basisV)
{
    float3 sum = float3(0.0f, 0.0f, 0.0f);
    sum  = basisV.x * (basisU.x*bezpatch[0].PosL  + basisU.y*bezpatch[1].PosL  + basisU.z*bezpatch[2].PosL  + basisU.w*bezpatch[3].PosL );
    sum += basisV.y * (basisU.x*bezpatch[4].PosL  + basisU.y*bezpatch[5].PosL  + basisU.z*bezpatch[6].PosL  + basisU.w*bezpatch[7].PosL );
    sum += basisV.z * (basisU.x*bezpatch[8].PosL  + basisU.y*bezpatch[9].PosL  + basisU.z*bezpatch[10].PosL + basisU.w*bezpatch[11].PosL);
    sum += basisV.w * (basisU.x*bezpatch[12].PosL + basisU.y*bezpatch[13].PosL + basisU.z*bezpatch[14].PosL + basisU.w*bezpatch[15].PosL);

    return sum;
}

float4 dBernsteinBasis(float t)
{
    float invT = 1.0f - t;

    return float4( -3 * invT * invT,
                   3 * invT * invT - 6 * t * invT,
                   6 * t * invT - 3 * t * t,
                   3 * t * t );
}

float3 ComputeBezierNormal(const OutputPatch<HullOut, 16> bezpatch, float4 basisU, float4 basisV, float4 dbasisU, float4 dbasisV)
{
	float3 normal = float3(0.f, 0.f, 0.f);   //最终的法线方向
	float3 deriU = float3(0.f, 0.f, 0.f);    //U方向的偏导数
	float3 deriV = float3(0.f, 0.f, 0.f);    //V方向的偏导数

	//下面就是计算两个分量的切线
	deriU = basisV.x * (dbasisU.x*bezpatch[0].PosL + dbasisU.y*bezpatch[1].PosL + dbasisU.z*bezpatch[2].PosL + dbasisU.w*bezpatch[3].PosL);
	deriU += basisV.y * (dbasisU.x*bezpatch[4].PosL + dbasisU.y*bezpatch[5].PosL + dbasisU.z*bezpatch[6].PosL + dbasisU.w*bezpatch[7].PosL);
	deriU += basisV.z * (dbasisU.x*bezpatch[8].PosL + dbasisU.y*bezpatch[9].PosL + dbasisU.z*bezpatch[10].PosL + dbasisU.w*bezpatch[11].PosL);
	deriU += basisV.w * (dbasisU.x*bezpatch[12].PosL + dbasisU.y*bezpatch[13].PosL + dbasisU.z*bezpatch[14].PosL + dbasisU.w*bezpatch[15].PosL);

	deriV = dbasisV.x * (basisU.x*bezpatch[0].PosL + basisU.y*bezpatch[1].PosL + basisU.z*bezpatch[2].PosL + basisU.w*bezpatch[3].PosL);
	deriV += dbasisV.y * (basisU.x*bezpatch[4].PosL + basisU.y*bezpatch[5].PosL + basisU.z*bezpatch[6].PosL + basisU.w*bezpatch[7].PosL);
	deriV += dbasisV.z * (basisU.x*bezpatch[8].PosL + basisU.y*bezpatch[9].PosL + basisU.z*bezpatch[10].PosL + basisU.w*bezpatch[11].PosL);
	deriV += dbasisV.w * (basisU.x*bezpatch[12].PosL + basisU.y*bezpatch[13].PosL + basisU.z*bezpatch[14].PosL + basisU.w*bezpatch[15].PosL);

	normal = cross(deriU, deriV);    //两个切线叉积求法线
	return normal;
}

// The domain shader is called for every vertex created by the tessellator.  
// It is like the vertex shader after tessellation.
[domain("quad")]
DomainOut DS(PatchTess patchTess, 
             float2 uv : SV_DomainLocation, 
             const OutputPatch<HullOut, 16> bezPatch)
{
	DomainOut dout;
	
	float4 basisU = BernsteinBasis(uv.x);
	float4 basisV = BernsteinBasis(uv.y);

	float4 dbasisU = dBernsteinBasis(uv.x);   //基量的导数
	float4 dbasisV = dBernsteinBasis(uv.y);

	float3 p  = CubicBezierSum(bezPatch, basisU, basisV);
	
	dout.PosW = p;
	dout.PosH = mul(float4(p, 1.0f), gWorldViewProj);

	//下面计算该点的法线，通过偏导数获得
	dout.NormalW = ComputeBezierNormal(bezPatch, basisU, basisV, dbasisU, dbasisV);
	
	return dout;
}

float4 PS(DomainOut pin) : SV_Target
{
	// Interpolating normal can unnormalize it, so normalize it.
	pin.NormalW = normalize(pin.NormalW);

	// The toEye vector is used in lighting.
	float3 toEye = gEyePosW - pin.PosW;

	// Cache the distance to the eye from this surface point.
	float distToEye = length(toEye);

	// Normalize.
	toEye /= distToEye;

	// Sum the light contribution from each light source.
	float4 litColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	
	// Start with a sum of zero.
	float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

	// Sum the light contribution from each light source.  
	[unroll]
	for (int i = 0; i < 3; ++i)
	{
		float4 A, D, S;
		ComputeDirectionalLight(gMaterial, gDirLights[i], pin.NormalW, toEye,
			A, D, S);

		ambient += A;
		diffuse += D;
		spec += S;
	}

	// Modulate with late add.
	litColor = ambient + diffuse + spec;

	litColor.a = gMaterial.Diffuse.a;

	return litColor;
}

technique11 Tess
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetHullShader( CompileShader( hs_5_0, HS() ) );
        SetDomainShader( CompileShader( ds_5_0, DS() ) );
        SetPixelShader( CompileShader( ps_5_0, PS() ) );
    }
}
