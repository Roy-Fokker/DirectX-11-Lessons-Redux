Texture2D textureObj;
SamplerState sampleState;

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
	float2 uv : TEXCOORD0;
	float3 eye_pos : TEXCOORD1;
};

float4 main(PS_INPUT input) : SV_TARGET
{
	float4 color;

	color = textureObj.Sample(sampleState, input.uv);

	return color;
}