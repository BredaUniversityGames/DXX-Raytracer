#include "WinIncludes.h"

namespace FSR2
{

	void Init();
	void Exit();

	void Dispatch(ID3D12CommandList* command_list, ID3D12Resource* rt_color, ID3D12Resource* rt_depth,
		ID3D12Resource* rt_motion_vector, ID3D12Resource* rt_output, uint32_t render_width, uint32_t render_height,
		float camera_jitter_x, float camera_jitter_y, float camera_near, float camera_far, float camera_vfov_angle, float delta_time, bool reset);

}