Texture2D textureObj;
SamplerState sampleState;

cbuffer light_buffer : register (b0)
{
	float4 diffuse;
	float4 ambient;
	float3 light_dir;
};

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
	float2 uv : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
	float light_intensity = saturate(dot(input.nor, light_dir));
	float4 tex_color = textureObj.Sample(sampleState, input.uv);

	float4 color = tex_color * ambient;
	color += saturate(light_intensity * diffuse * tex_color);
	color.w = tex_color.w;
	color = saturate(color);

	return color;
}