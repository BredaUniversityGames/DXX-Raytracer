#pragma once
#include "WinIncludes.h"

namespace RT
{

	class CommandList;

	class CommandQueue
	{
	public:
		CommandQueue(D3D12_COMMAND_LIST_TYPE type);
		~CommandQueue();

		CommandList& GetCommandList();
		uint64_t ExecuteCommandList(CommandList& entry);

		uint64_t Signal();
		bool IsFenceComplete() const;
		void WaitForFenceValue(uint64_t fence_value);

		void Flush();

		ID3D12CommandQueue* GetD3D12CommandQueue() const { return m_d3d12_command_queue; }

	private:
		ID3D12CommandQueue* m_d3d12_command_queue;
		D3D12_COMMAND_LIST_TYPE m_d3d12_command_list_type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		ComPtr<ID3D12Fence> m_d3d12_fence;
		uint64_t m_fence_value = 0;

		CommandList* m_first_free_command_list = nullptr;
		CommandList* m_first_in_flight_command_list = nullptr;

	};

}
