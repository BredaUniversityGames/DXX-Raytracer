#pragma once
#include "WinIncludes.h"

enum FsrMode
{
	FSR_MODE_NO_UPSCALING,
	FSR_MODE_QUALITY,
	FSR_MODE_BALANCED,
	FSR_MODE_PERFORMANCE,
	FSR_MODE_ULTRA_PERFORMANCE,
	FSR_MODE_NUM_MODES
};

namespace FSR2
{

	void Init();
	void Exit();

	void Dispatch(ID3D12CommandList* command_list, ID3D12Resource* rt_color, ID3D12Resource* rt_depth,
		ID3D12Resource* rt_motion_vector, ID3D12Resource* rt_reactive, ID3D12Resource* rt_output, uint32_t render_width, uint32_t render_height,
		float camera_jitter_x, float camera_jitter_y, float camera_near, float camera_far, float camera_vfov_angle, float delta_time, bool reset);

	void AdjustRenderResolutionForFSRMode(uint32_t output_width, uint32_t output_height, uint32_t& render_width, uint32_t& render_height);
	RT_Vec2 GetMipBiasForFSRMode(uint32_t output_width, uint32_t output_height, uint32_t render_width, uint32_t render_height);

	void OnWindowResize(uint32_t width, uint32_t height);

}