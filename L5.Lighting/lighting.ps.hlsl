Texture2D textureObj;
SamplerState sampleState;

cbuffer light_buffer : register (b0)
{
	float4 diffuse;
	float4 ambient;
	float3 light_pos;
	float specular_power;
	float4 specular;
};

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
	float2 uv : TEXCOORD0;
	float3 eye_pos : TEXCOORD1;
};

float4 main(PS_INPUT input) : SV_TARGET
{
	float light_intensity = saturate(dot(input.nor, light_pos));

	float3 reflection = normalize(2 * light_intensity * input.nor - light_pos);
	float3 view_dir = input.eye_pos - input.pos;
	float4 spec_col = specular * pow(saturate(dot(reflection, view_dir)), specular_power);

	float4 tex_color = textureObj.Sample(sampleState, input.uv);

	float4 color = tex_color * ambient;
	color += saturate(light_intensity * diffuse * tex_color);
	color.w = tex_color.w;

	color = saturate(color + spec_col);

	return color;
}