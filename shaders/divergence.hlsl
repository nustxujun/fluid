#include "common.hlsl"

cbuffer Batch
{
	float2 halfTexelSize;
	float deltaTime;
};


Texture2D V;
Texture2D Barrier;
sampler linearSampler: register(s0);
sampler pointSampler: register(s1);

half4 ps(QuadInput input) :SV_Target
{
	float2 uv = input.uv;
	half b = Barrier.SampleLevel(pointSampler, uv, 0).x;
	if (b > 0)
		return 0;
	half L = V.SampleLevel(pointSampler, uv, 0, int2(-1,0)).x;
	half R = V.SampleLevel(pointSampler, uv, 0, int2(1, 0)).x;
	half T = V.SampleLevel(pointSampler, uv, 0, int2(0, -1)).y;
	half B = V.SampleLevel(pointSampler, uv, 0, int2(0, 1)).y;

	//return (R - L) * halfTexelSize.x + (B - T) * halfTexelSize.y;
	return (R - L  + B - T) * 0.5f;

}
