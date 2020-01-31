#include "common.hlsl"

cbuffer Batch
{
	float2 halfTexelSize;
	float deltaTime;
};


Texture2D V;
Texture2D P;

sampler linearSampler: register(s0);
sampler pointSampler: register(s1);


half4 ps(QuadInput input) :SV_Target
{
	float2 uv = input.uv;
	half L = P.SampleLevel(pointSampler, uv, 0, int2(-1,0)).x;
	half R = P.SampleLevel(pointSampler, uv, 0, int2(1, 0)).x;
	half T = P.SampleLevel(pointSampler, uv, 0, int2(0, -1)).x;
	half B = P.SampleLevel(pointSampler, uv, 0, int2(0, 1)).x;

	half4 v = V.SampleLevel(pointSampler, uv, 0);
	half2 grad = float2(R - L, B - T);
	//return half4(v.xy - grad * halfTexelSize,v.zw);
	return half4(v.xy - grad,v.zw);
}
