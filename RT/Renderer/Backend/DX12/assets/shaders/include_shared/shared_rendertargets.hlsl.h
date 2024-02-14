#pragma once

// ------------------------------------------------------------------
// This file defines a macro that is used on the HLSL and C++ side
// to generate required code associated with render targets, so that
// it is easy to add/remove/change them.

// -- name ----------------------------- reg --- 1/w -- 1/h -- output/render? ---- type - format --------------------------
#define RT_RENDER_TARGETS(_) \
	_(color,                               0,     1,     1,	   false,			   float4, DXGI_FORMAT_R16G16B16A16_FLOAT) \
	_(albedo,                              1,     1,     1,	   false,			   float4, DXGI_FORMAT_R16G16B16A16_FLOAT) \
	_(emissive,                            2,     1,     1,	   false,			   float4, DXGI_FORMAT_R16G16B16A16_FLOAT) \
	_(diff,                                3,     1,     1,	   false,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(diff_hist,                           4,     1,     1,	   false,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(diff_denoise_ping,                   5,     1,     1,	   false,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(diff_denoise_pong,                   6,     1,     1,	   false,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(diff_stable,                         7,     1,     1,	   false,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(diff_stable_hist,                    8,     1,     1,	   false,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(spec,                                9,     1,     1,	   false,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(spec_hist,                          10,     1,     1,	   false,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(spec_denoise_ping,                  11,     1,     1,	   false,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(spec_denoise_pong,				  12,     1,     1,	   false,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(spec_stable,                        13,     1,     1,	   false,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(spec_stable_hist,                   14,     1,     1,	   false,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(normal,							  15,     1,     1,	   false,			   float2, DXGI_FORMAT_R16G16_FLOAT) \
	_(normal_prev,						  16,     1,     1,	   false,			   float2, DXGI_FORMAT_R16G16_FLOAT) \
	_(depth,							  17,     1,     1,	   false,			   float, DXGI_FORMAT_R32_FLOAT) \
	_(depth_prev,						  18,     1,     1,	   false,			   float, DXGI_FORMAT_R32_FLOAT) \
	_(view_dir,						      19,     1,     1,	   false,			   float4, DXGI_FORMAT_R16G16B16A16_FLOAT) \
	_(view_dir_prev,				      20,     1,     1,	   false,			   float4, DXGI_FORMAT_R16G16B16A16_FLOAT) \
	_(restir_yM,						  21,     1,     1,	   false,			   uint2,  DXGI_FORMAT_R16G16_UINT) \
	_(restir_weights,					  22,     1,     1,	   false,			   float2, DXGI_FORMAT_R16G16_FLOAT) \
	_(visibility_prim,                    23,     1,     1,	   false,			   uint2,  DXGI_FORMAT_R32G32_UINT) \
	_(visibility_bary,                    24,     1,     1,	   false,			   float2, DXGI_FORMAT_R16G16_FLOAT) \
	_(metallic,					          25,     1,     1,	   false,			   float,  DXGI_FORMAT_R8_UNORM) \
	_(metallic_prev,					  26,     1,     1,	   false,			   float,  DXGI_FORMAT_R8_UNORM) \
	_(roughness,						  27,     1,     1,	   false,			   float,  DXGI_FORMAT_R8_UNORM) \
	_(roughness_prev,					  28,     1,     1,	   false,			   float,  DXGI_FORMAT_R8_UNORM) \
	_(material,							  29,	  1,	 1,	   false,			   uint,   DXGI_FORMAT_R16_UINT) \
	_(material_prev,					  30,	  1,	 1,	   false,			   uint,   DXGI_FORMAT_R16_UINT) \
	_(history_length,                     31,     1,     1,	   false,			   uint2,  DXGI_FORMAT_R8G8_UINT)          \
	_(motion,                             32,     1,     1,	   false,			   float4, DXGI_FORMAT_R16G16B16A16_FLOAT) \
	_(moments,                            33,     1,     1,	   false,			   float4, DXGI_FORMAT_R16G16B16A16_FLOAT) \
	_(moments_denoise_ping,               34,     1,     1,	   false,			   float4, DXGI_FORMAT_R16G16B16A16_FLOAT) \
	_(moments_denoise_pong,               35,     1,     1,	   false,			   float4, DXGI_FORMAT_R16G16B16A16_FLOAT) \
	_(moments_hist,                       36,     1,     1,	   false,			   float4, DXGI_FORMAT_R16G16B16A16_FLOAT) \
	_(denoise_pong,                       37,     1,     1,	   false,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(taa_result,                         38,     1,     1,	   true,			   float4, DXGI_FORMAT_R16G16B16A16_FLOAT) \
	_(taa_history,                        39,     1,     1,	   true,			   float4, DXGI_FORMAT_R16G16B16A16_FLOAT) \
	_(bloom_pong,                         40,     4,     2,	   true,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(bloom_prepass,                      41,     2,     2,	   true,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(bloom0,                             42,     4,     4,	   true,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(bloom1,                             43,     8,     8,	   true,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(bloom2,                             44,    16,    16,	   true,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(bloom3,                             45,    32,    32,	   true,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(bloom4,                             46,    64,    64,	   true,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(bloom5,                             47,   128,   128,	   true,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(bloom6,                             48,   256,   256,	   true,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(bloom7,                             49,   512,   512,	   true,			   float4, DXGI_FORMAT_R11G11B10_FLOAT) \
	_(postfx,							  50,     1,     1,	   true,			   float4, DXGI_FORMAT_R8G8B8A8_UNORM) \
	_(resolve,							  51,     1,     1,	   true,			   float4, DXGI_FORMAT_R8G8B8A8_UNORM) \
	_(color_reference,                    52,     1,     1,	   false,			   float4, DXGI_FORMAT_R32G32B32A32_FLOAT) \
	_(color_final,                        53,     1,     1,	   true,			   float4, DXGI_FORMAT_R8G8B8A8_UNORM) \
	_(debug,							  54,     1,     1,	   false,			   float4, DXGI_FORMAT_R8G8B8A8_UNORM) \
	_(fsr2_reactive_mask,				  55,     1,     1,	   false,			   float, DXGI_FORMAT_R8_UNORM)
