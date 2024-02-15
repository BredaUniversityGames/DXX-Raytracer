#include "ShaderTable.h"
#include "Resource.h"
#include "GlobalDX.h"

#if RT_DISPATCH_RAYS
namespace RT
{
	ShaderTable CreateShaderTable(const wchar_t* name, size_t num_records, size_t stride)
	{
		D3D12_RANGE read_range = { 0, 0 };

		ShaderTable table = {};
		table.num_records = num_records;
		table.record_stride = stride;
		table.byte_size = table.num_records * table.record_stride;
		table.resource = RT_CreateUploadBuffer(name, table.byte_size);
		table.resource->Map(0, &read_range, (void**)&table.base_ptr);
		table.current_ptr = table.base_ptr;

		return table;
	}

	void ResetShaderTable(ShaderTable* table)
	{
		table->current_ptr = table->base_ptr;
	}

	void AddEntryToShaderTable(ShaderTable* table, size_t num_records, const ShaderRecord* records)
	{
		// Get the total byte size left in the shader table and check if the records will fit
		size_t byte_size_left = (table->byte_size - (table->current_ptr - table->base_ptr));
		ALWAYS(byte_size_left >= num_records * table->record_stride);

		for (size_t i = 0; i < num_records; ++i)
		{
			memcpy(table->current_ptr, &records[i], table->record_stride);
			table->current_ptr += table->record_stride;
		}
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetShaderTableGPUPtr(ShaderTable* table, size_t offset)
	{
		return table->resource->GetGPUVirtualAddress() + offset * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetShaderTableGPUPtr(ID3D12Resource* resource, size_t offset)
	{
		return resource->GetGPUVirtualAddress() + offset * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	}

}
#endif
