#pragma once
#include "WinIncludes.h"
#include "Resource.h"

namespace RT
{

	struct ShaderRecord;

	struct ShaderTable
	{
		ID3D12Resource* resource;

		uint8_t* base_ptr;
		uint8_t* current_ptr;

		size_t num_records;
		size_t record_stride;
		size_t byte_size;
	};

	ShaderTable CreateShaderTable(const wchar_t* name, size_t num_records, size_t stride = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	void ResetShaderTable(ShaderTable* table);
	void AddEntryToShaderTable(ShaderTable* table, size_t num_records, const ShaderRecord* records);
	D3D12_GPU_VIRTUAL_ADDRESS GetShaderTableGPUPtr(ShaderTable* table, size_t offset);
	D3D12_GPU_VIRTUAL_ADDRESS GetShaderTableGPUPtr(ID3D12Resource* resource, size_t offset);

}
