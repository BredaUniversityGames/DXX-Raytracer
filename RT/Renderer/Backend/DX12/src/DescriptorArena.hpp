#pragma once

#include <d3d12.h>
#include "Core/Common.h"

namespace RT
{

	struct DescriptorAllocation
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpu;
		D3D12_GPU_DESCRIPTOR_HANDLE gpu;
		UINT descriptor_count;
		UINT increment_size;
		UINT heap_offset;

		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptor(UINT offset) const
		{
			RT_ASSERT(offset < descriptor_count);
			return { cpu.ptr + offset * increment_size };
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptor(UINT offset) const
		{
			RT_ASSERT(offset < descriptor_count);
			return { gpu.ptr + offset * increment_size };
		}

		bool IsNull() const
		{
			return cpu.ptr == 0;
		}
	};

	class DescriptorArena
	{
	public:
		void Init(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT capacity, D3D12_DESCRIPTOR_HEAP_FLAGS flags);
		void Release();

		DescriptorAllocation Allocate(UINT count);

		// Reset arena back to start
		void Reset();

		UINT GetDescriptorsRemaining() const { return m_capacity - m_at; }
		ID3D12DescriptorHeap *GetHeap() const { return m_heap; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUBase() const { return m_cpu_base; }
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUBase() const { return m_gpu_base; }

	private:
		ID3D12DescriptorHeap *m_heap;
		D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_base;
		D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_base;
		UINT m_at;
		UINT m_capacity;
		UINT m_increment;
	};

	class DescriptorArenaFreelist
	{
	public:
		void Init(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT capacity, D3D12_DESCRIPTOR_HEAP_FLAGS flags);
		void Release();

		DescriptorAllocation Allocate(UINT count);
		void Free(DescriptorAllocation allocation);
		// Reset arena back to start
		void Reset();

		//UINT GetDescriptorsRemaining() const { return m_capacity - m_at; }
		ID3D12DescriptorHeap* GetHeap() const { return m_heap; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUBase() const { return m_cpu_base; }
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUBase() const { return m_gpu_base; }

	private:
		ID3D12DescriptorHeap* m_heap;
		D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_base;
		D3D12_GPU_DESCRIPTOR_HANDLE m_gpu_base;
		
		struct DescriptorBlock
		{
			UINT descriptor_offset;
			UINT descriptor_count;
			union
			{
				DescriptorBlock* next;
				UINT next_index;
			};

		};

		DescriptorBlock* m_blocks;
		UINT m_next_free;
		DescriptorBlock* m_current_block;
		UINT m_capacity;
		UINT m_increment;
	};
}