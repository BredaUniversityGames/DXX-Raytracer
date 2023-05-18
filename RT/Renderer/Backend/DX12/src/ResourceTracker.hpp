#pragma once

//------------------------------------------------------------------------

#include <d3d12.h>
#include "ApiTypes.h"

//------------------------------------------------------------------------

namespace RT
{

	class CommandList;

	// Macros for passing around source code location info
	#define RT_RESOURCE_TRACKER_DEBUG_PARAMS int line__, const char *file__, 
	#define RT_RESOURCE_TRACKER_DEBUG_ARGS   __LINE__, __FILE__, 
	#define RT_RESOURCE_TRACKER_FWD_ARGS line__, file__, 
	#define RT_TRACK_RESOURCE(resource, state) g_d3d.resource_tracker.Track(RT_RESOURCE_TRACKER_DEBUG_ARGS resource, state)
	#define RT_RELEASE_RESOURCE(resource) g_d3d.resource_tracker.Release(resource)
	#define RT_TRACK_TEMP_RESOURCE(resource, command_list) g_d3d.resource_tracker.TrackTemp(RT_RESOURCE_TRACKER_DEBUG_ARGS resource, command_list)

	class D3D12ResourceTracker
	{
	public:
		//------------------------------------------------------------------------
		// Public Methods

		// You have to pass RT_RESOURCE_TRACKER_DEBUG_ARGS before any other arguments. No comma after the macro.
		void Track(RT_RESOURCE_TRACKER_DEBUG_PARAMS ID3D12Resource *resource, D3D12_RESOURCE_STATES initial_state);
		void TrackTemp(RT_RESOURCE_TRACKER_DEBUG_PARAMS ID3D12Resource* resource, CommandList* command_list);

		D3D12_RESOURCE_STATES GetResourceState(ID3D12Resource *resource);
		D3D12_RESOURCE_STATES Transition(ID3D12GraphicsCommandList *list, ID3D12Resource *resource, D3D12_RESOURCE_STATES dst_state);
		void Transitions(ID3D12GraphicsCommandList* list, size_t num_resources, ID3D12Resource** resources, D3D12_RESOURCE_STATES dst_state);
		void Transitions(ID3D12GraphicsCommandList* list, size_t num_resources, ID3D12Resource** resources, D3D12_RESOURCE_STATES* dst_states);

		// Calls ->Release() on the resource and stops tracking it.
		// Safe to call on untracked resource or nullptrs.
		void Release(ID3D12Resource *resource);
		void ReleaseAllResources();
		void ReleaseStaleTempResources(uint64_t fence_value);

	private:
		//------------------------------------------------------------------------

		struct ResourceEntry
		{
			ResourceEntry *next;

			ID3D12Resource *resource;
			D3D12_RESOURCE_STATES state;
			CommandList* command_list;

			// Debug info
			int track_line;
			const char* track_file;

			int temp_track_line;
			const char* temp_track_file;
		};

		//------------------------------------------------------------------------
		// Private Methods

		ResourceEntry **FindResourceEntrySlot(ID3D12Resource *resource);
		ResourceEntry  *FindResourceEntry    (ID3D12Resource *resource);

		//------------------------------------------------------------------------
		// Data

		ResourceEntry *m_first_free_resource;
		ResourceEntry *m_resource_table[1024];
	};

}