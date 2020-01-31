#include "common.hlsl"

cbuffer Batch
{
	float2 texelSize;
	float deltaTime;
	float force;
	int count;
};


Texture2D V;
Texture2D Barrier;
sampler linearSampler: register(s0);
sampler pointSampler: register(s1);

half4 samp(half2 uv, sampler s)
{
	return V.SampleLevel(s, uv, 0);
}

half2 getVel(half2 uv, sampler s)
{
	half4 c = samp(uv,s);
	return c.xy ;
}

half4 ps(QuadInput input):SV_Target
{
	half2 old = getVel(input.uv,pointSampler);
	half2 uv = input.uv - old * texelSize * deltaTime;
	half4 v = samp(uv, linearSampler) ;
	//if (input.uv.x <0.1 && input.uv.y > 0.49f && input.uv.y < 0.51f)
	//{
	//	v = half4(force,0,1,0) ;
	//}

	if (input.uv.x <0.1 && (int(input.uv.y * 64) % count) == 0)
	{
		v.xy = half2(force ,0)  ;
		v.zw = half2(1,0);
	}

	if (input.uv.x < 0.1 && (int(input.uv.y * 64) % count) == (count / 2))
	{
		v.xy = half2(force * 2, 0) ;
		v.zw = half2(0, 1);
	}

	half4 b = Barrier.SampleLevel(pointSampler, input.uv,0);
	if (b.x > 0 )
	{
		v = 0;
	}

	return v;
}
