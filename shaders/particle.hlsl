struct  PSInput
{
	float4 position: SV_POSITION;
	float4 color :COLOR;
};

Texture2D P;
sampler pointSampler: register(s0);

cbuffer Constants
{
	uint size;
};

PSInput vs(uint vertexId: SV_VertexID)
{
	PSInput o;
	float x = float(vertexId % size) / float(size);
	float y = float(vertexId / size) / float(size);
	o.position = P.SampleLevel(pointSampler, float2(x,y),0);
	o.position.y *= -1;
	o.position.w = 1;
	float brightness = o.position.z;
	o.color = float4(0, brightness * 0.5 + 0.1, brightness+ 0.1,0);
	return o;
}

const static float scale = 0.01f;


half4 ps(PSInput IN):SV_Target
{
	return IN.color;
}