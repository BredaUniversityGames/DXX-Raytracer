#include "include/common.hlsl"

//------------------------------------------------------------------------

[numthreads(GROUP_X, GROUP_Y, 1)]
void CompositeCS(COMPUTE_ARGS)
{
	EARLY_OUT

	int2 co = pixel_pos.xy;

	float3 albedo   = img_albedo  [co].rgb;
	float3 emissive = img_emissive[co].rgb;

	float3 diff;

	[branch]
	if (tweak.svgf_stabilize) diff = img_diff_stable[co].rgb;
	else                                               diff = img_diff_denoise_ping[co].rgb;

	float3 spec;

	[branch]
	if (tweak.svgf_stabilize) spec = img_spec_stable[co].rgb;
	else                                               spec = img_spec_denoise_ping[co].rgb;

	float metallic = img_metallic[co].x;
	float roughness = img_roughness[co].x;
	uint  material_index = img_material[co].x;

	[branch]
	if (g_global_cb.debug_flags & GlobalDebugFlags_HideDirectLighting)
		diff.rgb = 0;

	if (g_global_cb.debug_flags & GlobalDebugFlags_HideSpecularLighting)
		spec.rgb = 0;

	if ((g_global_cb.debug_flags & GlobalDebugFlags_HideDirectLighting) &&
		(g_global_cb.debug_flags & GlobalDebugFlags_HideSpecularLighting))
	{
		// fullbright so you can see something
		diff.rgb = 1;
	}

	float3 composite_color;

	[branch]
	if (tweak.svgf_enabled)
	{
		float3 reflectivity = lerp(0.04, 0.01 + albedo, metallic);
		composite_color = albedo*diff + reflectivity*spec + emissive;
	}
	else
	{
		composite_color = albedo*diff + spec + emissive;
	}

	float3 out_color = composite_color;
	img_color[co].rgb = out_color;
}
