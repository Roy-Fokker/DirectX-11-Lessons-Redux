cbuffer frameBuffer : register(b0)
{
	matrix viewProj;
}

cbuffer objectBuffer : register(b1)
{
	matrix wrld;
}

cbuffer transformBuffer : register(b2)
{
	matrix transform;
}

struct VS_INPUT
{
	float4 pos : POSITION;
	float4 col : COLOR;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float4 col : COLOR;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	input.pos.w = 1.0f;

	output.pos = mul(input.pos, transform);
	output.pos = mul(output.pos, wrld);
	output.pos = mul(output.pos, viewProj);
	output.col = input.col;

	return output;
}