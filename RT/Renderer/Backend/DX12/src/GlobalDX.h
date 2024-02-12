#pragma once

#include "WinIncludes.h"

#include "Core/Common.h"
#include "Core/Arena.h"
#include "Core/MemoryScope.hpp"
#include "Core/SlotMap.hpp"
#include "Core/Config.h"
#include "Core/String.h"

#include "Renderer.h"
#include "ResourceTracker.hpp"
#include "MeshTracker.hpp"
#include "DescriptorArena.hpp"
#include "RingBuffer.h"

//------------------------------------------------------------------------	
// Shared includes with HLSL

#include "shared_common.hlsl.h"
#include "shared_rendertargets.hlsl.h"

#if RT_DISPATCH_RAYS
#include "ShaderTable.h"
#endif

// Uncomment this to wait after each frame, effectively disabling triple buffering
// #define RT_FORCE_HEAVY_SYNCHRONIZATION

//------------------------------------------------------------------------	

namespace RT
{

	extern TweakVars tweak_vars;

	class CommandQueue;

	constexpr bool     GPU_VALIDATION_ENABLED   = false;
	constexpr uint32_t BACK_BUFFER_COUNT		= 3;
	constexpr uint32_t MAX_INSTANCES			= 1000;
	constexpr uint32_t MAX_BOTTOM_LEVELS		= 1000;
	constexpr uint32_t HALTON_SAMPLE_COUNT      = 128;
	constexpr uint32_t MAX_RASTER_TRIANGLES		= 10000;
	constexpr uint32_t MAX_RASTER_LINES			= 5000;
	constexpr uint32_t MAX_DEBUG_LINES_WORLD	= 5000;
	constexpr uint32_t CBV_SRV_UAV_HEAP_SIZE	= 65536;

#if RT_DISPATCH_RAYS
	struct RaytracingShader
	{
		const wchar_t *source_file;
		const wchar_t *name;
		const wchar_t *export_name;

		uint64_t timestamp;
		bool     dirty; // was reloaded this frame

		IDxcBlob *blob;
	};

	struct HitGroup
	{
		RaytracingShader *closest;
		RaytracingShader *any;
		const wchar_t *export_name;
	};
	
	constexpr size_t MAX_SHADERS_PER_RAYTRACING_PIPELINE   = 8;
	constexpr size_t MAX_HITGROUPS_PER_RAYTRACING_PIPELINE = 2;

	struct RaytracingPipelineParams
	{
		UINT shader_count;
		RaytracingShader *shaders[MAX_SHADERS_PER_RAYTRACING_PIPELINE];

		UINT hitgroup_count;
		HitGroup hitgroups[MAX_SHADERS_PER_RAYTRACING_PIPELINE];

		UINT max_payload_size;
		UINT max_attribute_size;
		UINT max_recursion_depth;
	};

	struct RaytracingPipeline
	{
		ID3D12StateObject *pso;
		ID3D12StateObjectProperties *pso_properties;
	};

	struct PrimaryRayPayload
	{
		uint32_t instance_idx;
		uint32_t primitive_idx;
		RT_Vec2 barycentrics;
		float hit_distance;
	};

	struct OcclusionRayPayload
	{
		uint32_t visible;
	};

	struct ShaderRecord
	{
		char identifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
	};

	constexpr wchar_t* primary_raygen_export_name = L"PrimaryRayGen";
	constexpr wchar_t* primary_closesthit_export_name = L"PrimaryClosestHit";
	constexpr wchar_t* primary_anyhit_export_name = L"PrimaryAnyHit";
	constexpr wchar_t* primary_miss_export_name = L"PrimaryMiss";
	constexpr wchar_t* primary_hitgroup_export_name = L"PrimaryHitGroup";

	constexpr wchar_t* direct_lighting_raygen_export_name = L"DirectLightingRaygen";
	constexpr wchar_t* indirect_lighting_raygen_export_name = L"IndirectLightingRaygen";
	constexpr wchar_t* occlusion_anyhit_export_name = L"OcclusionAnyhit";
	constexpr wchar_t* occlusion_miss_export_name = L"OcclusionMiss";
	constexpr wchar_t* occlusion_hitgroup_export_name = L"OcclusionHitGroup";
#endif

	struct ComputeShader
	{
		uint64_t timestamp;
		ID3D12PipelineState *pso;
	};

	struct BufferAllocation
	{
		ID3D12Resource *buffer;

		void* cpu;
		D3D12_GPU_VIRTUAL_ADDRESS gpu;

		size_t size;
		size_t offset;

		template <typename T>
		T *As()
		{
			return (T *)cpu;
		}
	};

	struct MeshResource
	{
		ID3D12Resource* blas;
		ID3D12Resource *triangle_buffer;
		DescriptorAllocation triangle_buffer_descriptor;
	};

	struct BLASResource
	{
		size_t num_meshes;
		MeshResource* meshes;
	};

	struct TextureResource
	{
		RT_ResourceHandle handle;

		ID3D12Resource* texture;
		DescriptorAllocation descriptors;
		DescriptorAllocation rtv_descriptor;
	};

	struct FrameData
	{
		uint64_t fence_value;

		ID3D12Resource *upload_buffer;

		RT_Arena upload_buffer_arena;
		RT_ArenaMarker upload_buffer_arena_reset;

		BufferAllocation scene_cb;
		BufferAllocation tweak_vars;
		BufferAllocation lights;
		BufferAllocation instance_descs;
		BufferAllocation instance_data;
		BufferAllocation material_edges;
		BufferAllocation material_indices;

		ID3D12Resource *pixel_debug_readback;

		UINT64 tlas_size, tlas_scratch_size; 
		ID3D12Resource *top_level_as;
		ID3D12Resource *top_level_as_scratch;

		DescriptorAllocation descriptors;
		DescriptorAllocation non_shader_descriptors;
	};

	// TODO(daniel): Move these into the g_d3d struct, probably, but be wary of constructors preventing the default initialization
	// of all the trivial types in D3DState.
	extern RT::SlotMap<MeshResource> g_mesh_slotmap;
	extern RT::SlotMap<TextureResource> g_texture_slotmap;

	enum RenderTarget
	{
		#define RT_RENDER_TARGETS_DECLARE_ENUM(name, reg, scale_x, scale_y, type, format) \
			RenderTarget_##name = reg,

		RT_RENDER_TARGETS(RT_RENDER_TARGETS_DECLARE_ENUM)

		RenderTarget_COUNT,
	};

	enum RaytraceRootParameters
	{
		RaytraceRootParameters_MainDescriptorTable,
		RaytraceRootParameters_DenoiseIteration,
		RaytraceRootParameters_BindlessSRVTable,
		RaytraceRootParameters_BindlessTriangleBufferTable,
		RaytraceRootParameters_COUNT,
	};

	enum D3D12GlobalDescriptors
	{
		// ------------------------------------------------------------------
		// Manually declared descriptors

		D3D12GlobalDescriptors_UAV_PixelDebugBuffer,

		D3D12GlobalDescriptors_SRV_AccelerationStructure,
		D3D12GlobalDescriptors_SRV_LightBuffer,
		D3D12GlobalDescriptors_SRV_Materials,
		D3D12GlobalDescriptors_SRV_InstanceDataBuffer,
		D3D12GlobalDescriptors_SRV_MaterialEdges,
		D3D12GlobalDescriptors_SRV_MaterialIndices,
		D3D12GlobalDescriptors_SRV_BlueNoiseFirst,
		D3D12GlobalDescriptors_SRV_BlueNoiseLast = D3D12GlobalDescriptors_SRV_BlueNoiseFirst + BLUE_NOISE_TEX_COUNT - 1,
		D3D12GlobalDescriptors_SRV_ImGui,

		D3D12GlobalDescriptors_CBV_GlobalConstantBuffer,
		D3D12GlobalDescriptors_CBV_TweakVars,

		// ------------------------------------------------------------------
		// Render targets

#define RT_RENDER_TARGETS_DECLARE_UAVS(name, reg, scale_x, scale_y, type, format) \
	D3D12GlobalDescriptors_UAV_##name,

		RT_RENDER_TARGETS(RT_RENDER_TARGETS_DECLARE_UAVS)

#define RT_RENDER_TARGETS_DECLARE_SRVS(name, reg, scale_x, scale_y, type, format) \
	D3D12GlobalDescriptors_SRV_##name,

		RT_RENDER_TARGETS(RT_RENDER_TARGETS_DECLARE_SRVS)

		// ------------------------------------------------------------------

		D3D12GlobalDescriptors_COUNT, // Always keep this at the end so it is correct

		// ------------------------------------------------------------------

		D3D12GlobalDescriptors_UAV_START = D3D12GlobalDescriptors_UAV_PixelDebugBuffer,
		D3D12GlobalDescriptors_SRV_START = D3D12GlobalDescriptors_SRV_AccelerationStructure,
		D3D12GlobalDescriptors_CBV_START = D3D12GlobalDescriptors_CBV_GlobalConstantBuffer,
		D3D12GlobalDescriptors_UAV_RT_START = D3D12GlobalDescriptors_UAV_color,
		D3D12GlobalDescriptors_SRV_RT_START = D3D12GlobalDescriptors_SRV_color,
	};

	struct D3D12State
	{
		HWND hWnd;

		bool queued_screenshot;
		char queued_screenshot_name[1024];

		RT_Vec2 halton_samples[HALTON_SAMPLE_COUNT];
		float viewport_offset_y;

		RT_RendererIO io;
		uint64_t tweakvars_config_last_modified_time;

		D3D12ResourceTracker resource_tracker;
		MeshTracker mesh_tracker;

		TextureResource  *white_texture;
		RT_ResourceHandle white_texture_handle;

		TextureResource  *black_texture;
		RT_ResourceHandle black_texture_handle;

		TextureResource *default_normal_texture;
		RT_ResourceHandle pink_checkerboard_texture;

		RT_ResourceHandle billboard_quad;
		RT_ResourceHandle cube;

		RT_Arena *arena;

		ID3D12Device5* device;

		IDXGIAdapter4* dxgi_adapter4;

		DescriptorArena rtv;
		DescriptorArena dsv;
		DescriptorArenaFreelist cbv_srv_uav;
		DescriptorArena cbv_srv_uav_staging;

		IDXGISwapChain4* dxgi_swapchain4;
		ID3D12Resource* back_buffers[BACK_BUFFER_COUNT];
		uint32_t current_back_buffer_index;
		uint32_t prev_back_buffer_index;

		ID3D12Resource *material_edges;
		ID3D12Resource *material_indices;

		uint32_t material_count;
		ID3D12Resource *material_buffer;
		Material *material_buffer_cpu;

		ID3D12QueryHeap* query_heap;
		ID3D12Resource* query_readback_buffer;

		CommandQueue* command_queue_direct;

		uint32_t render_width;
		uint32_t render_height;

		uint32_t render_width_override;
		uint32_t render_height_override;
       
		bool vsync = true;
		bool tearing_supported;

		uint64_t frame_index;
		uint64_t accum_frame_index;
		FrameData frame_data[BACK_BUFFER_COUNT];

		IDxcCompiler* dxc_compiler;
		IDxcUtils* dxc_utils;
		IDxcIncludeHandler* dxc_include_handler;

		ID3D12RootSignature* global_root_sig;

		RT_Config global_shader_defines;

#if RT_DISPATCH_RAYS
		ID3D12Resource* raygen_shader_table;
		ID3D12Resource* miss_shader_table;
		ID3D12Resource* hitgroups_shader_table;

		union
		{
			struct  
			{
				RaytracingShader primary_raygen;
				RaytracingShader primary_closest;
				RaytracingShader primary_any;
				RaytracingShader primary_miss;

				RaytracingShader direct_raygen;
				RaytracingShader indirect_raygen;

				RaytracingShader occlusion_any;
				RaytracingShader occlusion_miss;
			} rt_shaders;

			RaytracingShader rt_shaders_all[sizeof(rt_shaders) / sizeof(RaytracingShader)];
		};

		union
		{
			struct  
			{
				RaytracingPipeline primary;
				RaytracingPipeline direct;
				RaytracingPipeline indirect;
			} rt_pipelines;

			RaytracingPipeline rt_pipelines_all[sizeof(rt_pipelines) / sizeof(RaytracingPipeline)];
		};
#elif RT_INLINE_RAYTRACING
		union
		{
			struct
			{
				ComputeShader primary_inline;
				ComputeShader direct_lighting_inline;
				ComputeShader indirect_lighting_inline;
			} rt_shaders;

			ComputeShader rt_shaders_all[sizeof(rt_shaders) / sizeof(ComputeShader)];
		};
#endif

		ID3D12RootSignature* gen_mipmap_root_sig;

		struct 
		{
			ComputeShader restir_gen_candidates;

			ComputeShader svgf_prepass;
			ComputeShader svgf_history_fix;
			ComputeShader svgf_resample;
			ComputeShader svgf_post_resample;
			ComputeShader svgf_blur;

			ComputeShader taa;

			ComputeShader bloom_prepass;
			ComputeShader bloom_blur_horz;
			ComputeShader bloom_blur_vert;

			ComputeShader composite;
			ComputeShader post_process;
			ComputeShader resolve_final_color;

			ComputeShader gen_mipmaps;
		} cs;

		union
		{
			struct
			{
				#define RT_RENDER_TARGETS_DECLARE_RESOURCES(name, reg, scale_x, scale_y, type, format) \
					ID3D12Resource *name;

				RT_RENDER_TARGETS(RT_RENDER_TARGETS_DECLARE_RESOURCES)
			} rt;

			ID3D12Resource *render_targets[RenderTarget_COUNT];
		};
		DXGI_FORMAT render_target_formats[RenderTarget_COUNT];

		DescriptorAllocation color_final_rtv;
		ID3D12Resource *blue_noise_textures[BLUE_NOISE_TEX_COUNT];

		struct  
		{
			ID3D12Resource *uav_resource;
		} pixel_debug;

		uint32_t tlas_instance_count;
		ID3D12Resource *tlas_instance_buffer;
		ID3D12Resource *instance_data_buffer;

		uint32_t prev_lights_count;
		uint32_t lights_count;
		ID3D12Resource *lights_buffer;

		struct SceneData
		{
			RT_Camera camera;
			RT_Camera prev_camera;

			uint64_t last_camera_update_frame;
			int freezeframe;
			
			bool render_blit;
		} scene;

		RingBuffer resource_upload_ring_buffer;
	};

	extern D3D12State g_d3d;

	struct D3D12RasterState
	{
		ID3D12Resource* depth_target;
		D3D12_CPU_DESCRIPTOR_HANDLE depth_target_dsv;

		D3D12_VIEWPORT viewport;
		ID3D12Resource* render_target;
		D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle;

		// UI quad rendering
		ID3D12PipelineState* tri_state;
		ID3D12PipelineState* tri_state_srgb;
		ID3D12RootSignature* tri_root_sig;

		ID3D12Resource* tri_vertex_buffer;
		size_t tri_at;
		size_t tri_count;
		RT_RasterTriVertex* tri_vertex_buf_ptr;

		// Line rendering
		ID3D12PipelineState* line_state;
		ID3D12RootSignature* line_root_sig;

		ID3D12Resource* line_vertex_buffer;
		size_t line_at;
		size_t line_count;
		RT_RasterLineVertex* line_vertex_buf_ptr;

		// Debug line rendering (world space)
		ID3D12PipelineState* debug_line_state;
		ID3D12PipelineState* debug_line_state_depth;
		ID3D12RootSignature* debug_line_root_sig;

		ID3D12Resource* debug_line_vertex_buffer;
		size_t debug_line_at;
		size_t debug_line_count;
		RT_RasterLineVertex* debug_line_vertex_buf_ptr;

		ID3D12PipelineState* blit_state;
		ID3D12RootSignature* blit_root_sig;
	};

	extern D3D12RasterState g_d3d_raster;

	// ------------------------------------------------------------------
	// Helper functions (TODO: refactor and such)

	inline FrameData *CurrentFrameData() 
	{
		return &g_d3d.frame_data[g_d3d.current_back_buffer_index];
	}

	inline FrameData *PrevFrameData() 
	{
		return &g_d3d.frame_data[g_d3d.prev_back_buffer_index];
	}

	BufferAllocation AllocateFromUploadBuffer(FrameData *frame, size_t size, size_t align = 256);

	template <typename T>
	inline BufferAllocation CopyIntoUploadBuffer(FrameData *frame, const T *data, size_t count, size_t align = alignof(T))
	{
		BufferAllocation result = AllocateFromUploadBuffer(frame, sizeof(T)*count, align);
		memcpy(result.cpu, data, sizeof(T)*count);
		return result;
	}

	D3D12_RESOURCE_BARRIER GetUAVBarrier(ID3D12Resource* resource);
	void UAVBarrier(ID3D12GraphicsCommandList *command_list, ID3D12Resource *resource);
	void UAVBarriers(ID3D12GraphicsCommandList *command_list, size_t count, ID3D12Resource **resources);

	D3D12_RESOURCE_BARRIER GetTransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before_state, D3D12_RESOURCE_STATES after_state);
	// NOTE(daniel): For tracked resources only!
	D3D12_RESOURCE_STATES ResourceTransition(ID3D12GraphicsCommandList *list, ID3D12Resource *resource, D3D12_RESOURCE_STATES dst_state);
	void ResourceTransitions(ID3D12GraphicsCommandList* list, size_t num_resources, ID3D12Resource** resources, D3D12_RESOURCE_STATES dst_state);
	void ResourceTransitions(ID3D12GraphicsCommandList* list, size_t num_resources, ID3D12Resource** resources, D3D12_RESOURCE_STATES* dst_states);

	inline HRESULT GetName(ID3D12Object *object, UINT buffer_size, wchar_t *buffer)
	{
		UINT size = buffer_size;
		return object->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, buffer);
	}
}
