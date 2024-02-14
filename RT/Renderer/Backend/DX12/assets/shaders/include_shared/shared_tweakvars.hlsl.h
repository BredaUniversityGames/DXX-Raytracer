// ------------------------------------------------------------------------------------------------------------
// Tweak variables file.
// Each tweak variable declared is accessible in HLSL and C++ through the TweakVars struct:
// TweakVars vars;
// vars.enable_pbr = false;
// vars.svgf_depth_sigma = 420;
//
// The following are supported:
// TWEAK_CATEGORY_BEGIN(name)                                              // begin a category
// TWEAK_CATEGORY_END()                                                    // ends a category 
//
// TWEAK_BOOL(name, default_value)                                         // simple checkbox
// TWEAK_INT(name, default_value, min, max)                                // int slider
// TWEAK_FLOAT(name, default_value, min, max)                              // float slider
// TWEAK_COLOR(name, default_value)                                        // RGB color picker
// TWEAK_OPTIONS(name, default_value, list of options as name-value pairs) // dropdown box

TWEAK_BOOL("Vsync", vsync, true)

TWEAK_CATEGORY_BEGIN("Camera");
// ---------- name ----------------------- variable ----------------- default - min - max ---------------------------
TWEAK_FLOAT  ("FOV",					   fov,						  60.0,		60.0, 90.0)
// ------------------------------------------------------------------------------------------------------------------
TWEAK_CATEGORY_END();

TWEAK_CATEGORY_BEGIN("Pathtracer")
// ---------- name ----------------------- variable ----------------- default - min - max ---------------------------
TWEAK_BOOL   ("Freezeframe",               freezeframe,               false)
TWEAK_BOOL   ("Enable PBR",                enable_pbr,                true)
TWEAK_BOOL   ("Enable Pathtracing",        enable_pathtracing,        true)
TWEAK_COLOR  ("Ambient Color",             ambient_color,             (0.05f, 0.07f, 0.1f))
TWEAK_BOOL   ("Importance Sample BRDF",    importance_sample_brdf,    true)
TWEAK_FLOAT  ("Direct Specular Threshold", direct_specular_threshold, 0.25,     0,    1)
TWEAK_OPTIONS("RIS",                       ris,                       2, "Off", "Subset", "Full")
TWEAK_BOOL   ("RIS Indirect",              ris_indirect,              true)
TWEAK_INT    ("RIS SPP",                   ris_spp,                   4,        1,    16)
TWEAK_BOOL   ("Use Oren-Nayar BRDF",       use_oren_nayar_brdf,       false)
TWEAK_BOOL   ("Path-space Regularization", path_space_regularization, true)
TWEAK_BOOL   ("Object Motion Vectors",     object_motion_vectors,     true)
TWEAK_BOOL   ("Enable Normal Maps",        enable_normal_maps,        true)
TWEAK_BOOL   ("Ignore Me...",              smooth_textures,           false)
TWEAK_OPTIONS("Base Color Sampler",        albedo_sample_linear,             0, "Point", "Linear")
TWEAK_OPTIONS("Normal Map Sampler",        normal_sample_linear,             0, "Point", "Linear")
TWEAK_OPTIONS("Metallic/Roughness Sampler",metallic_roughness_sample_linear, 0, "Point", "Linear")
TWEAK_BOOL   ("Override Materials",        override_materials,        false)
TWEAK_FLOAT  ("Override Metallic",         override_metallic,         0,        0,    1)
TWEAK_FLOAT  ("Override Roughness",        override_roughness,        0.3,      0,    1)
// ------------------------------------------------------------------------------------------------------------------
TWEAK_CATEGORY_END()


TWEAK_CATEGORY_BEGIN("SVGF")
// ---------- name ------------------------ variable ----------------- default - min - max ---------------------------
TWEAK_BOOL   ("Enabled",                    svgf_enabled,              true)
// TWEAK_BOOL   ("Prepass",                   svgf_prepass,              true)
TWEAK_FLOAT  ("Luma Sigma (Diffuse)",       svgf_luma_sigma_diff,      48,       0,    128)
TWEAK_FLOAT  ("Luma Sigma (Specular)",      svgf_luma_sigma_spec,      10,       0,    64)
TWEAK_FLOAT  ("Depth Sigma",                svgf_depth_sigma,          2,        0,    8)
TWEAK_FLOAT  ("Normal Sigma",               svgf_normal_sigma,         8,        0,    64)
TWEAK_FLOAT  ("Metallic Sigma",             svgf_metallic_sigma,       0.25,     0,    1)
TWEAK_FLOAT  ("Jitter",                     svgf_jitter,               1,        0,    1)
TWEAK_INT    ("History Fix Frames",         svgf_history_fix_frames,   2,        0,    8)
TWEAK_FLOAT  ("History Fix Scale",          svgf_history_fix_scale,    4,        0,    8)
TWEAK_INT    ("Max History (Diffuse)",      svgf_max_hist_len_diff,    4,        1,    64)
TWEAK_INT    ("Max History (Specular)",     svgf_max_hist_len_spec,    4,        1,    64)
TWEAK_BOOL   ("Stabilize",                  svgf_stabilize,            true)
TWEAK_BOOL   ("Stabilize Sharp (Diffuse)",  diff_stabilize_sharp,      true)
TWEAK_FLOAT  ("Stabilize Gamma (Diffuse)",  diff_stabilize_gamma,      2.0,      0.5,  4.0)
TWEAK_FLOAT  ("Stabilize Alpha (Diffuse)",  diff_stabilize_alpha,      0.01,     0.0,  1.0)
TWEAK_BOOL   ("Stabilize Sharp (Specular)", spec_stabilize_sharp,      true)
TWEAK_FLOAT  ("Stabilize Gamma (Specular)", spec_stabilize_gamma,      2.0,      0.5,  4.0)
TWEAK_FLOAT  ("Stabilize Alpha (Specular)", spec_stabilize_alpha,      0.01,     0.0,  1.0)
TWEAK_BOOL   ("Anti-Lag",                   svgf_antilag,              true)
// -------------------------------------------------------------------------------------------------------------------
TWEAK_CATEGORY_END()


TWEAK_CATEGORY_BEGIN("Upscaling & Anti-aliasing")
// ---------- name ----------------------- variable ----------------- default - min - max ---------------------------
TWEAK_OPTIONS("Upscaling & AA mode", upscaling_aa_mode, 1, "Off", "TAA", "AMD FSR 2.2")
TWEAK_OPTIONS("FSR2 mode", amd_fsr2_mode, 1, "No Upscaling", "Quality", "Balanced", "Performance", "Ultra performance")
// ------------------------------------------------------------------------------------------------------------------
TWEAK_CATEGORY_END();


TWEAK_CATEGORY_BEGIN("TAA")
// ---------- name ----------------------- variable ----------------- default - min - max ---------------------------
TWEAK_BOOL   ("Per-pixel Jitter",          taa_per_pixel_jitter,      false)
TWEAK_OPTIONS("Neighborhood Mode",         taa_neighborhood_mode,     TaaNeighborhoodMode_VarianceClip, "Off", "Clamp", "Clip", "Variance Clip")
TWEAK_BOOL   ("Tonemapped Blend",          taa_tonemapped_blend,      true)
TWEAK_BOOL   ("Catmull-Rom",               taa_catmull_rom,           true)
TWEAK_FLOAT  ("VClip Gamma",               taa_variance_clip_gamma,   1,        0.5,   4)
TWEAK_FLOAT  ("Feedback Min",              taa_feedback_min,          0.8,      0,     1)
TWEAK_FLOAT  ("Feedback Max",              taa_feedback_max,          0.95,     0,     1)
// ------------------------------------------------------------------------------------------------------------------
TWEAK_CATEGORY_END()


TWEAK_CATEGORY_BEGIN("Motion Blur")
// ---------- name ----------------------- variable ----------------- default - min - max ---------------------------
TWEAK_INT    ("Quality",                   motion_blur_quality,       1,        0,    4)
TWEAK_FLOAT  ("Jitter",                    motion_blur_jitter,        1,        0,    1)
TWEAK_FLOAT  ("Nonlinearity",              motion_blur_curve,         0.5,      0,    1)
// ------------------------------------------------------------------------------------------------------------------
TWEAK_CATEGORY_END()

TWEAK_CATEGORY_BEGIN("Bloom")
// ---------- name ----------------------- variable ----------------- default - min - max ---------------------------
TWEAK_FLOAT  ("Amount",                    bloom_amount,              0.025,    0,    1)
TWEAK_OPTIONS("Blend Mode",                bloom_blend_mode, 0, "Lerp", "Additive", "Subtractive")
TWEAK_FLOAT  ("Threshold",                 bloom_threshold,           0.0,      0,    4)
// ------------------------------------------------------------------------------------------------------------------
TWEAK_CATEGORY_END()

TWEAK_CATEGORY_BEGIN("Tonemap")
// ---------- name ----------------------- variable ----------------- default - min - max ---------------------------
TWEAK_FLOAT("Exposure", exposure, 0.1, -2, 2)
TWEAK_FLOAT("Linear Section", tonemap_linear_section, 0.25, 0.0, 1.0)
TWEAK_FLOAT("Whitepoint", tonemap_whitepoint, 8.0, 1.0, 16.0)
TWEAK_FLOAT("Hue Shift", tonemap_hue_shift, 0.7, 0.0, 1.0)
// ------------------------------------------------------------------------------------------------------------------
TWEAK_CATEGORY_END()

TWEAK_CATEGORY_BEGIN("Post-Processing")
// ---------- name ----------------------- variable ----------------- default - min - max ---------------------------
TWEAK_FLOAT  ("Sharpen",                   sharpen_amount,            2.0,      0,    8)
TWEAK_FLOAT  ("Gamma",                     gamma,                    -0.3,     -1,    1)
TWEAK_FLOAT  ("White Level",               white_level,               1.0,      0,    1)
TWEAK_FLOAT  ("Black Level",               black_level,               0.0,      0,    1)
TWEAK_FLOAT  ("Vignette Scale",            vignette_scale,            0.875,    0.5,  1.5)
TWEAK_FLOAT  ("Vignette Strength",         vignette_strength,         0.75,     0,    1)
// ------------------------------------------------------------------------------------------------------------------
TWEAK_CATEGORY_END()

TWEAK_CATEGORY_BEGIN("Debug render")
// ---------- name ----------------------- variable ----------------- default - min - max ---------------------------
TWEAK_FLOAT  ("Debug render blend factor", debug_render_blend_factor, 1.0,      0.0,  1.0)
// ------------------------------------------------------------------------------------------------------------------
TWEAK_CATEGORY_END()

TWEAK_CATEGORY_BEGIN("Mipmaps")
// ---------- name ----------------------- variable ----------------- default - min - max ---------------------------
TWEAK_INT    ("Mip Bias",				   mip_bias,				  -1,		-10,  10)
TWEAK_FLOAT  ("Secondary bounce bias",	   secondary_bounce_bias,	  0.0005,	0.00001, 1.0)
TWEAK_FLOAT  ("Angle cutoff",			   angle_cutoff,			  0.125,    0.0000001, 1.0)
// ------------------------------------------------------------------------------------------------------------------
TWEAK_CATEGORY_END();
