#include "common.hlsl"

cbuffer Batch
{
	float2 texelSize;
	float deltaTime;
};

cbuffer Params
{
	float alpha;
	float invBeta;
};


Texture2D X;
Texture2D B;

sampler linearSampler: register(s0);
sampler pointSampler: register(s1);

half4 ps(QuadInput input) :SV_Target
{
	float2 uv = input.uv;
	half4 C1 = X.SampleLevel(pointSampler, uv, 0, int2(-1,0));
	half4 C2 = X.SampleLevel(pointSampler, uv, 0, int2(1, 0));
	half4 C3 = X.SampleLevel(pointSampler, uv, 0, int2(0, 1));
	half4 C4 = X.SampleLevel(pointSampler, uv, 0, int2(0, -1));

	half4 C0 = B.SampleLevel(pointSampler, uv, 0);

	return (C1 + C2 + C3 + C4 + alpha * C0) * invBeta;
}
