#include "common.hlsl"

Texture2D field;
Texture2D barrier;

sampler linearSampler: register(s0);
sampler pointSampler: register(s1);

cbuffer Batch
{
	float add;
	float scale;
	float clampMin;
	float clampMax;
};

half4 ps(QuadInput input):SV_Target
{
	half4 d = field.Sample(pointSampler, input.uv);
	d = (clamp(d  , -1, 1)) * 0.5 + 0.5  ;
	half4 b = barrier.Sample(pointSampler, input.uv);
	return half4(d.zw,b.x + 0.5f,1);
	//return half4(d.xy , b.x + 0.5f,1);
}


