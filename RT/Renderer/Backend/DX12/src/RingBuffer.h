#pragma once
#include "WinIncludes.h"

namespace RT
{

	class CommandList;

	struct RingBuffer
	{
		ID3D12Resource* resource;

		size_t total_byte_size;
		uint8_t* base_ptr;
		uint8_t* write_ptr;

		CommandList* command_list;
		bool commands_recorded;
	};

	struct RingBufferAllocation
	{
		ID3D12Resource* resource;
		CommandList* command_list;

		size_t byte_size;
		size_t byte_offset;
		uint8_t* ptr;
	};

	// Creates a ring buffer
	// Releasing ring buffers is handled in the resource tracker, because the underlying D3D12 resource is tracked there
	RingBuffer CreateRingBuffer(size_t byte_size);
	void ResetRingBuffer(RingBuffer* ring_buf);
	void FlushRingBuffer(RingBuffer* ring_buf);

	// Suballocate from the ring buffer
	RingBufferAllocation AllocateFromRingBuffer(RingBuffer* ring_buf, size_t num_bytes, size_t align);
	// Suballocate from the ring buffer and immediately copy data to it
	RingBufferAllocation WriteToRingBuffer(RingBuffer* ring_buf, size_t num_bytes, size_t align, const void* data_ptr);

}
