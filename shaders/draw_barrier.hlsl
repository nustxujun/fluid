#include "common.hlsl"

cbuffer PSConstants
{
	float2 texelSize;
	float2 pos;
	float radiusSQ;
};

half4 ps(QuadInput input) :SV_Target
{
	float2 vec = pos - input.position.xy;
	float2 uv = input.uv;
	if ( (dot(vec, vec) < radiusSQ )|| 
		uv.x <= texelSize.x || uv.y <= texelSize.y || uv.x >= (1.0f - texelSize.x) || uv.y >= (1.0f - texelSize.y))
	{
		return 1;
	}
	

	return 0;
}


