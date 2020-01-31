#include "common.hlsl"

cbuffer Batch
{
	float2 halfTexelSize;
	float deltaTime;
};


Texture2D frame: register(t0);
sampler linearSampler: register(s0);
sampler pointSampler: register(s1);

half4 samp(half2 uv, sampler s)
{
	return frame.SampleLevel(s, uv, 0);
}

half4 ps(QuadInput input) :SV_Target
{
	float2 uv = input.uv;
	half L = frame.SampleLevel(pointSampler, uv, 0, int2(-1, 0)).z;
	half R = frame.SampleLevel(pointSampler, uv, 0, int2(1, 0)).z;
	half T = frame.SampleLevel(pointSampler, uv, 0, int2(0, -1)).z;
	half B = frame.SampleLevel(pointSampler, uv, 0, int2(0, 1)).z;

	half4 C0 = frame.SampleLevel(pointSampler, uv, 0);

	half2 g =  half2((R - L).x , (B - T).y) * halfTexelSize ;
	return half4(C0.xy - g, C0.zw);
