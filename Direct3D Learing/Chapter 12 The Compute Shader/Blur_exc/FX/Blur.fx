//=============================================================================
// Blur.fx by Frank Luna (C) 2011 All Rights Reserved.
//
// Performs a separable blur with a blur radius of 5.  
//=============================================================================

cbuffer cbSettings
{
	float gWeights[11] = 
	{
		0.05f, 0.05f, 0.1f, 0.1f, 0.1f, 0.2f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f,
	};
};

cbuffer cbFixed
{
	static const int gBlurRadius = 5;
};

Texture2D gInput;
RWTexture2D<float4> gOutput;

#define N 256
#define CacheSize (N + 2*gBlurRadius)
groupshared float4 gCache[CacheSize];     //�߳��鹲���ڴ�

[numthreads(N, 1, 1)]
void HorzBlurCS(int3 groupThreadID : SV_GroupThreadID,
				int3 dispatchThreadID : SV_DispatchThreadID)
{
	//
	// Fill local thread storage to reduce bandwidth.  To blur 
	// N pixels, we will need to load N + 2*BlurRadius pixels
	// due to the blur radius.
	//-
	
	// This thread group runs N threads.  To get the extra 2*BlurRadius pixels
	// have 2*BlurRadius threads sample an extra pixel.
	if(groupThreadID.x < gBlurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
		int x = max(dispatchThreadID.x - gBlurRadius, 0);
		gCache[groupThreadID.x] = gInput[int2(x, dispatchThreadID.y)];
	}
	if(groupThreadID.x >= N-gBlurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
		int x = min(dispatchThreadID.x + gBlurRadius, gInput.Length.x-1);
		gCache[groupThreadID.x+2*gBlurRadius] = gInput[int2(x, dispatchThreadID.y)];
	}

	// Clamp out of bound samples that occur at image borders.
	gCache[groupThreadID.x+gBlurRadius] = gInput[min(dispatchThreadID.xy, gInput.Length.xy-1)];     //�߽��ж�

	// Wait for all threads to finish.
	GroupMemoryBarrierWithGroupSync();
	
	//
	// Now blur each pixel.
	//
	float4 blurColor = float4(0, 0, 0, 0);

	float unitBilateralblur = 0.f;    //˫���˲�����λ��

	//��λ��Ȩ��
	unitBilateralblur = 1 / unitBilateralblur;

	[unroll]
	for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
	{
		float o = groupThreadID.x + gBlurRadius;    //�������ص�x����λ��
		int k = groupThreadID.x + gBlurRadius + i;      //groupThreadID.y �����߳����е�yλ��,�������256�����ص��߳����е�y������

														//������ص�Ļҽ�ֵ
		float oGray = gCache[o].x*0.299f + gCache[o].y*0.587f + gCache[o].z*0.114f;
		float kGray = gCache[k].x*0.299f + gCache[k].y*0.587f + gCache[k].z*0.114f;

		unitBilateralblur += (kGray / oGray);

		blurColor += gWeights[i + gBlurRadius] * (kGray / oGray) * gCache[k];
	}
	//��λ��Ȩ��
	//unitBilateralblur = 1 / unitBilateralblur;

	gOutput[dispatchThreadID.xy] = blurColor;

	
	/*[unroll]
	for(int i = -gBlurRadius; i <= gBlurRadius; ++i)
	{
		int k = groupThreadID.x + gBlurRadius + i;
		
		blurColor += gWeights[i+gBlurRadius]*gCache[k];
	}
	
	gOutput[dispatchThreadID.xy] = blurColor;*/
}

[numthreads(1, N, 1)]
void VertBlurCS(int3 groupThreadID : SV_GroupThreadID,
				int3 dispatchThreadID : SV_DispatchThreadID)
{
	//
	// Fill local thread storage to reduce bandwidth.  To blur 
	// N pixels, we will need to load N + 2*BlurRadius pixels
	// due to the blur radius.
	//
	
	// This thread group runs N threads.  To get the extra 2*BlurRadius pixels, 
	// have 2*BlurRadius threads sample an extra pixel.
	if(groupThreadID.y < gBlurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
		int y = max(dispatchThreadID.y - gBlurRadius, 0);
		gCache[groupThreadID.y] = gInput[int2(dispatchThreadID.x, y)];
	}
	if(groupThreadID.y >= N-gBlurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
		int y = min(dispatchThreadID.y + gBlurRadius, gInput.Length.y-1);
		gCache[groupThreadID.y+2*gBlurRadius] = gInput[int2(dispatchThreadID.x, y)];
	}
	
	// Clamp out of bound samples that occur at image borders.
	gCache[groupThreadID.y+gBlurRadius] = gInput[min(dispatchThreadID.xy, gInput.Length.xy-1)];


	// �߳����е������߳�ִ�е��ⲽ���gCache������ؾͻ������ϣ�Ҫע��ù����ڴ�ֻ�����߳�������Ч
	GroupMemoryBarrierWithGroupSync();
	
	//
	// Now blur each pixel.
	//

	float4 blurColor = float4(0, 0, 0, 0);

	float unitBilateralblur = 0.f;    //˫���˲�����λ��

	//��λ��Ȩ��
	unitBilateralblur = 1 / unitBilateralblur;

	[unroll]
	for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
	{
		float o = groupThreadID.y + gBlurRadius;    //�������ص�y����λ��
		int k = groupThreadID.y + gBlurRadius + i;      //groupThreadID.y �����߳����е�yλ��,�������256�����ص��߳����е�y������

		//������ص�Ļҽ�ֵ
		float oGray = gCache[o].x*0.299f + gCache[o].y*0.587f + gCache[o].z*0.114f;
		float kGray = gCache[k].x*0.299f + gCache[k].y*0.587f + gCache[k].z*0.114f;

		unitBilateralblur += (kGray / oGray);

		blurColor += gWeights[i + gBlurRadius] * (kGray / oGray) * gCache[k] * 0.9f;
	}
	//��λ��Ȩ��
	//unitBilateralblur = 1 / unitBilateralblur;

	gOutput[dispatchThreadID.xy] = blurColor;
	
	//[unroll]
	//for(int i = -gBlurRadius; i <= gBlurRadius; ++i)
	//{
	//	int k = groupThreadID.y + gBlurRadius + i;      //groupThreadID.y �����߳����е�yλ��,�������256�����ص��߳����е�y������
	//	
	//	blurColor += gWeights[i+gBlurRadius]*gCache[k];
	//}
	//
	//gOutput[dispatchThreadID.xy] = blurColor;
}

technique11 HorzBlur
{
    pass P0
    {
		SetVertexShader( NULL );
        SetPixelShader( NULL );
		SetComputeShader( CompileShader( cs_5_0, HorzBlurCS() ) );
    }
}

technique11 VertBlur
{
    pass P0
    {
		SetVertexShader( NULL );
        SetPixelShader( NULL );
		SetComputeShader( CompileShader( cs_5_0, VertBlurCS() ) );
    }
}
