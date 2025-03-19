// ------------------------------------------------------------------
// Game-code includes

#include "Level.h"
#include "segment.h"
#include "textures.h"
#include "gameseq.h"
#include "wall.h"
#include "automap.h"
#include "render.h"

// ------------------------------------------------------------------
// RT includes

#include "Core/MiniMath.h"
#include "Core/Arena.h"
#include "Core/String.h"
#include "RTgr.h"
#include "RTmaterials.h"
#include "Renderer.h"
#include "Lights.h"

// ------------------------------------------------------------------

// Current active level
RT_ResourceHandle g_level_resource = { 0 };
RT_ResourceHandle g_level_with_portals_resource = { 0 };
int g_active_level = 0;

int m_light_count = 0;
RT_Light m_lights[1024] = {0};
int m_lights_definitions[1024] = {0};
side* m_extracted_light_sides[1024] = {0};

short m_lights_seg_ids[1024] = {-1};
short m_lights_relevance_score[1024] = { 0.0f };
short m_lights_to_sort[1024];
int m_lights_found = 0;

RT_Triangle RT_TriangleFromIndices(RT_Vertex* verts, int vert_offset, int v0, int v1, int v2, int tmap, bool portal, int segment, int segment_adjacent)
{
	RT_Triangle triangle = { 0 };

	triangle.material_edge_index = tmap;

	RT_Vec3 pos0 = verts[vert_offset + v0].pos;
	RT_Vec3 pos1 = verts[vert_offset + v1].pos;
	RT_Vec3 pos2 = verts[vert_offset + v2].pos;

	RT_Vec3 p10 = RT_Vec3Normalize(RT_Vec3Sub(pos1, pos0));
	RT_Vec3 p20 = RT_Vec3Normalize(RT_Vec3Sub(pos2, pos0));

	RT_Vec3 normal = RT_Vec3Normalize(RT_Vec3Cross(p10, p20));

	triangle.pos0 = pos0;
	triangle.pos1 = pos1;
	triangle.pos2 = pos2;
	triangle.normal0 = normal;
	triangle.normal1 = normal;
	triangle.normal2 = normal;
	triangle.uv0 = verts[vert_offset + v0].uv;
	triangle.uv1 = verts[vert_offset + v1].uv;
	triangle.uv2 = verts[vert_offset + v2].uv;
	triangle.color = 0xFFFFFFFF;

	triangle.portal = portal;
	triangle.segment = segment;
	triangle.segment_adjacent = segment_adjacent;

	return triangle;
}

void RT_ExtractLightsFromSide(side* side, RT_Vertex* vertices, RT_Vec3 normal, int seg_id)
{
	int light_index = RT_IsLight(side->tmap_num2 & 0x3FFF);
	if (light_index < 0)
	{
		light_index = RT_IsLight(side->tmap_num & 0x3FFF);
	}
	
	
	if (light_index > -1)
	{
		RT_Vec2 uv_min = RT_Vec2Make(INFINITY, INFINITY);
		RT_Vec2 uv_max = RT_Vec2Make(-INFINITY, -INFINITY);
		for (int i = 0; i < 4; i++)
		{
			RT_Vec2 uv = RT_Vec2Make((f2fl(side->uvls[i].u)),(f2fl(side->uvls[i].v)));
			
			uv_min = RT_Vec2Min(uv, uv_min);
			uv_max = RT_Vec2Max(uv, uv_max);
		}

		bool multiple_lights = false;
		if(uv_min.x < -1.0 || uv_min.y < -1.0 || uv_max.x > 1.0 || uv_max.y > 1.0)
		{
			RT_Vec2 uv = RT_Vec2Sub(uv_max, uv_min);
			RT_Vec2 light_size = g_light_definitions[light_index].size;

			int num_x = max((int)(uv.x / light_size.x),1);
			int num_y = max((int)(uv.y / light_size.y),1);

			RT_LOGF(RT_LOGSERVERITY_INFO, "Creating lights in the following directions. {X: %i, Y: %i}", num_x, num_y);
			if(num_x > 1 && num_y > 1)
			{
				RT_LOGF(RT_LOGSERVERITY_INFO, "Multiple lights created!");
				multiple_lights = true;
			}
		}

		if (!multiple_lights) 
		{
			if (ALWAYS(m_light_count < RT_ARRAY_COUNT(m_lights)))
			{
				m_lights[m_light_count] = RT_InitLight(g_light_definitions[light_index], vertices, normal);
				m_lights_definitions[m_light_count] = light_index;
				m_extracted_light_sides[m_light_count] = side;

				m_lights_seg_ids[m_light_count] = seg_id;
				m_light_count++;
			}
		}
	}
}

bool RT_UploadLevelGeometry(RT_ResourceHandle* level_handle, RT_ResourceHandle* portals_handle)
{

	// set all textures next/future state to not loaded
	for (uint16_t bm_index = 1; bm_index < MAX_BITMAP_FILES; bm_index++)
	{
		RT_Material* def = &g_rt_materials[bm_index];
		if (!def->always_load_texture)
			def->texture_load_state_next = RT_MaterialTextureLoadState_Unloaded;
	}

	RT_ArenaMemoryScope(&g_thread_arena)
	{
		RT_Vertex* verts = RT_ArenaAllocArray(&g_thread_arena, Num_segments * 6 * 4, RT_Vertex);
		RT_Triangle* triangles = RT_ArenaAllocArray(&g_thread_arena, Num_segments * 6 * 2, RT_Triangle);
		RT_Triangle* portal_triangles = RT_ArenaAllocArray(&g_thread_arena, Num_segments * 6 * 2, RT_Triangle);

		// Init lights segment id list
		for (size_t i = 0; i < _countof(m_lights_seg_ids); ++i) {
			m_lights_seg_ids[i] = -1;
		}

		int num_verts = 0;
		int num_indices = 0; 
		int num_triangles = 0;
		int num_portal_triangles = 0;

		int num_mesh = 0;
		for (int seg_id = 0; seg_id < Num_segments; seg_id++)
		{
			segment *seg = &Segments[seg_id];

			for (int side_index = 0; side_index < MAX_SIDES_PER_SEGMENT; side_index++)
			{
				const int vertex_offset = num_verts;
				const int indices_offset = num_indices;

				side* s = &seg->sides[side_index];
				int vertnum_list[4];
				get_side_verts(&vertnum_list, seg_id, side_index);

				int vert_ids[4];

				for (int v = 0; v < 4; v++)
				{
					// Extract Vertex Data
					const int vertex_id = vertnum_list[v];
					vms_vector raw_vertex = Vertices[vertex_id];
					vms_vector raw_normal = s->normals[0]; // if quadrilateral use this as normal.

					RT_Vertex vert =
					{
						f2fl(raw_vertex.x),
						f2fl(raw_vertex.y),
						f2fl(raw_vertex.z),
						f2fl(s->uvls[v].u),
						f2fl(s->uvls[v].v),
						f2fl(raw_normal.x),
						f2fl(raw_normal.y),
						f2fl(raw_normal.z)
					};
					verts[vertex_offset + v] = vert;
					num_verts++;

					//assert(vert.uv.x >= -10.0 && vert.uv.x <= 10.0);
				}


				// Mark invisible walls as portals
				bool is_portal = true;
				bool is_wall = false;
				if (seg->children[side_index] == -1)
				{
					is_portal = false;
				}
				else if (s->wall_num != -1)
				{
					wall* w = &Walls[s->wall_num];
					// TODO(daniel): What about blastable wallls?
					if (w->type != WALL_OPEN)
					{
						is_wall = true;
					}
				}
				else if (seg->children[side_index] == -2)
				{
					is_portal = true;
				}

				int absolute_side_index = MAX_SIDES_PER_SEGMENT * seg_id + side_index;

				// Create two different meshes, one for visible level geo and one for portals (including walls) for retracing rays

				// if not a portal surface or is a wall add to regular level geometry
				if (!is_portal || is_wall)
				{
					switch (s->type)
					{
					case SIDE_IS_TRI_13:
						triangles[num_triangles++] = RT_TriangleFromIndices(verts, vertex_offset, 0, 1, 3, absolute_side_index, false, seg_id, seg->children[side_index]);
						triangles[num_triangles++] = RT_TriangleFromIndices(verts, vertex_offset, 1, 2, 3, absolute_side_index, false, seg_id, seg->children[side_index]);
						break;

					case SIDE_IS_QUAD:
					case SIDE_IS_TRI_02:
					default:
						triangles[num_triangles++] = RT_TriangleFromIndices(verts, vertex_offset, 0, 1, 2, absolute_side_index, false, seg_id, seg->children[side_index]);
						triangles[num_triangles++] = RT_TriangleFromIndices(verts, vertex_offset, 0, 2, 3, absolute_side_index, false, seg_id, seg->children[side_index]);
						break;
					}
				}
				
				if (is_portal || is_wall) // portals and walls to portals mesh
				{
					switch (s->type)
					{
					case SIDE_IS_TRI_13:
						portal_triangles[num_portal_triangles++] = RT_TriangleFromIndices(verts, vertex_offset, 0, 1, 3, absolute_side_index, is_portal, seg_id, seg->children[side_index]);
						portal_triangles[num_portal_triangles++] = RT_TriangleFromIndices(verts, vertex_offset, 1, 2, 3, absolute_side_index, is_portal, seg_id, seg->children[side_index]);
						break;

					case SIDE_IS_QUAD:
					case SIDE_IS_TRI_02:
					default:
						portal_triangles[num_portal_triangles++] = RT_TriangleFromIndices(verts, vertex_offset, 0, 1, 2, absolute_side_index, is_portal, seg_id, seg->children[side_index]);
						portal_triangles[num_portal_triangles++] = RT_TriangleFromIndices(verts, vertex_offset, 0, 2, 3, absolute_side_index, is_portal, seg_id, seg->children[side_index]);
						break;
					}
				}
				
				
				RT_ExtractLightsFromSide(s, &verts[vertex_offset], triangles[num_triangles - 1].normal0, seg_id);

				// Set tmaps next/future state to loaded so they will be loaded into memory
				for (char topbottom = 0; topbottom < 2; topbottom++)
				{
					// convert tmap_num to bitmap index

					ushort bm_index;
					if (topbottom == 0)
						bm_index = Textures[s->tmap_num & 0x3FFF].index;
					else if ( (s->tmap_num2 & 0x3FFF) != 0)
						bm_index = Textures[s->tmap_num2 & 0x3FFF].index;
					else
					{
						// No overlay texture found
						continue;
					}

					
					// get the bitmap name
					char bitmap_name[13];
					piggy_get_bitmap_name(bm_index, bitmap_name);
					
					// check if material is part of an animation sequence and set all the frames of the material to load as well
					const char* pound_pos = strchr(bitmap_name, '#');
					if (pound_pos)
					{
						// texture animation found
						int char_count = pound_pos - bitmap_name;  // subtract the two pointers to get the length

						char animation_root_name[13];
						RT_SaneStrncpy(animation_root_name, bitmap_name, char_count + 2);

						int frame_num = 0;
						char animation_frame_name[13];
						bitmap_index frame_index;
						frame_index.index = 0;

						//
						do
						{
							sprintf(animation_frame_name, "%s%d", animation_root_name, frame_num);

							frame_index = piggy_find_bitmap(animation_frame_name);

							RT_Material* def = &g_rt_materials[frame_index.index];
							if (frame_index.index != 0 && !def->always_load_texture)
								def->texture_load_state_next = RT_MaterialTextureLoadState_Loaded;

							frame_num++;

						} while (frame_index.index != 0);
					}
					else // not part of animation sequence... just set the one texture to load
					{
						// set this material to load
						RT_Material* def = &g_rt_materials[bm_index];
						if (!def->always_load_texture)
							def->texture_load_state_next = RT_MaterialTextureLoadState_Loaded;
					}
				}
				
			}
		}

		// NOTE(daniel): This is a separate call, because I don't want to do something tweaky like
		// detecting whether tangents need to be calculated in RT_UploadMesh. You, the uploader, should know.
		RT_GenerateTangents(triangles, num_triangles);
		RT_GenerateTangents(portal_triangles, num_portal_triangles);

		RT_UploadMeshParams params =
		{
			.triangle_count = num_triangles,
			.triangles      = triangles,
		};

		RT_LOGF(RT_LOGSERVERITY_INFO, "UPLOADING MESH >>\n");
		*level_handle = RT_UploadMesh(&params);
		RT_LOGF(RT_LOGSERVERITY_INFO, "UPLOADING MESH OK\n");

		RT_UploadMeshParams params_portals =
		{
			.triangle_count = num_portal_triangles,
			.triangles = portal_triangles,
		};

		RT_LOGF(RT_LOGSERVERITY_INFO, "UPLOADING MESH >>\n");
		*portals_handle = RT_UploadMesh(&params_portals);
		RT_LOGF(RT_LOGSERVERITY_INFO, "UPLOADING MESH OK\n");

		// load and unload materials based on if they are needed for this level.
		RT_SyncMaterialStates();
	}

	return true;
}

bool RT_UnloadLevel()
{
	// Only unload if a level acceleration structure actually exists
	if (RT_RESOURCE_HANDLE_VALID(g_level_resource))
	{
		RT_ReleaseMesh(g_level_resource);
		g_level_resource = RT_RESOURCE_HANDLE_NULL;

		m_light_count = 0;
		memset(m_lights, 0, sizeof(RT_Light) * 1024);

		return true;
	}

	// now unload the version with portals
	if (RT_RESOURCE_HANDLE_VALID(g_level_with_portals_resource))
	{
		RT_ReleaseMesh(g_level_with_portals_resource);
		g_level_with_portals_resource = RT_RESOURCE_HANDLE_NULL;

		return true;
	}

	return false;
}

bool RT_LoadLevel() 
{
	// Load a level only if a level acceleration structure does not exist yet
	if (!RT_RESOURCE_HANDLE_VALID(g_level_resource))
	{
		assert(!RT_RESOURCE_HANDLE_VALID(g_level_resource));
		// Load level geometry
		RT_UploadLevelGeometry(&g_level_resource, &g_level_with_portals_resource);
		RT_ResetLightEmission();		// added this here as resetting light emission was getting skipped when doing a level warp
		g_active_level = Current_level_num;

		return RT_RESOURCE_HANDLE_VALID(g_level_resource) && RT_RESOURCE_HANDLE_VALID(g_level_with_portals_resource);
	}

	return false;
}

void RT_RenderLevel(RT_Vec3 player_pos) 
{
	// ------------------------------------------------------------------
	RT_UpdateMaterialEdges();
	RT_UpdateMaterialIndices();

	RT_FindAndSubmitNearbyLights(player_pos);

	RT_Mat4 mat = RT_Mat4Identity();
	
	// render each mesh on the proper render mask
	RT_RaytraceMesh(g_level_resource, &mat, &mat, RT_RENDER_MASK_LEVELGEOMETRY);
	if(RT_GetRetraceRays())
		RT_RaytraceMesh(g_level_with_portals_resource, &mat, &mat, RT_RENDER_MASK_PORTALS);
}

void TraverseSegmentsForLights(short seg_num, uint8_t* visit_list, uint8_t* lights_added, int curr_rec_depth, RT_Vec3 curr_seg_entry_pos, float curr_segment_distance) {
	// Did we reach max recursion depth already? then we skip
	if (curr_rec_depth >= max_rec_depth)
		return;

	// Did we visit this segment already? then we skip it
	if (visit_list[seg_num] == 1)
		return;

	// Mark this segment as visited
	visit_list[seg_num] = 1;

	// For the current segment, go over all the sides
	segment* seg = &Segments[seg_num];
	for (size_t i = 0; i < MAX_SIDES_PER_SEGMENT; ++i) {
		// Assuming that RENDPAST means "render past this wall", if that is 0, we stop the traversal here
		const int wid = WALL_IS_DOORWAY(seg, i);
		if ((wid & WID_RENDPAST_FLAG) == 0)
			continue;

		// Get the segment number of this child segment
		const short seg_num_child = seg->children[i];

		// If it's -1 or -2, there is no segment on this side, skip it
		if (seg_num_child < 0)
			continue;

		// Upload all the lights in this segment
		for (int j = 0; j < m_light_count; ++j) {
			// Filter out lights that aren't in this segment
			if (m_lights_seg_ids[j] == -1)
				continue;
			if (m_lights_seg_ids[j] != seg_num_child) 
				continue;

			// Filter out lights that have already been added - this should fix the issue with lights being added twice
			if (lights_added[j] != 0)
				continue;

			// Filter out lights that are too far away from the camera - direct path
			const float distance_from_player = RT_Vec3Length(RT_Vec3Sub(RT_Vec3Fromvms_vector(&Viewer->pos), RT_TranslationFromMat34(m_lights[j].transform)));
			if (distance_from_player > max_distance)
				continue;

			// Filter out lights that are too far away from the camera - segment distance - this is broken, don't use it
			//const float distance_from_seg_entry_pos = RT_Vec3Length(RT_Vec3Sub(curr_seg_entry_pos, RT_TranslationFromMat34(m_lights[j].transform)));
			//if (distance_from_seg_entry_pos > max_seg_distance)
			//	continue;

			// Mark this light as added
			lights_added[j] = 1;

			// The lower the value, the more relevant the light is
			m_lights_relevance_score[m_lights_found] = (float)curr_rec_depth;
			m_lights_to_sort[m_lights_found] = j;
			++m_lights_found;
			
		}

		// Find the current segment's side's vertices
		RT_Vec3 verts[4];
		for (size_t j = 0; j < _countof(Side_to_verts_int[j]); ++j) {
			// Get one of the vertices of the side
			verts[j] = RT_Vec3Fromvms_vector(&Vertices[Segments[seg_num_child].verts[Side_to_verts_int[i][j]]]);
		}

		// Calculate center
		const RT_Vec3 tmp1 = RT_Vec3Add(verts[0], verts[1]);
		const RT_Vec3 tmp2 = RT_Vec3Add(verts[2], verts[3]);
		const RT_Vec3 center = RT_Vec3Add(tmp1, tmp2);

		// Find distance between segment entry
		const RT_Vec3 vector_from_entry_to_curr_segment = RT_Vec3Sub(center, curr_seg_entry_pos);
		const float distance_from_entry_to_curr_segment_squared = RT_Vec3Length(vector_from_entry_to_curr_segment);

		// Traverse all the children
		TraverseSegmentsForLights(seg_num_child, visit_list, lights_added, curr_rec_depth + 1, center, distance_from_entry_to_curr_segment_squared);
	}
}

void RT_UpdateLight(int index)
{
	RT_LightDefinition definition = g_light_definitions[m_lights_definitions[index]];
	RT_Light* light = &m_lights[index];
	light->kind = definition.kind;

	light->spot_angle    = RT_Uint8FromFloat(definition.spot_angle);
	light->spot_softness = RT_Uint8FromFloat(definition.spot_softness);

	light->emission = RT_PackRGBE(RT_Vec3Muls(definition.emission, g_light_multiplier));

	if(light->kind == RT_LightKind_Area_Sphere)
	{
		float r = definition.radius;
		RT_Vec3 position = RT_TranslationFromMat34(light->transform);

		RT_Mat34 transform = 
		{
			.e = 
			{
				r, 0, 0, position.x,
				0, r, 0, position.y,
				0, 0, r, position.z,
			}
		};
		light->transform = transform;
	}
	
}

void RT_FindAndSubmitNearbyLights(RT_Vec3 player_pos)
{
	int total = 0;

	for (int i = 0; i < m_light_count; i++)
	{
		if (g_pending_light_update)
		{
			RT_UpdateLight(i);
		}

		if (g_light_visual_debug)
		{
			RT_VisualizeLight(&m_lights[i]);
		}
	}

	if (light_culling_heuristic == 0){
		for (int i = 0; i < m_light_count; i++) 
		{
            const float distance = RT_Vec3Length(RT_Vec3Sub(RT_TranslationFromMat34(m_lights[i].transform), player_pos));
			if (distance < max_distance)
			{
				RT_RaytraceSubmitLight(m_lights[i]);
				total++;
			}
		}
	}

	// Segment based
	else if (light_culling_heuristic == 1) {
		const auto max_lights = RT_MAX_LIGHTS - RT_RaytraceGetCurrentLightCount(); // keep some room for dynamic lights
		m_lights_found = 0;
		uint8_t visit_list[MAX_SEGMENTS] = { 0 };
		uint8_t lights_added[_countof(m_lights)] = { 0 };

		// Find all the lights that the player has a direct path towards
		TraverseSegmentsForLights(Viewer->segnum, visit_list, lights_added, 0, RT_Vec3Fromvms_vector(&Viewer->pos), 0.0f);

		// If the number of lights exceeds the max number of lights, we need to pick the best ones
		if (m_lights_found > max_lights) {
			// Bubble sort them based on segment distance. We want the ones with the lowest number to appear first in the list
			for (int end = m_lights_found - 1; end > 0; --end) {
				for (int i = 0; i < end; ++i) {
					if (m_lights_relevance_score[i + 0] > m_lights_relevance_score[i + 1]) {
						// Swap the scores
						const short temp1 = m_lights_relevance_score[i + 0];
						m_lights_relevance_score[i + 0] = m_lights_relevance_score[i + 1];
						m_lights_relevance_score[i + 1] = temp1;

						// Swap the indices in the list
						const short temp2 = m_lights_to_sort[i + 0];
						m_lights_to_sort[i + 0] = m_lights_to_sort[i + 1];
						m_lights_to_sort[i + 1] = temp2;
					}
				}
			}

			// We only want to upload the best ones
			m_lights_found = max_lights;
		}

		total = m_lights_found;

		// Submit the lights
		for (int i = 0; i < m_lights_found; ++i) {
			RT_RaytraceSubmitLight(m_lights[m_lights_to_sort[i]]);
		}
	}

	g_pending_light_update = false;
	g_active_lights = total;
}