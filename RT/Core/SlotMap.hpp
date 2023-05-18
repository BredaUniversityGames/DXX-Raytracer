#pragma once

#include <utility>

#include "ApiTypes.h" 
#include "Core/VirtualMemory.h"

namespace RT
{

	template <typename T>
    class SlotMap
    {
        static constexpr uint32_t OCCUPIED = 0xFFFFFFFF;

    public:
        SlotMap(uint32_t capacity)
            : m_capacity(capacity)
        {
            // NOTE(daniel): Virtual memory is always cleared to 0
            m_slots = reinterpret_cast<Slot *>(RT_ReserveVirtualMemory(sizeof(Slot) * m_capacity));
            RT_CommitVirtualMemory(m_slots, sizeof(Slot) * m_capacity);

            for (size_t i = 0; i < m_capacity - 1; i++)
            {
                m_slots[i].next_free = (uint32_t)(i + 1);
            }
        }

        ~SlotMap()
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                for (size_t i = 1; i < m_capacity; i++)
                {
                    Slot *slot = &m_slots[i];
                    if (slot->next_free == OCCUPIED)
                    {
                        slot->resource.~T();
                    }
                }
            }
            RT_ReleaseVirtualMemory(m_slots);
        }

        SlotMap(const SlotMap&) = delete;
        SlotMap(SlotMap &&)     = delete;

        RT_ResourceHandle Insert(T &&resource)
        {
            RT_ResourceHandle result = AllocateSlot();

            Slot* slot = &m_slots[result.index];
            new (&slot->resource) T(std::move(resource));

            return result;
        }

        RT_ResourceHandle Insert(const T &resource)
        {
            RT_ResourceHandle result = AllocateSlot();

            Slot* slot = &m_slots[result.index];
            new (&slot->resource) T(resource);

            return result;
        }

        void Remove(RT_ResourceHandle handle)
        {
            T *result = nullptr;

            Slot *sentinel = &m_slots[0];
            if (RT_RESOURCE_HANDLE_VALID(handle))
            {
                Slot* slot = &m_slots[handle.index];
                if (handle.generation == slot->generation)
                {
                    slot->generation += 1;

                    slot->next_free = sentinel->next_free;
                    sentinel->next_free = handle.index;

                    // @NonTrivial: Call destructor as needed
                    if constexpr (!std::is_trivially_destructible_v<T>)
                    {
                        slot->resource.~T();
                    }
                }
            }
        }

        T *Find(RT_ResourceHandle handle) const
        {
            T *result = nullptr;

            if (RT_RESOURCE_HANDLE_VALID(handle))
            {
                Slot* slot = &m_slots[handle.index];
                if (handle.generation == slot->generation)
                {
                    result = &slot->resource;
                }
            }

            return result;
        }

    private:
        RT_ResourceHandle AllocateSlot()
        {
            Slot* sentinel = &m_slots[0];

            RT_ResourceHandle result = {};

            if (ALWAYS(sentinel->next_free))
            {
                uint32_t index = sentinel->next_free;

                Slot* slot = &m_slots[index];
                sentinel->next_free = slot->next_free;

                slot->next_free = OCCUPIED;

                result = { index, slot->generation };
            }

            return result;
        }

        struct Slot
        {
            uint32_t next_free;
            uint32_t generation;
            T resource;
        };

        size_t   m_capacity;
        Slot    *m_slots;
    };

}