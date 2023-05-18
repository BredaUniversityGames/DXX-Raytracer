#pragma once
#include "WinIncludes.h"

namespace RT
{

	class CommandList
	{
	public:
		CommandList(D3D12_COMMAND_LIST_TYPE type);
		~CommandList();

		void Close();
		void Reset();

		ID3D12GraphicsCommandList4* GetD3D12CommandList() const { return m_d3d12_graphics_command_list2; }

		ID3D12GraphicsCommandList4* operator -> () { return m_d3d12_graphics_command_list2; }
		operator ID3D12GraphicsCommandList4* () { return m_d3d12_graphics_command_list2; }

		uint64_t GetFenceValue() const { return m_fence_value; }

	private:
		ID3D12GraphicsCommandList4* m_d3d12_graphics_command_list2;
		ID3D12CommandAllocator* m_d3d12_command_allocator;
		D3D12_COMMAND_LIST_TYPE m_d3d12_command_list_type;

		friend class CommandQueue;

		uint64_t m_fence_value = UINT64_MAX;
		CommandList* next = nullptr;

	};

}
