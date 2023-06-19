#pragma once

#include "ApiTypes.h"

namespace RT
{

	struct TrackedMeshData
	{
		RT_Mat4 prev_transform;
	};

	class MeshTracker
	{
	public:
		void Init(RT_Arena *arena);
		bool GetTrackedMeshData(uint64_t key, uint64_t frame_index, TrackedMeshData **data);
		void PruneOldEntries(uint64_t frame_index);

	private:
		struct Entry
		{
			Entry *next;
			uint64_t key;
			uint64_t last_touched_frame_index;
			TrackedMeshData data;
		};

		RT_Arena *m_arena;
		Entry *m_first_free;
		Entry *m_table[1024];
	};

}
