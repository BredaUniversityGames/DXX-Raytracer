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