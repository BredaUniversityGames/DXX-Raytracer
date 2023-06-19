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

	// This will track any ID3D12Object* as a peristent object
	#define RT_TRACK_OBJECT(object) g_d3d.resource_tracker.TrackObject(RT_RESOURCE_TRACKER_DEBUG_ARGS object)
	// This will track any ID3D12Resource* as a persistent resource, and set its inital state for automatic state detection when doing resource transitions
	#define RT_TRACK_RESOURCE(resource, state) g_d3d.resource_tracker.TrackObject(RT_RESOURCE_TRACKER_DEBUG_ARGS resource, state)
	// This will track an object as temporary, meaning that it will be released auomatically when the fence value of the command list has been reached.
	// Note: If the object passed in here has not been tracked as persistent before, it will create a new resource entry for this resource
	#define RT_TRACK_TEMP_OBJECT(object, command_list) g_d3d.resource_tracker.TrackTempObject(RT_RESOURCE_TRACKER_DEBUG_ARGS object, command_list)
	// This will track a resource as temporary, meaning that it will be released auomatically when the fence value of the command list has been reached.
	// Note: If the resource passed in here has not been tracked as persistent before, it will create a new resource entry for this resource and set its state to D3D12_RESOURCE_STATE_COMMON
	#define RT_TRACK_TEMP_RESOURCE(resource, command_list) g_d3d.resource_tracker.TrackTempObject(RT_RESOURCE_TRACKER_DEBUG_ARGS resource, command_list)
	#define RT_RELEASE_RESOURCE(resource) g_d3d.resource_tracker.Release(resource)
	#define RT_RELEASE_OBJECT(object) g_d3d.resource_tracker.Release(object)

	class D3D12ResourceTracker
	{
	public:
		//------------------------------------------------------------------------
		// Public Methods

		// You have to pass RT_RESOURCE_TRACKER_DEBUG_ARGS before any other arguments. No comma after the macro.
		void TrackObject(RT_RESOURCE_TRACKER_DEBUG_PARAMS ID3D12Object* object, D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON);
		void TrackTempObject(RT_RESOURCE_TRACKER_DEBUG_PARAMS ID3D12Object* object, CommandList* command_list);

		D3D12_RESOURCE_STATES GetResourceState(ID3D12Resource *resource);
		D3D12_RESOURCE_STATES Transition(ID3D12GraphicsCommandList *list, ID3D12Resource *resource, D3D12_RESOURCE_STATES dst_state);
		void Transitions(ID3D12GraphicsCommandList* list, size_t num_resources, ID3D12Resource** resources, D3D12_RESOURCE_STATES dst_state);
		void Transitions(ID3D12GraphicsCommandList* list, size_t num_resources, ID3D12Resource** resources, D3D12_RESOURCE_STATES* dst_states);

		// Calls ->Release() on the resource and stops tracking it.
		// Safe to call on untracked resource or nullptrs.
		void Release(ID3D12Object* object);
		void ReleaseAllResources();
		void ReleaseStaleTempResources(uint64_t fence_value);

	private:
		//------------------------------------------------------------------------

		struct ResourceEntry
		{
			ResourceEntry *next;

			ID3D12Object *resource;
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

		ResourceEntry **FindResourceEntrySlot(ID3D12Object *resource);
		ResourceEntry  *FindResourceEntry    (ID3D12Object *resource);

		//------------------------------------------------------------------------
		// Data

		ResourceEntry *m_first_free_resource;
		ResourceEntry *m_resource_table[1024];

	};

}