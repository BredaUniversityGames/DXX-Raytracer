#include "CommandList.h"
#include "GlobalDX.h"

using namespace RT;

CommandList::CommandList(D3D12_COMMAND_LIST_TYPE type)
    : m_d3d12_command_list_type(type)
{
    DX_CALL(g_d3d.device->CreateCommandAllocator(m_d3d12_command_list_type, IID_PPV_ARGS(&m_d3d12_command_allocator)));
    DX_CALL(g_d3d.device->CreateCommandList(0, m_d3d12_command_list_type, m_d3d12_command_allocator, nullptr, IID_PPV_ARGS(&m_d3d12_graphics_command_list2)));
}

CommandList::~CommandList()
{
    SafeRelease(m_d3d12_graphics_command_list2);
    SafeRelease(m_d3d12_command_allocator);
}

void CommandList::Close()
{
    m_d3d12_graphics_command_list2->Close();
}

void CommandList::Reset()
{
    DX_CALL(m_d3d12_command_allocator->Reset());
    DX_CALL(m_d3d12_graphics_command_list2->Reset(m_d3d12_command_allocator, nullptr));
}
