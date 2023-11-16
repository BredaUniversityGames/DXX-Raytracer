#pragma once
#include "WinIncludes.h"
#include "ResourceTracker.hpp"

namespace RT
{

	// -------------------------------------------------------------------------
	// Resources

	void CopyResource(ID3D12GraphicsCommandList* command_list, ID3D12Resource* dst, ID3D12Resource* src);

	// -------------------------------------------------------------------------
	// Buffers

	ID3D12Resource* CreateBuffer(RT_RESOURCE_TRACKER_DEBUG_PARAMS const wchar_t* name, size_t size, D3D12_HEAP_TYPE heap_type, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initial_state);
	ID3D12Resource* CreateReadOnlyBuffer(RT_RESOURCE_TRACKER_DEBUG_PARAMS const wchar_t* name, size_t size);
	ID3D12Resource* CreateReadWriteBuffer(RT_RESOURCE_TRACKER_DEBUG_PARAMS const wchar_t* name, size_t size);
	ID3D12Resource* CreateUploadBuffer(RT_RESOURCE_TRACKER_DEBUG_PARAMS const wchar_t* name, size_t size);
	ID3D12Resource* CreateReadbackBuffer(RT_RESOURCE_TRACKER_DEBUG_PARAMS const wchar_t* name, size_t size);
	ID3D12Resource* CreateAccelerationStructureBuffer(RT_RESOURCE_TRACKER_DEBUG_PARAMS const wchar_t* name, size_t size);

#define RT_CreateBuffer(...) CreateBuffer(RT_RESOURCE_TRACKER_DEBUG_ARGS __VA_ARGS__)
#define RT_CreateReadOnlyBuffer(...) CreateReadOnlyBuffer(RT_RESOURCE_TRACKER_DEBUG_ARGS __VA_ARGS__)
#define RT_CreateReadWriteBuffer(...) CreateReadWriteBuffer(RT_RESOURCE_TRACKER_DEBUG_ARGS __VA_ARGS__)
#define RT_CreateUploadBuffer(...) CreateUploadBuffer(RT_RESOURCE_TRACKER_DEBUG_ARGS __VA_ARGS__)
#define RT_CreateReadbackBuffer(...) CreateReadbackBuffer(RT_RESOURCE_TRACKER_DEBUG_ARGS __VA_ARGS__)
#define RT_CreateAccelerationStructureBuffer(...) CreateAccelerationStructureBuffer(RT_RESOURCE_TRACKER_DEBUG_ARGS __VA_ARGS__)

	void CopyBuffer(ID3D12GraphicsCommandList* command_list, ID3D12Resource* dst, ID3D12Resource* src);
	void CopyBufferRegion(ID3D12GraphicsCommandList* command_list, ID3D12Resource* dst, size_t dst_offset, ID3D12Resource* src, size_t src_offset, size_t num_bytes);

	void CreateBufferSRV(ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE descriptor, size_t first_element, uint32_t num_elements, uint32_t byte_stride);
	void CreateBufferUAV(ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE descriptor, size_t first_element, uint32_t num_elements, uint32_t byte_stride);
	void CreateAccelerationStructureSRV(ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);

	// -------------------------------------------------------------------------
	// Textures

	ID3D12Resource* CreateTexture(RT_RESOURCE_TRACKER_DEBUG_PARAMS const wchar_t* name, const D3D12_RESOURCE_DESC* resource_desc, D3D12_CLEAR_VALUE* clear_value = nullptr);
	ID3D12Resource* CreateTexture(RT_RESOURCE_TRACKER_DEBUG_PARAMS const wchar_t* name, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags, size_t width, uint32_t height,
		D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, uint16_t mips = 1, D3D12_CLEAR_VALUE* clear_value = nullptr);

#define RT_CreateTexture(...) CreateTexture(RT_RESOURCE_TRACKER_DEBUG_ARGS __VA_ARGS__)

	void CopyTexture(ID3D12GraphicsCommandList* command_list, ID3D12Resource* dst, ID3D12Resource* src);
	void CopyTextureRegion(ID3D12GraphicsCommandList* command_list, ID3D12Resource* dst, uint32_t dst_x, uint32_t dst_y, uint32_t dst_z,
		const D3D12_TEXTURE_COPY_LOCATION* src_loc, const D3D12_BOX* src_box);
	void UploadTextureData(ID3D12Resource* dst, size_t row_pitch, size_t row_count, const void* data_ptr);
	void UploadTextureDataDDS(ID3D12Resource* dst, size_t width, size_t height, size_t bpp, size_t mipCount, const void* data_ptr);

	void CreateTextureSRV(ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE descriptor, DXGI_FORMAT format, uint32_t mips = UINT32_MAX);
	void CreateTextureUAV(ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE descriptor, DXGI_FORMAT format);
	void CreateTextureRTV(ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE descriptor, DXGI_FORMAT format);
	void CreateTextureDSV(ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE descriptor, DXGI_FORMAT format);

}
