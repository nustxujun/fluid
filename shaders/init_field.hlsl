#include "common.hlsl"

cbuffer Constants
{
	float2 pos;
	float2 force;
	float2 color;
	float intensity;
	float deltaTime;
};

Texture2D field;
sampler linearSampler: register(s0);
sampler pointSampler: register(s1);
half4 ps(QuadInput input) :SV_Target
{
	half4 c = field.SampleLevel(pointSampler, input.uv, 0);
	float2 deltapos = pos - input.position.xy;
	if (dot(deltapos, deltapos) < 100)
	{
		//c.xy += -deltapos * intensity * deltaTime;
		c.xy += force * intensity * deltaTime;
		//c.xy = half2(100,0) * intensity;
		c.zw += color * deltaTime;
	}



	return c;
}


