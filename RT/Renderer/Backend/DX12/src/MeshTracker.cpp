#include "MeshTracker.hpp"

#include "Core/Arena.h"

using namespace RT;

void MeshTracker::Init(RT_Arena *arena)
{
	m_arena      = arena;
	m_first_free = nullptr;
}

bool MeshTracker::GetTrackedMeshData(uint64_t key, uint64_t frame_index, TrackedMeshData **data)
{
	bool result = false;

	uint64_t slot = key % RT_ARRAY_COUNT(m_table);
	
	Entry *entry = nullptr;
	for (Entry *test_entry = m_table[slot]; test_entry; test_entry = test_entry->next)
	{
		if (test_entry->key == key)
		{
			entry  = test_entry;
			result = true;
			break;
		}
	}

	if (!entry)
	{
		if (!m_first_free)
		{
			m_first_free = RT_ArenaAllocStructNoZero(m_arena, Entry);
			m_first_free->next = nullptr;
		}

		entry = RT_SLL_POP(m_first_free);
		RT_ZERO_STRUCT(entry);

		entry->next = m_table[slot];
		m_table[slot] = entry;
	}

	entry->key = key;
	entry->last_touched_frame_index = frame_index;

	if (data)
	{
		*data = &entry->data;
	}

	return result;
}

void MeshTracker::PruneOldEntries(uint64_t frame_index)
{
	for (size_t index = 0; index < RT_ARRAY_COUNT(m_table); index++)
	{
		for (Entry **entry_at = &m_table[index]; *entry_at;)
		{
			Entry *entry = *entry_at;

			uint64_t frame_difference = frame_index - entry->last_touched_frame_index;
			if (frame_difference > 1)
			{
				*entry_at = entry->next;
				RT_SLL_PUSH(m_first_free, entry);
			}
			else
			{
				entry_at = &entry->next;
			}
		}
	}
}