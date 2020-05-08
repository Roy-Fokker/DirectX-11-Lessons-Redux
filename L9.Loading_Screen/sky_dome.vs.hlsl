cbuffer frame_buffer : register(b0)
{
	matrix projection;
}

cbuffer object_buffer : register(b1)
{
	matrix view;
	float3 eye_pos;
}

struct VS_INPUT
{
	float4 pos : POSITION;
	float3 nor : NORMAL;
	float2 uv : TEXCOORD;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float3 uvw : TEXCOORD0;
};

float4x4 matrix_identity()
{
	return float4x4
	(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);
}

float4x4 matrix_scale(float4x4 m, float3 v)
{
	float x = v.x, y = v.y, z = v.z;

	m[0][0] *= x;
	m[0][1] *= y;
	m[0][2] *= z;
	m[1][0] *= x;
	m[1][1] *= y;
	m[1][2] *= z;
	m[2][0] *= x;
	m[2][1] *= y;
	m[2][2] *= z;
	m[3][0] *= x;
	m[3][1] *= y;
	m[3][2] *= z;

	return m;
}

float4x4 matrix_sky_dome_wrld(float4x4 m, float3 v)
{
	float x = v.x, y = v.y, z = v.z;
	
	m[3][0] = x;
	m[3][1] = y;
	m[3][2] = z;
	
	return m;
}

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	output.uvw = normalize(input.pos.xyz);
	
	matrix sky_dome_wrld = matrix_scale(matrix_identity(), float3(100.0f, 100.0f, 100.0f));
	sky_dome_wrld = matrix_sky_dome_wrld(sky_dome_wrld, eye_pos);

	input.pos.w = 1.0f;
	
	output.pos = mul(input.pos, sky_dome_wrld);
	output.pos = mul(output.pos, view);
	output.pos = mul(output.pos, projection);


	return output;
}