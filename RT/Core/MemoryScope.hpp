#pragma once

// insane-o-style
#include <utility>

// ------------------------------------------------------------------

#include "Arena.h"

namespace RT
{

template <typename T>
static inline void CallObjectDestructor(void *object)
{
	// yes.
	reinterpret_cast<T *>(object)->~T();
}

class MemoryScope
{
public:
	explicit MemoryScope(RT_Arena *arena)
		: m_marker{ RT_ArenaGetMarker(arena) }
	{
		// wonderful
	}

	MemoryScope()
		: m_marker { RT_ArenaGetMarker(&g_thread_arena) }
	{
		// if you don't pass an arena when constructing a MemoryScope,
		// it uses the thread local arena. Use this power judiciously,
		// but wisely.
	}

	~MemoryScope()
	{
		while (m_first_cleanup)
		{
			CleanupTracker *tracker = m_first_cleanup;
			m_first_cleanup = tracker->next;

			void *object = reinterpret_cast<void *>(tracker + 1); // the object is directly after the tracker (unless it was allocated with an align greater than 16)
																  // TODO FIXME: Make it work for greater aligns?

			tracker->Destructor(object);
		}

		RT_ArenaResetToMarker(m_marker.arena, m_marker);
	}

	// ------------------------------------------------------------------

	RT_Arena *operator->()
	{
		return m_marker.arena;
	}

	operator RT_Arena *()
	{
		return m_marker.arena;
	}

	// ------------------------------------------------------------------

	template <typename T, typename ...Args>
	T *New(Args &&...args)
	{
		if constexpr(!std::is_trivially_destructible<T>::value)
		{
			CleanupTracker *tracker = RT_ArenaAllocStructNoZero(m_marker.arena, CleanupTracker);
			tracker->Destructor = CallObjectDestructor<T>;

			tracker->next = m_first_cleanup;
			m_first_cleanup = tracker;
		}

		T *result = RT_ArenaAllocStructNoZero(m_marker.arena, typename T);
		result = new (result) T{ static_cast<Args &&>(args)... };

		return result;
	}

private:
	RT_ArenaMarker m_marker = {};

	struct CleanupTracker
	{
		CleanupTracker *next;
		void (*Destructor)(void *object);
	};

	// Using a linked list, this is a FILO queue that calls destructors of all the allocated objects
	// once the MemoryScope gets destructed. Very cool!
	CleanupTracker *m_first_cleanup = nullptr;
};

}