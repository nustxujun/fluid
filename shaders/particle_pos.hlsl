#include "common.hlsl"

Texture2D P;
Texture2D V;

sampler linearSampler: register(s0);
sampler pointSampler: register(s1);

half4 ps(QuadInput input):SV_Target
{
	half4 p = P.SampleLevel(pointSampler, input.uv,0);

	half4 v = V.SampleLevel(linearSampler,p.xy * 0.5f + 0.5f,0);
	p.xy += v.xy * 0.00001f ;

	p.xy = clamp(p.xy, -1,1);
	return half4(p.xy, (abs(v.x) + abs(v.y)) * 0.01 + 0.3,0);

}


