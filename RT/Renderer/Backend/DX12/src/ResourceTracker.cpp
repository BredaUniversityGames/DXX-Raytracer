#include "ResourceTracker.hpp"
#include "GlobalDX.h"
#include "CommandList.h"

//------------------------------------------------------------------------

#define RESOURCE_TRACKER_VERBOSE_OUTPUT 0

using namespace RT;

namespace
{
	uint64_t HashResource(ID3D12Object *resource)
	{
		// Bottom bits of a pointer tend to be 0 because of alignment
		uint64_t result = (uint64_t)resource >> 4;
		return result;
	}
}

void D3D12ResourceTracker::TrackObject(RT_RESOURCE_TRACKER_DEBUG_PARAMS ID3D12Object* object, D3D12_RESOURCE_STATES initial_state)
{
	if (NEVER(!object))
		return;

	// allocate entry
	if (!m_first_free_resource)
	{
		m_first_free_resource = RT_ArenaAllocStruct(g_d3d.arena, ResourceEntry);
	}

	// initialize entry
	ResourceEntry* entry = RT_SLL_POP(m_first_free_resource);
	entry->resource = object;
	entry->state = initial_state;
	entry->command_list = nullptr;

	// debug info
	entry->track_line = line__;
	entry->track_file = file__;

	// insert entry into hashtable
	uint64_t hash = HashResource(object);
	uint64_t slot = hash % RT_ARRAY_COUNT(m_resource_table);
	RT_SLL_PUSH(m_resource_table[slot], entry);
}

void D3D12ResourceTracker::TrackTempObject(RT_RESOURCE_TRACKER_DEBUG_PARAMS ID3D12Object* object, CommandList* command_list)
{
	if (NEVER(!object))
		return;

	ResourceEntry* entry = FindResourceEntry(object);
	if (!entry)
	{
		TrackObject(RT_RESOURCE_TRACKER_FWD_ARGS object);
		entry = FindResourceEntry(object);
	}

	entry->command_list = command_list;
	entry->temp_track_line = line__;
	entry->temp_track_file = file__;
}

D3D12_RESOURCE_STATES D3D12ResourceTracker::GetResourceState(ID3D12Resource *resource)
{
	D3D12_RESOURCE_STATES result = D3D12_RESOURCE_STATE_COMMON;

	ResourceEntry *entry = FindResourceEntry(resource);

	if (ALWAYS(entry))
	{
		result = entry->state;
	}

	return result;
}

D3D12_RESOURCE_STATES D3D12ResourceTracker::Transition(ID3D12GraphicsCommandList *list, ID3D12Resource *resource, D3D12_RESOURCE_STATES dst_state)
{
	ResourceEntry *entry = FindResourceEntry(resource);

	D3D12_RESOURCE_STATES result = {};

	if (ALWAYS(entry))
	{
#if RESOURCE_TRACKER_VERBOSE_OUTPUT
		wchar_t name[256];
		GetName(entry->resource, sizeof(name), name);

		OutputDebugStringW(L"Transitioning resource: ");
		OutputDebugStringW(name);
		OutputDebugStringW(L"\n");
#endif

		// NOTE(Justin): We have to check here if the resource state is GENERIC_READ, to keep the debug layer from warning us when we transition
		// resources in the UPLOAD_HEAP. Maybe we should do this more elegantly?
		if (dst_state != entry->state && entry->state != D3D12_RESOURCE_STATE_GENERIC_READ)
		{
			D3D12_RESOURCE_BARRIER barrier = GetTransitionBarrier((ID3D12Resource*)entry->resource, entry->state, dst_state);
			list->ResourceBarrier(1, &barrier);
			entry->state = dst_state;
		}

		result = entry->state;
	}

	return result;
}

void D3D12ResourceTracker::Transitions(ID3D12GraphicsCommandList* list, size_t num_resources, ID3D12Resource** resources, D3D12_RESOURCE_STATES dst_state)
{
	RT::MemoryScope mem_scope;
	uint32_t num_barriers = 0;
	D3D12_RESOURCE_BARRIER* barriers = RT_ArenaAllocArray(mem_scope, num_resources, D3D12_RESOURCE_BARRIER);

	for (size_t res_index = 0; res_index < num_resources; ++res_index)
	{
		ResourceEntry* entry = FindResourceEntry(resources[res_index]);
		
		if (ALWAYS(entry))
		{
#if RESOURCE_TRACKER_VERBOSE_OUTPUT
			wchar_t name[256];
			GetName(entry->resource, sizeof(name), name);

			OutputDebugStringW(L"Transitioning resource: ");
			OutputDebugStringW(name);
			OutputDebugStringW(L"\n");
#endif

			if (dst_state != entry->state)
			{
				// Add barrier
				barriers[num_barriers++] = GetTransitionBarrier((ID3D12Resource*)entry->resource, entry->state, dst_state);
				entry->state = dst_state;
			}
		}
	}

	list->ResourceBarrier(num_barriers, barriers);
}

void D3D12ResourceTracker::Transitions(ID3D12GraphicsCommandList* list, size_t num_resources, ID3D12Resource** resources, D3D12_RESOURCE_STATES* dst_states)
{
	RT::MemoryScope mem_scope;
	uint32_t num_barriers = 0;
	D3D12_RESOURCE_BARRIER* barriers = RT_ArenaAllocArray(mem_scope, num_resources, D3D12_RESOURCE_BARRIER);

	for (size_t res_index = 0; res_index < num_resources; ++res_index)
	{
		ResourceEntry* entry = FindResourceEntry(resources[res_index]);

		if (ALWAYS(entry))
		{
#if RESOURCE_TRACKER_VERBOSE_OUTPUT
			wchar_t name[256];
			GetName(entry->resource, sizeof(name), name);

			OutputDebugStringW(L"Transitioning resource: ");
			OutputDebugStringW(name);
			OutputDebugStringW(L"\n");
#endif

			if (dst_states[res_index] != entry->state)
			{
				// Add barrier
				barriers[num_barriers++] = GetTransitionBarrier((ID3D12Resource*)entry->resource, entry->state, dst_states[res_index]);
				entry->state = dst_states[res_index];
			}
		}
	}

	list->ResourceBarrier(num_barriers, barriers);
}

void D3D12ResourceTracker::Release(ID3D12Object *resource)
{
	ResourceEntry **entry_slot = FindResourceEntrySlot(resource);

	if (entry_slot)
	{
		ResourceEntry *entry = *entry_slot;

		if (resource)
		{
			entry->resource->Release();
			entry->resource = nullptr;
		}

		// remove from hashtable
		*entry_slot = entry->next;

		// append to freelist
		RT_SLL_PUSH(m_first_free_resource, entry);
	}
	else
	{
		// Still call Release, so that this function can be a replacement for
		// any SafeRelease call
		if (resource)
		{
			resource->Release();
		}
	}
}

void D3D12ResourceTracker::ReleaseAllResources()
{
	for (size_t i = 0; i < RT_ARRAY_COUNT(m_resource_table); i++)
	{
		while (m_resource_table[i])
		{
			ResourceEntry *entry = RT_SLL_POP(m_resource_table[i]);

			entry->resource->Release();
			entry->resource = nullptr;

			RT_SLL_PUSH(m_first_free_resource, entry);
		}
	}
}

void RT::D3D12ResourceTracker::ReleaseStaleTempResources(uint64_t fence_value)
{
	for (size_t i = 0; i < RT_ARRAY_COUNT(m_resource_table); ++i)
	{
		// Release temp resources if their fence value has been reached
		// (operation has been executed on the GPU and the resource is no longer in-flight)
		for (ResourceEntry** entry_at = &m_resource_table[i]; *entry_at;)
		{
			ResourceEntry* entry = *entry_at;
			
			if (entry->command_list &&
				entry->command_list->GetFenceValue() <= fence_value)
			{
				entry->resource->Release();
				entry->resource = nullptr;

				*entry_at = entry->next;

				RT_SLL_PUSH(m_first_free_resource, entry);
			}
			else
			{
				entry_at = &entry->next;
			}
		}
	}
}

//------------------------------------------------------------------------

D3D12ResourceTracker::ResourceEntry **D3D12ResourceTracker::FindResourceEntrySlot(ID3D12Object *resource)
{
	ResourceEntry **result = nullptr;

	uint64_t hash = HashResource(resource);
	uint64_t slot = hash % RT_ARRAY_COUNT(m_resource_table);
	
	for (ResourceEntry **entry_at = &m_resource_table[slot]; *entry_at; entry_at = &(*entry_at)->next)
	{
		ResourceEntry *entry = *entry_at;

		if (entry->resource == resource)
		{
			result = entry_at;
			break;
		}
	}

	return result;
}

D3D12ResourceTracker::ResourceEntry *D3D12ResourceTracker::FindResourceEntry(ID3D12Object *resource)
{
	RT_ASSERT(resource);

	ResourceEntry *result = nullptr;
	ResourceEntry **slot = FindResourceEntrySlot(resource);

	if (slot)
	{
		result = *slot;
	}

	return result;
}
