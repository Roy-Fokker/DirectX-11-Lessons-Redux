TextureCube textureObj;
SamplerState sampleState;

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float3 uvw : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
	float4 color;

	color = textureObj.Sample(sampleState, input.uvw);

	return color;
}