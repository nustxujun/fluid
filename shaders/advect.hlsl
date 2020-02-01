#include "common.hlsl"

cbuffer Constants
{
	float2 texelSize;
	float deltaTime;
	float intensity;
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

	half4 b = Barrier.SampleLevel(pointSampler, input.uv,0);
	if (b.x > 0 )
	{
		v = 0;
	}

	return v;
}
