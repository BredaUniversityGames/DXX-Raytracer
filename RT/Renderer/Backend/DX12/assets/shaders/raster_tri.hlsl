// ------------------------------------------------------
// Vertex shader

struct VertexInput
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD;
	float4 Color : COLOR;
	uint TextureIndex : TEXTURE_INDEX;
};

struct VertexShaderOutput
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
	float4 Color : COLOR;
	uint TextureIndex : TEXTURE_INDEX;
};

VertexShaderOutput VertexShaderEntry(VertexInput IN)
{
	VertexShaderOutput OUT;

	OUT.Position = float4(IN.Position, 1.0f);
	OUT.TexCoord = IN.TexCoord;
	OUT.Color = IN.Color;
	OUT.TextureIndex = IN.TextureIndex;

	return OUT;
}

// ------------------------------------------------------
// Pixel shader

Texture2D g_bindless_srvs[] : register(t0);
SamplerState g_sampler : register(s0);

float3 ConvertToLinear(float3 x)
{
	return select(x < 0.04045f, x / 12.92, pow((x + 0.055) / 1.055, 2.4));
}

float4 PixelShaderEntry(VertexShaderOutput IN) : SV_Target
{
	float4 sampled_color = g_bindless_srvs[NonUniformResourceIndex(IN.TextureIndex)].SampleLevel(g_sampler, IN.TexCoord, 0);
	return IN.Color * sampled_color;
}
