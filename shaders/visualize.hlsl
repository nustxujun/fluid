#include "common.hlsl"

Texture2D field;
Texture2D barrier;

sampler linearSampler: register(s0);
sampler pointSampler: register(s1);

cbuffer Batch
{
	bool velocity;
};

half4 ps(QuadInput input):SV_Target
{
	half4 d = field.Sample(pointSampler, input.uv);
	half4 b = barrier.Sample(pointSampler, input.uv);
	if (!velocity)
	{
		d = (clamp(d  , -1, 1)) * 0.5 + 0.5  ;
		return half4(d.zw,b.x + 0.5f,1);
	}
	else
	{
		d = (clamp(d  * 0.01f, -1, 1)) * 0.5 + 0.5  ;
		return half4(d.xy  , b.x + 0.5f,1);
	}

}


