#include "CommandQueue.h"
#include "GlobalDX.h"
#include "Core/Common.h"
#include "CommandList.h"

using namespace RT;

CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE type)
	: m_d3d12_command_list_type(type)
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = m_d3d12_command_list_type;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.NodeMask = 0;

	DX_CALL(g_d3d.device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_d3d12_command_queue)));
	DX_CALL(g_d3d.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_d3d12_fence)));
}

CommandQueue::~CommandQueue()
{
	Flush();
}

CommandList& CommandQueue::GetCommandList()
{
	if (!m_first_free_command_list)
	{
		m_first_free_command_list = RT_ArenaAllocStruct(g_d3d.arena, CommandList);
		new (m_first_free_command_list) CommandList{ m_d3d12_command_list_type };
	}

	CommandList *list = RT_SLL_POP(m_first_free_command_list);
	return *list;
}

uint64_t CommandQueue::ExecuteCommandList(CommandList& command_list)
{
	command_list->Close();

	ID3D12CommandList* const ppCommandLists[] = { command_list };
	m_d3d12_command_queue->ExecuteCommandLists(1, ppCommandLists);

	uint64_t fence_value = Signal();
	command_list.m_fence_value = fence_value;

	RT_SLL_PUSH(m_first_in_flight_command_list, &command_list);

	return fence_value;
}

uint64_t CommandQueue::Signal()
{
	uint64_t fenceValue = ++m_fence_value;
	m_d3d12_command_queue->Signal(m_d3d12_fence.Get(), fenceValue);

	return fenceValue;
}

bool CommandQueue::IsFenceComplete() const
{
	return m_d3d12_fence->GetCompletedValue() >= m_fence_value;
}

void CommandQueue::WaitForFenceValue(uint64_t fence_value)
{
	if (!IsFenceComplete())
	{
		HANDLE fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		RT_ASSERT(fenceEvent && "Failed to create fence event handle");

		DX_CALL(m_d3d12_fence->SetEventOnCompletion(fence_value, fenceEvent));
		::WaitForSingleObject(fenceEvent, static_cast<DWORD>(DWORD_MAX));

		::CloseHandle(fenceEvent);
	}

	// Reset in-flight command lists that have their fence value <= to the value that has been waited on
	// After writing this we all laid down our keyboards and moved on to become monks in the himalayas
	for (CommandList **command_list_at = &m_first_in_flight_command_list; *command_list_at;)
	{
		CommandList *command_list = *command_list_at;

		if (command_list->m_fence_value <= fence_value)
		{
			command_list->Reset();

			*command_list_at = command_list->next;
			RT_SLL_PUSH(m_first_free_command_list, command_list);
		}
		else
		{
			command_list_at = &command_list->next;
		}
	}
}

void CommandQueue::Flush()
{
	WaitForFenceValue(m_fence_value);
}
