#include "DescriptorArena.hpp"
#include "GlobalDX.h"

using namespace RT;

void DescriptorArena::Init(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT capacity, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type           = type;
	desc.NumDescriptors = capacity;
	desc.Flags          = flags;

	DX_CALL(g_d3d.device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap)));

	m_cpu_base  = m_heap->GetCPUDescriptorHandleForHeapStart();
	if (desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
		m_gpu_base  = m_heap->GetGPUDescriptorHandleForHeapStart();
	m_at        = 0;
	m_capacity  = capacity;
	m_increment = g_d3d.device->GetDescriptorHandleIncrementSize(type);
}

void DescriptorArena::Release()
{
	if (m_heap)
	{
		m_heap->Release();
		m_heap = nullptr;
	}
}

DescriptorAllocation DescriptorArena::Allocate(UINT count)
{
	DescriptorAllocation result = {};

	if (ALWAYS(m_at + count <= m_capacity))
	{
		result.cpu.ptr = m_cpu_base.ptr + m_at*m_increment;
		result.gpu.ptr = m_gpu_base.ptr + m_at*m_increment;
		result.descriptor_count = count;
		result.increment_size = m_increment;
		result.heap_offset = m_at;

		m_at += count;
	}

	return result;
}

void DescriptorArena::Reset()
{
	m_at = 0;
}

void DescriptorArenaFreelist::Init(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT capacity, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = type;
	desc.NumDescriptors = capacity;
	desc.Flags = flags;

	DX_CALL(g_d3d.device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap)));

	m_cpu_base = m_heap->GetCPUDescriptorHandleForHeapStart();
	if (desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
		m_gpu_base = m_heap->GetGPUDescriptorHandleForHeapStart();

	m_capacity = capacity;
	m_increment = g_d3d.device->GetDescriptorHandleIncrementSize(type);
	
	//NOTE (SAM)
	//Just allocate as many freeblocks as we have descriptors.
	//Not the best way, but solid for now.
	m_blocks = RT_ArenaAllocArray(g_d3d.arena, capacity, DescriptorArenaFreelist::DescriptorBlock);
	m_current_block = &m_blocks[0];
	m_current_block->descriptor_count = m_capacity;
	m_current_block->descriptor_offset = 0;
	m_current_block->next = nullptr;
	m_next_free = 1;

	for (UINT i = m_next_free; i < m_capacity; i++)
	{
		m_blocks[i].next_index = i + 1;
	}
}

void DescriptorArenaFreelist::Release()
{
	if (m_heap)
	{
		m_heap->Release();
		m_heap = nullptr;
	}
}

DescriptorAllocation DescriptorArenaFreelist::Allocate(UINT count)
{
	DescriptorAllocation result = {};
	DescriptorBlock* previous_freeblock = nullptr;
	DescriptorBlock* freeblock = m_current_block;

	while (freeblock != nullptr)
	{
		RT_ASSERT(freeblock != freeblock->next);
		if (count > freeblock->descriptor_count)
		{
			previous_freeblock = freeblock;
			freeblock = freeblock->next;
			continue;
		}

		result.cpu.ptr = m_cpu_base.ptr + freeblock->descriptor_offset * m_increment;
		result.gpu.ptr = m_gpu_base.ptr + freeblock->descriptor_offset * m_increment;
		result.descriptor_count = count;
		result.increment_size = m_increment;
		result.heap_offset = freeblock->descriptor_offset;

		freeblock->descriptor_count -= count;
		freeblock->descriptor_offset += count;

		if (freeblock->descriptor_count == 0)
		{
			if (previous_freeblock != nullptr)
				previous_freeblock->next = freeblock->next;
			else
				m_current_block = freeblock->next;
		}
		return result;
	}

	//No free blocks available.
	RT_ASSERT(false);
	return result;
}

void DescriptorArenaFreelist::Free(DescriptorAllocation allocation)
{
	DescriptorBlock* freeblock = m_current_block;
	const UINT block_end = allocation.heap_offset + allocation.descriptor_count;

	while (freeblock != nullptr)
	{
		//assert to check if we point to ourselves.
		RT_ASSERT(freeblock != freeblock->next);

		//check back of the freeblock
		if (block_end == freeblock->descriptor_offset)
		{
			freeblock->descriptor_offset -= allocation.descriptor_count;
			freeblock->descriptor_count += allocation.descriptor_count;
			return;
		}
		//check front of the freeblock
		const UINT free_block_end = freeblock->descriptor_offset + freeblock->descriptor_count;
		if (free_block_end == allocation.heap_offset)
		{
			freeblock->descriptor_count += allocation.descriptor_count;
			return;
		}

		freeblock = freeblock->next;
	} 
	//If we cannot combine we make a new block.
	DescriptorBlock& new_block = m_blocks[m_next_free];
	const UINT next_free_index = new_block.next_index;
	new_block.descriptor_offset = allocation.heap_offset;
	new_block.descriptor_count = allocation.descriptor_count;
	new_block.next = m_current_block;
	m_current_block = &new_block;
	m_next_free = next_free_index;
}

// Reset arena back to start
void DescriptorArenaFreelist::Reset()
{
	DescriptorBlock first_block = {0};
	first_block.descriptor_count = m_capacity;
	first_block.descriptor_offset = 0;
	first_block.next = nullptr;
	m_next_free = 1;

	//note SAM
	//memset the rest back to 0 to be nice.
	memset(m_blocks, 0, m_capacity);

	m_blocks[0] = first_block;
	for (UINT i = m_next_free; i < m_capacity; i++)
	{
		m_blocks[i].next_index = i + 1;
	}
}