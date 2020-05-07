cbuffer frame_buffer : register(b0)
{
	matrix viewProj;
}

cbuffer object_buffer : register(b1)
{
	matrix wrld;
	float3 eye_pos;
}

cbuffer transform_buffer : register(b2)
{
	matrix transform;
}

struct VS_INPUT
{
	float4 pos : POSITION;
	float3 nor : NORMAL;
	float2 uv : TEXCOORD;
	float4x4 inst_mat : TRANSFORM;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
	float2 uv : TEXCOORD0;
	float3 eye_pos : TEXCOORD1;
};

VS_OUTPUT main(VS_INPUT input, uint instance_id : SV_InstanceID)
{
	VS_OUTPUT output;

	input.pos.w = 1.0f;

	output.pos = mul(input.pos, input.inst_mat);
	output.pos = mul(output.pos, wrld);
	output.pos = mul(output.pos, viewProj);

	output.uv = input.uv;

	output.nor = mul(input.nor, (float3x3)input.inst_mat);
	output.nor = normalize(output.nor);

	float4 vert_pos = mul(input.pos, input.inst_mat);
	output.eye_pos = normalize(eye_pos.xyz - vert_pos.xyz);

	return output;
}