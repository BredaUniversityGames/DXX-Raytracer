#include "RingBuffer.h"
#include "GlobalDX.h"
#include "Resource.h"
#include "CommandQueue.h"
#include "CommandList.h"

namespace RT
{

	RingBuffer CreateRingBuffer(size_t byte_size)
	{
		D3D12_RANGE read_range = { 0, 0 };

		RingBuffer ring_buf = {};
		ring_buf.resource = RT_CreateUploadBuffer(L"Ring buffer resource", byte_size);
		ring_buf.resource->Map(0, &read_range, (void**)&ring_buf.base_ptr);
		ring_buf.write_ptr = ring_buf.base_ptr;
		ring_buf.total_byte_size = byte_size;
		ring_buf.command_list = &g_d3d.command_queue_direct->GetCommandList();
		ring_buf.commands_recorded = false;

		return ring_buf;
	}

	void AlignPointer(RingBuffer* ring_buf, size_t align)
	{
		// Aligns the write pointer to the alignment required
		ring_buf->write_ptr = (uint8_t*)RT_ALIGN_POW2((size_t)ring_buf->write_ptr, align);
	}

	void ResetRingBuffer(RingBuffer* ring_buf)
	{
		// Resets the write pointer of the ring buffer to its base pointer
		ring_buf->write_ptr = ring_buf->base_ptr;
	}

	void FlushRingBuffer(RingBuffer* ring_buf)
	{
		// If there were no commands recorded on the ring buffer, dont flush, this would cause an unnecessary CPU stall otherwise
		if (ring_buf->commands_recorded && ring_buf->command_list)
		{
			uint64_t fence_value = g_d3d.command_queue_direct->ExecuteCommandList(*ring_buf->command_list);
			g_d3d.command_queue_direct->WaitForFenceValue(fence_value);
			ring_buf->command_list = &g_d3d.command_queue_direct->GetCommandList();

			ring_buf->commands_recorded = false;
		}
	}

	void MakeSpaceForAllocation(RingBuffer* ring_buf, size_t num_bytes, size_t align)
	{
		// Align the pointer and calculate the total byte size left until the end of the buffer
		AlignPointer(ring_buf, align);
		size_t byte_size_left = ring_buf->total_byte_size - (ring_buf->write_ptr - ring_buf->base_ptr);

		// If we cannot satisfy the allocation, the ring buffer is full, so we need to flush all commands to not accidently overwriting in-flight copies
		if (byte_size_left < num_bytes)
		{
			FlushRingBuffer(ring_buf);
			ResetRingBuffer(ring_buf);
		}
	}

	void AdvancePointer(RingBuffer* ring_buf, size_t num_bytes)
	{
		// Advance the pointer by the amount of bytes requested
		ring_buf->write_ptr += num_bytes;
		size_t write_ptr_byte_location = (ring_buf->write_ptr - ring_buf->base_ptr);

		// If the write pointer is out of the total byte size, reset it
		if (write_ptr_byte_location >= ring_buf->total_byte_size)
		{
			FlushRingBuffer(ring_buf);
			ResetRingBuffer(ring_buf);
		}
	}

	RingBufferAllocation MakeAllocation(RingBuffer* ring_buf, size_t num_bytes)
	{
		RingBufferAllocation ring_buf_alloc = {};
		ring_buf_alloc.resource = ring_buf->resource;
		ring_buf_alloc.command_list = ring_buf->command_list;
		ring_buf_alloc.byte_size = num_bytes;
		ring_buf_alloc.byte_offset = ring_buf->write_ptr - ring_buf->base_ptr;
		ring_buf_alloc.ptr = ring_buf->write_ptr;

		ring_buf->commands_recorded = true;

		return ring_buf_alloc;
	}

	RingBufferAllocation AllocateFromRingBuffer(RingBuffer* ring_buf, size_t num_bytes, size_t align)
	{
		// Make space for the allocation request, and advance the write pointer
		MakeSpaceForAllocation(ring_buf, num_bytes, align);
		RingBufferAllocation ring_buf_alloc = MakeAllocation(ring_buf, num_bytes);
		AdvancePointer(ring_buf, num_bytes);

		return ring_buf_alloc;
	}

	RingBufferAllocation WriteToRingBuffer(RingBuffer* ring_buf, size_t num_bytes, size_t align, const void* data_ptr)
	{
		// Make space for the allocation request, and advance the write pointer, but also copy the data into the allocation already
		RingBufferAllocation ring_buf_alloc = AllocateFromRingBuffer(ring_buf, num_bytes, align);
		memcpy((void*)ring_buf_alloc.ptr, data_ptr, num_bytes);

		return ring_buf_alloc;
	}

}
