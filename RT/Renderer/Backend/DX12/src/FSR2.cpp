#include "FSR2.h"
#include "GlobalDX.h"
#include "Resource.h"

#include "ffx_fsr2.h"
#include "dx12/ffx_fsr2_dx12.h"

#include <cmath>

using namespace RT;

static float FsrModeScalingFactors[FSR_MODE_NUM_MODES] = { 1.0f, 1.5f, 1.7f, 2.0f, 3.0f };

#define FFX_CALL(ffx_code_) \
    do { \
		FfxErrorCode ffx_code = ffx_code_; \
		if (ffx_code != FFX_OK) \
		{ \
			RT_FATAL_ERROR(RT_ArenaPrintF(&g_thread_arena, "FSR2 threw an error with code: %u", ffx_code_)); \
		} \
	} \
	while (false)

static void FSR2_MessageCallback(FfxFsr2MsgType type, const wchar_t* message)
{
	if (type == FFX_FSR2_MESSAGE_TYPE_ERROR)
	{
		// TODO: Eh.. we cant use RT_FATAL_ERROR with wide chars yet
		OutputDebugStringW(message);
		__debugbreak();
		ExitProcess(1);
	}
	else if (type == FFX_FSR2_MESSAGE_TYPE_WARNING)
	{
		OutputDebugStringW(message);
	}
}

struct Data
{
#ifdef interface
#undef interface
#endif
	ID3D12Resource* scratch_buffer;
	void* scratch_buffer_ptr;
	
	FfxFsr2ContextDescription context_desc;
	FfxFsr2Context context;
} static data;

void FSR2::Init()
{
	// FSR2 scratch buffer and interface
	size_t scratch_buffer_size = ffxFsr2GetScratchMemorySizeDX12();
	data.scratch_buffer = RT_CreateUploadBuffer(L"FSR2 scratch buffer", scratch_buffer_size);
	data.scratch_buffer->Map(0, nullptr, (void**)&data.scratch_buffer_ptr);

	FFX_CALL(ffxFsr2GetInterfaceDX12(&data.context_desc.callbacks, g_d3d.device, data.scratch_buffer_ptr, scratch_buffer_size));

	// FSR2 context description
	data.context_desc.device = ffxGetDeviceDX12(g_d3d.device);
	data.context_desc.displaySize = { g_d3d.output_width, g_d3d.output_height };
	// If dynamic resolution is enabled, this is the maximum that FSR2 will upscale to
	// If dynamic resolution is disabled, this will be the target resolution for FSR2
	data.context_desc.maxRenderSize = { g_d3d.output_width, g_d3d.output_height };
	data.context_desc.fpMessage = FSR2_MessageCallback;
	data.context_desc.flags = FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE | FFX_FSR2_ENABLE_AUTO_EXPOSURE;
	//data.context_desc.flags = FfxFsr2InitializationFlagBits;
	FFX_CALL(ffxFsr2ContextCreate(&data.context, &data.context_desc));

	data.scratch_buffer->Unmap(0, nullptr);
}

void FSR2::Exit()
{
	FFX_CALL(ffxFsr2ContextDestroy(&data.context));
}

void FSR2::Dispatch(ID3D12CommandList* command_list, ID3D12Resource* rt_color, ID3D12Resource* rt_depth,
	ID3D12Resource* rt_motion_vector, ID3D12Resource* rt_output, uint32_t render_width, uint32_t render_height,
	float camera_jitter_x, float camera_jitter_y, float camera_near, float camera_far, float camera_vfov_angle, float delta_time, bool reset)
{
	FfxFsr2DispatchDescription dispatch_desc = {};
	dispatch_desc.commandList = ffxGetCommandListDX12(command_list);
	dispatch_desc.color = ffxGetResourceDX12(&data.context, rt_color);
	dispatch_desc.depth = ffxGetResourceDX12(&data.context, rt_depth);
	dispatch_desc.motionVectors = ffxGetResourceDX12(&data.context, rt_motion_vector);
	dispatch_desc.exposure = ffxGetResourceDX12(&data.context, nullptr);
	// Masks objects that have no velocity vectors to reduce ghosting on these objects
	dispatch_desc.reactive = ffxGetResourceDX12(&data.context, nullptr);
	dispatch_desc.transparencyAndComposition = ffxGetResourceDX12(&data.context, nullptr);
	dispatch_desc.output = ffxGetResourceDX12(&data.context, rt_output);
	// Jitter offsets can be calculated using ffxFsr2GetJitterPhaseCount and ffxFsr2GetJitterOffset
	// However we can also supply our own, which we already have implemented, so we will use our own halton sequence
	dispatch_desc.jitterOffset.x = camera_jitter_x;
	dispatch_desc.jitterOffset.y = camera_jitter_y;
	dispatch_desc.motionVectorScale.x = (float)render_width;
	dispatch_desc.motionVectorScale.y = (float)render_height;
	// Used to reset the temporal accumulation in case of jump cuts
	dispatch_desc.reset = reset;
	dispatch_desc.enableSharpening = false;
	dispatch_desc.sharpness = 0.0;
	dispatch_desc.frameTimeDelta = delta_time;
	dispatch_desc.preExposure = 1.0;
	dispatch_desc.renderSize.width = render_width;
	dispatch_desc.renderSize.height = render_height;
	dispatch_desc.cameraFar = camera_far;
	dispatch_desc.cameraNear = camera_near;
	dispatch_desc.cameraFovAngleVertical = camera_vfov_angle;

	FFX_CALL(ffxFsr2ContextDispatch(&data.context, &dispatch_desc));
}

void FSR2::AdjustRenderResolutionForFSRMode(uint32_t output_width, uint32_t output_height, uint32_t& render_width, uint32_t& render_height)
{
	switch (g_d3d.amd_fsr2_mode)
	{
	case FSR_MODE_NO_UPSCALING:
	{
		render_width = output_width;
		render_height = output_height;
		break;
	}
	case FSR_MODE_QUALITY:
	{
		render_width = (uint32_t)((float)output_width / FsrModeScalingFactors[FSR_MODE_QUALITY]);
		render_height = (uint32_t)((float)output_height / FsrModeScalingFactors[FSR_MODE_QUALITY]);
		break;
	}
	case FSR_MODE_BALANCED:
	{
		render_width = (uint32_t)((float)output_width / FsrModeScalingFactors[FSR_MODE_BALANCED]);
		render_height = (uint32_t)((float)output_height / FsrModeScalingFactors[FSR_MODE_BALANCED]);
		break;
	}
	case FSR_MODE_PERFORMANCE:
	{
		render_width = (uint32_t)((float)output_width / FsrModeScalingFactors[FSR_MODE_PERFORMANCE]);
		render_height = (uint32_t)((float)output_height / FsrModeScalingFactors[FSR_MODE_PERFORMANCE]);
		break;
	}
	case FSR_MODE_ULTRA_PERFORMANCE:
	{
		render_width = (uint32_t)((float)output_width / FsrModeScalingFactors[FSR_MODE_ULTRA_PERFORMANCE]);
		render_height = (uint32_t)((float)output_height / FsrModeScalingFactors[FSR_MODE_ULTRA_PERFORMANCE]);
		break;
	}
	}

	render_width = std::max(render_width, 1u);
	render_height = std::max(render_height, 1u);
}

RT_Vec2 FSR2::GetMipBiasForFSRMode(uint32_t output_width, uint32_t output_height, uint32_t render_width, uint32_t render_height)
{
	return RT_Vec2Make(std::log2f((float)render_width / (float)output_width) - 1.0f, std::log2f((float)render_height / (float)output_height) - 1.0f);
}

void FSR2::OnWindowResize(uint32_t width, uint32_t height)
{
	width = std::max(width, 2u);
	height = std::max(height, 2u);

	FFX_CALL(ffxFsr2ContextDestroy(&data.context));

	data.context_desc.displaySize = { width, height };
	data.context_desc.maxRenderSize = { width, height };
	FFX_CALL(ffxFsr2ContextCreate(&data.context, &data.context_desc));
}
