// ------------------------------------------------------
// Vertex shader

struct CViewProj
{
	float4x4 mat;
};

struct CFarPlane
{
	float plane;
};

ConstantBuffer<CViewProj> c_view_projection : register(b0);
ConstantBuffer<CFarPlane> c_far_plane : register(b1);

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

	OUT.Position = mul(c_view_projection.mat, float4(IN.Position, 1.0f));
	OUT.Color = IN.Color;

	return OUT;
}

// ------------------------------------------------------
// Pixel shader

struct PixelShaderOutput
{
#ifdef DEPTH_ENABLED
	float Depth : SV_DEPTH;
#endif
	float4 Color : SV_TARGET;
};

PixelShaderOutput PixelShaderEntry(VertexShaderOutput IN)
{
	PixelShaderOutput OUT;
	
#ifdef DEPTH_ENABLED
	OUT.Depth = (IN.Position.z / c_far_plane.plane) * IN.Position.w;
#endif
	OUT.Color = IN.Color;

	return OUT;
}
