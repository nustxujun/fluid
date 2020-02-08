struct VSInput
{
	float3 position: POSITION;
	//float4 userdata: COLOR;
	//float3 offset :INSTANCEPOS;
};

struct  PSInput
{
	float4 position: SV_POSITION;
	float4 color :COLOR;
};

Texture2D V;
sampler linearSampler: register(s0);

cbuffer Constants
{
	uint numInstance;
};

float4 vs(uint vertexId: SV_VertexID):SV_POSITION
{
	return float4( ((float)vertexId / (float)numInstance) * 2.0f - 1.0f,0,0,1);
}

const static float scale = 0.01f;

struct GSInput
{
	float4 pos: SV_POSITION;
};

#define LENGTH 20

[maxvertexcount(LENGTH * 2)]
void gs(point GSInput pos[1],inout LineStream<PSInput> output)
{
	float offset = pos[0].pos.x;


	PSInput o;
	float2 last = float2(offset, -1);
	for (int i = 0; i < LENGTH; ++i)
	{
		float2 uv = float2(last.x * 0.5 + 0.5, 1 - last.y);
		float4 vel = V.SampleLevel(linearSampler, uv, 0);

		float height = float(i)  * scale - 1.0f;
		o.position = float4(last,0,1);
		o.color = float4(0, float(i) / LENGTH,0,1);
		output.Append(o);
		float2 add = vel.xy * 0.001  * (pow(float(i) / LENGTH,2) + 0.1)+ float2(0,scale);
		add = normalize(add) * scale;
		last += add;
		o.position = float4(last, 0, 1);
		output.Append(o);
		output.RestartStrip();
	}
}

half4 ps(PSInput IN):SV_Target
{
	return IN.color;
}