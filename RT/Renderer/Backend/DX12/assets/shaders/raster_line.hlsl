// ------------------------------------------------------
// Vertex shader

struct VertexShaderInput
{
	float3 Position : POSITION;
	float4 Color : COLOR;
};

struct VertexShaderOutput
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
};

VertexShaderOutput VertexShaderEntry(VertexShaderInput IN)
{
	VertexShaderOutput OUT;

	OUT.Position = float4(IN.Position, 1.0f);
	OUT.Color = IN.Color;

	return OUT;
}

// ------------------------------------------------------
// Pixel shader

float4 PixelShaderEntry(VertexShaderOutput IN) : SV_TARGET
{
	return IN.Color;
}
