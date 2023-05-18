#pragma once

#include <d3d12.h>
#include "Core/Common.h"

namespace RT
{

	namespace GPUProfiler
	{

		enum GPUTimer
		{
			GPUTimer_FrameTime,
			GPUTimer_BuildTLAS,
			GPUTimer_PrimaryRay,
			GPUTimer_DirectLighting,
			GPUTimer_IndirectLighting,
			GPUTimer_DenoiseResample,
			GPUTimer_DenoiseSVGF,
			GPUTimer_Composite,
			GPUTimer_TAA,
			GPUTimer_Bloom,
			GPUTimer_PostProcess,
			GPUTimer_NumTimers
		};

		struct GPUTimestamp
		{
			uint32_t StartIndex;
			uint64_t BeginTimestamp;
			uint64_t EndTimestamp;
		};

		void Init();
		void Exit();

		void BeginTimestampQuery(ID3D12GraphicsCommandList4* command_list, GPUTimer timer);
		void EndTimestampQuery(ID3D12GraphicsCommandList4* command_list, GPUTimer timer);
		void ResolveTimestampQueries(ID3D12GraphicsCommandList4* command_list);
		void ReadbackTimestampsFromBuffer();

		void RenderImGuiMenus();

	};

}