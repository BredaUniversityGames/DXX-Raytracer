#include <imgui.h>


#include "Renderer.h"
#include "Core/Arena.h"
#include "Core/MemoryScope.hpp"
#include "Core/Config.h"
#include "Core/String.h"
#include "RTmaterials.h"

extern "C"
{
	#include "piggy.h"
	#include "3d.h"
	#include "globvars.h"
};

#include "material_viewer.h"

struct RT_MaterialMeta
{
	float undo_flash;
	float redo_flash;
};

struct RT_MaterialUndoNode
{
	RT_MaterialUndoNode *next;
	uint64_t group_id;
	uint16_t material_index;
	RT_Material material;
};

struct RT_MaterialViewer
{
	RT_Arena arena;

	bool initialized;

	char texture_filter[256];

	int viewed_texture_slot;

	float textures_reloaded_timer;
	int   textures_reloaded_count;

	float materials_saved_timer;
	int   materials_saved_count;

	bool picker_open;
	bool highlight_blackbodies;
	bool highlight_no_casting_shadows;
	bool highlight_lights;
	bool highlight_fsr2_reactive_mask;
	bool highlight_has_normal_map;
	bool highlight_has_metalness_map;
	bool highlight_has_roughness_map;
	bool highlight_has_emissive_map;
	bool filter_on_highlighted;
	bool show_undo_redo_debug;

	bool editing;
	bool recenter_on_selection;
	bool suppress_hotkeys;

	float model_distance = 20.0f;
	float model_offset_x = 5.5f;
	float model_rotation_x;
	float model_rotation_dx;
	float model_rotation_y = 42.0f;
	float model_rotation_dy;
	float model_rotation_z;

	bool show_3d_preview = true;
	uint32_t currently_rendering_material_index;
	float render_next_material_timer;
	float render_next_material_speed = 2.0f;

	float spin_speed;
	float spin_offset;

	RT_MaterialMeta meta[RT_MAX_TEXTURES];

	uint16_t selected_material_count;
	uint16_t selected_materials[RT_MAX_TEXTURES];

	uint64_t next_undo_group_id;

	RT_Material materials_last_saved_states[RT_MAX_TEXTURES];
	RT_MaterialPaths materials_last_saved_paths[RT_MAX_TEXTURES];

	RT_Material undo_pre_edit_states[RT_MAX_TEXTURES];

	RT_MaterialUndoNode *first_free_undo_node;
	RT_MaterialUndoNode *first_undo;
	RT_MaterialUndoNode *first_redo;
};

static RT_MaterialViewer viewer;

typedef uint32_t RT_MaterialModifiedFlags;
enum RT_MaterialModifiedFlags_
{
	RT_MaterialModifiedFlags_AlbedoMap        = 1u << 0,
	RT_MaterialModifiedFlags_NormalMap        = 1u << 1,
	RT_MaterialModifiedFlags_MetalnessMap     = 1u << 2,
	RT_MaterialModifiedFlags_RoughnessMap     = 1u << 3,
	RT_MaterialModifiedFlags_EmissiveMap      = 1u << 4,

	RT_MaterialModifiedFlags_Metalness        = 1u << 5,
	RT_MaterialModifiedFlags_Roughness        = 1u << 6,
	RT_MaterialModifiedFlags_EmissiveColor    = 1u << 7,
	RT_MaterialModifiedFlags_EmissiveStrength = 1u << 8,
	RT_MaterialModifiedFlags_Flags            = 1u << 9,
};

static RT_MaterialModifiedFlags CompareMaterials(RT_Material *a, RT_Material *b)
{
	RT_MaterialModifiedFlags result = 0;
	result = RT_SET_FLAG(result, RT_MaterialModifiedFlags_Metalness, a->metalness != b->metalness);
	result = RT_SET_FLAG(result, RT_MaterialModifiedFlags_Roughness, a->roughness != b->roughness);
	result = RT_SET_FLAG(result, RT_MaterialModifiedFlags_EmissiveColor, !RT_Vec3AreEqual(a->emissive_color, b->emissive_color, 0.00001f));
	result = RT_SET_FLAG(result, RT_MaterialModifiedFlags_EmissiveStrength, a->emissive_strength != b->emissive_strength);
	result = RT_SET_FLAG(result, RT_MaterialModifiedFlags_Flags, a->flags != b->flags);
	return result;
}

static RT_MaterialModifiedFlags CompareMaterialPaths(RT_MaterialPaths *a, RT_MaterialPaths *b)
{
	RT_MaterialModifiedFlags result = 0;
	result = RT_SET_FLAG(result, RT_MaterialModifiedFlags_AlbedoMap, strncmp(a->albedo_texture, b->albedo_texture, sizeof(a->albedo_texture)));
	result = RT_SET_FLAG(result, RT_MaterialModifiedFlags_NormalMap, strncmp(a->normal_texture, b->normal_texture, sizeof(a->normal_texture)));
	result = RT_SET_FLAG(result, RT_MaterialModifiedFlags_MetalnessMap, strncmp(a->metalness_texture, b->metalness_texture, sizeof(a->metalness_texture)));
	result = RT_SET_FLAG(result, RT_MaterialModifiedFlags_RoughnessMap, strncmp(a->roughness_texture, b->roughness_texture, sizeof(a->roughness_texture)));
	result = RT_SET_FLAG(result, RT_MaterialModifiedFlags_EmissiveMap, strncmp(a->emissive_texture, b->emissive_texture, sizeof(a->emissive_texture)));
	return result;
}

static RT_MaterialModifiedFlags GetMaterialChangesFromLastSave(uint16_t bm_index)
{
	RT_MaterialModifiedFlags result = 0;

	RT_Material      *material         = &g_rt_materials                    [bm_index];
	RT_Material      *material_default = &viewer.materials_last_saved_states[bm_index];
	RT_MaterialPaths *paths            = &g_rt_material_paths               [bm_index];
	RT_MaterialPaths *paths_default    = &viewer.materials_last_saved_paths [bm_index];

	result |= CompareMaterials(material, material_default);
	result |= CompareMaterialPaths(paths, paths_default);

	return result;
}

static RT_MaterialModifiedFlags GetMaterialChangesFromDefaults(uint16_t bm_index)
{
	RT_MaterialModifiedFlags result = 0;

	RT_Material      *material         = &g_rt_materials             [bm_index];
	RT_Material      *material_default = &g_rt_materials_default     [bm_index];
	RT_MaterialPaths *paths            = &g_rt_material_paths        [bm_index];
	RT_MaterialPaths *paths_default    = &g_rt_material_paths_default[bm_index];

	result |= CompareMaterials(material, material_default);
	result |= CompareMaterialPaths(paths, paths_default);

	return result;
}

static void BeginUndoBatch()
{
	memcpy(viewer.undo_pre_edit_states, g_rt_materials, sizeof(g_rt_materials));
}

static void EndUndoBatch()
{
	while (viewer.first_redo)
	{
		RT_MaterialUndoNode *node = RT_SLL_POP(viewer.first_redo);
		RT_SLL_PUSH(viewer.first_free_undo_node, node);
	}

	uint64_t undo_group_id = viewer.next_undo_group_id++;
	for (uint16_t material_index = 0; material_index < MAX_BITMAP_FILES; material_index++)
	{
		RT_Material *material     = &g_rt_materials[material_index];
		RT_Material *pre_material = &viewer.undo_pre_edit_states[material_index];

		RT_MaterialModifiedFlags modified = CompareMaterials(material, pre_material);
		if (modified)
		{
			if (!viewer.first_free_undo_node)
			{
				viewer.first_free_undo_node = RT_ArenaAllocStructNoZero(&viewer.arena, RT_MaterialUndoNode);
				viewer.first_free_undo_node->next = NULL;
			}
			RT_MaterialUndoNode *node = RT_SLL_POP(viewer.first_free_undo_node);
			node->group_id       = undo_group_id;
			node->material_index = material_index;
			node->material       = *pre_material;
			RT_SLL_PUSH(viewer.first_undo, node);
		}
	}
}

static void SelectMaterial(uint16_t bm_index)
{
	size_t insert_index;
	for (insert_index = 0; insert_index < viewer.selected_material_count; insert_index++)
	{
		if (bm_index < viewer.selected_materials[insert_index])
		{
			break;
		}
	}

	memmove(&viewer.selected_materials[insert_index + 1], 
			&viewer.selected_materials[insert_index], 
			sizeof(uint16_t)*(viewer.selected_material_count - insert_index));

	viewer.selected_materials[insert_index] = bm_index;
	viewer.selected_material_count++;
}

static void DeselectMaterial(uint16_t bm_index)
{
	if (viewer.selected_material_count > 0)
	{
		size_t remove_index;
		for (remove_index = 0; remove_index < viewer.selected_material_count; remove_index++)
		{
			if (bm_index == viewer.selected_materials[remove_index])
			{
				break;
			}
		}

		if (remove_index != viewer.selected_material_count)
		{
			memmove(&viewer.selected_materials[remove_index],
					&viewer.selected_materials[remove_index + 1],
					sizeof(uint16_t)*(viewer.selected_material_count - (remove_index + 1)));
			viewer.selected_material_count--;
		}
	}
}

static bool MaterialIsSelected(uint16_t bm_index)
{
	bool is_selected = false;
	for (size_t i = 0; i < viewer.selected_material_count; i++)
	{
		if (viewer.selected_materials[i] == bm_index)
		{
			is_selected = true;
			break;
		}
	}

	return is_selected;
}

static void ToggleSelectMaterial(uint16_t bm_index)
{
	if (MaterialIsSelected(bm_index))
	{
		DeselectMaterial(bm_index);
	}
	else
	{
		SelectMaterial(bm_index);
	}
}

static void SelectAll()
{
	viewer.selected_material_count = 0;

	for (uint16_t bm_index = 0; bm_index < MAX_BITMAP_FILES; bm_index++)
	{
		grs_bitmap *bitmap = &GameBitmaps[bm_index];

		if (!bitmap->bm_w ||
			!bitmap->bm_h)
		{
			continue;
		}

		viewer.selected_materials[viewer.selected_material_count++] = bm_index;
	}
}

static void SelectNone()
{
	viewer.selected_material_count = 0;
}

static void ResetDefaultValues()
{
	BeginUndoBatch();

	for (size_t selection_index = 0; selection_index < viewer.selected_material_count; selection_index++)
	{
		uint16_t bm_index = viewer.selected_materials[selection_index];
		RT_Material *material         = &g_rt_materials[bm_index];
		RT_Material *material_default = &viewer.materials_last_saved_states[bm_index];

		// NOTE(daniel): I wouldn't exactly call this the right way to copy over these values, because it
		// isn't robust in the face of changes to the RT_Material struct. However, I don't have a bundle
		// struct right now for them separate from the texture handles which I don't want to copy over
		// for now, because I don't know what we're doing with deleting textures when reloaded or whatever
		material->metalness         = material_default->metalness;
		material->roughness         = material_default->roughness;
		material->emissive_color    = material_default->emissive_color;
		material->emissive_strength = material_default->emissive_strength;
		material->flags             = material_default->flags;
		RT_UpdateMaterial(bm_index, material);

		g_rt_material_paths[bm_index] = g_rt_material_paths_default[bm_index];
	}

	EndUndoBatch();
}

static void DoUndo()
{
	if (viewer.first_undo)
	{
		uint64_t group_id = viewer.first_undo->group_id;

		viewer.selected_material_count = 0;

		while (viewer.first_undo && viewer.first_undo->group_id == group_id)
		{
			RT_MaterialUndoNode *node = RT_SLL_POP(viewer.first_undo);
			RT_Material redo_definition = g_rt_materials[node->material_index];

			viewer.meta[node->material_index].undo_flash = 0.5f;

			g_rt_materials[node->material_index] = node->material;
			RT_UpdateMaterial(node->material_index, &g_rt_materials[node->material_index]);
			SelectMaterial(node->material_index);

			node->material = redo_definition;
			RT_SLL_PUSH(viewer.first_redo, node);
		}

		viewer.recenter_on_selection = true;
	}
}

static void DoRedo()
{
	if (viewer.first_redo)
	{
		uint64_t group_id = viewer.first_redo->group_id;

		viewer.selected_material_count = 0;

		while (viewer.first_redo && viewer.first_redo->group_id == group_id)
		{
			RT_MaterialUndoNode *node = RT_SLL_POP(viewer.first_redo);
			RT_Material undo_definition = g_rt_materials[node->material_index];

			viewer.meta[node->material_index].redo_flash = 0.5f;

			g_rt_materials[node->material_index] = node->material;
			RT_UpdateMaterial(node->material_index, &g_rt_materials[node->material_index]);
			SelectMaterial(node->material_index);

			node->material = undo_definition;
			RT_SLL_PUSH(viewer.first_undo, node);
		}

		viewer.recenter_on_selection = true;
	}
}

static void SaveIfModified(uint16_t bm_index)
{
	grs_bitmap *bitmap = &GameBitmaps[bm_index];

	if (bitmap->bm_w == 0 ||
		bitmap->bm_h == 0)
	{
		return;
	}

	RT_Material      *material = &g_rt_materials     [bm_index];
	RT_MaterialPaths *paths    = &g_rt_material_paths[bm_index];

	RT::MemoryScope temp;

	RT_MaterialModifiedFlags needs_to_be_saved = GetMaterialChangesFromLastSave(bm_index); // this check is only if the material differs at all from the last save, we don't care about the specifics

	if (needs_to_be_saved)
	{
		RT_MaterialModifiedFlags changes = GetMaterialChangesFromDefaults(bm_index); // anything that differs from the defaults needs to be saved

		RT_Config cfg;
		RT_InitializeConfig(&cfg, temp);

		if (changes & RT_MaterialModifiedFlags_AlbedoMap)
		{
			RT_ConfigWriteString(&cfg, RT_StringLiteral("albedo_texture"), RT_StringFromCString(paths->albedo_texture));
		}

		if (changes & RT_MaterialModifiedFlags_NormalMap)
		{
			RT_ConfigWriteString(&cfg, RT_StringLiteral("normal_texture"), RT_StringFromCString(paths->normal_texture));
		}

		if (changes & RT_MaterialModifiedFlags_MetalnessMap)
		{
			RT_ConfigWriteString(&cfg, RT_StringLiteral("metalness_texture"), RT_StringFromCString(paths->metalness_texture));
		}

		if (changes & RT_MaterialModifiedFlags_RoughnessMap)
		{
			RT_ConfigWriteString(&cfg, RT_StringLiteral("roughness_texture"), RT_StringFromCString(paths->roughness_texture));
		}

		if (changes & RT_MaterialModifiedFlags_EmissiveMap)
		{
			RT_ConfigWriteString(&cfg, RT_StringLiteral("emissive_texture"), RT_StringFromCString(paths->emissive_texture));
		}

		if (changes & RT_MaterialModifiedFlags_Metalness)
		{
			RT_ConfigWriteFloat(&cfg, RT_StringLiteral("metalness"), material->metalness);
		}

		if (changes & RT_MaterialModifiedFlags_Roughness)
		{
			RT_ConfigWriteFloat(&cfg, RT_StringLiteral("roughness"), material->roughness);
		}

		if (changes & RT_MaterialModifiedFlags_EmissiveColor)
		{
			RT_ConfigWriteVec3(&cfg, RT_StringLiteral("emissive_color"), material->emissive_color);
		}

		if (changes & RT_MaterialModifiedFlags_EmissiveStrength)
		{
			RT_ConfigWriteFloat(&cfg, RT_StringLiteral("emissive_strength"), material->emissive_strength);
		}

		if (changes & RT_MaterialModifiedFlags_Flags)
		{
			RT_ConfigWriteInt(&cfg, RT_StringLiteral("blackbody"), !!(material->flags & RT_MaterialFlag_BlackbodyRadiator));
			RT_ConfigWriteInt(&cfg, RT_StringLiteral("no_casting_shadow"), !!(material->flags & RT_MaterialFlag_NoCastingShadow));
			RT_ConfigWriteInt(&cfg, RT_StringLiteral("is_light"), !!(material->flags & RT_MaterialFlag_Light));
			RT_ConfigWriteInt(&cfg, RT_StringLiteral("fsr2_reactive_mask"), !!(material->flags & RT_MaterialFlag_Fsr2ReactiveMask));
		}

		char bitmap_name[13];
		piggy_get_bitmap_name(bm_index, bitmap_name);

		// NOTE(daniel): Substance Designer does not like hashtags in names and
		// turns them into underscores.
		for (char *c = bitmap_name; *c; c++)
		{
			if (*c == '#')
			{
				*c = '_';
			}
		}

		char *out_file = RT_ArenaPrintF(temp, "assets/textures/%s.material", bitmap_name);
		if (RT_SerializeConfigToFile(&cfg, out_file))
		{
			// Update last saved states so we know when the material has changed, again.
			viewer.materials_last_saved_states[bm_index] = *material;
			viewer.materials_last_saved_paths [bm_index] = *paths;

			viewer.materials_saved_timer = 2.0f;
			viewer.materials_saved_count++;
		}
	}
}

static void SaveSelected()
{
	for (size_t i = 0; i < viewer.selected_material_count; i++)
	{
		SaveIfModified(viewer.selected_materials[i]);
	}
}

static void SaveAll()
{
	for (uint16_t bm_index = 0; bm_index < MAX_BITMAP_FILES; bm_index++)
	{
		SaveIfModified(bm_index);
	}
}

void RT_DoMaterialViewerMenus()
{
	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();

	RT::MemoryScope temp;

	float dt = 1.0f / 60.0f; // hardcoded because developer tool laziness

	if (!viewer.initialized)
	{
		memcpy(viewer.materials_last_saved_states, g_rt_materials, sizeof(g_rt_materials));
		memcpy(viewer.materials_last_saved_paths, g_rt_material_paths, sizeof(g_rt_material_paths));
		viewer.initialized = true;
	}

	if (io.WantCaptureKeyboard && !viewer.suppress_hotkeys)
	{
		if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_A))
		{
			SelectNone();
		}
		else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A))
		{
			SelectAll();
		}

		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z))
		{
			DoUndo();
		}

		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y))
		{
			DoRedo();
		}

		if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_R))
		{
			ResetDefaultValues();
		} 
		else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_R))
		{
			viewer.textures_reloaded_count = RT_ReloadMaterials();
			viewer.textures_reloaded_timer = 2.0f;
		}

		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
		{
			SaveSelected();
		}

		if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_S))
		{
			SaveAll();
		}
	}

	if (ImGui::Begin("Material Editor", nullptr, ImGuiWindowFlags_MenuBar|ImGuiWindowFlags_AlwaysVerticalScrollbar))
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Reload Textures", "Ctrl + R"))
				{
					viewer.textures_reloaded_count = RT_ReloadMaterials();
					viewer.textures_reloaded_timer = 2.0f;
				} 

				if (ImGui::MenuItem("Save Selected", "Ctrl + S"))
				{
					SaveSelected();
				}

				if (ImGui::MenuItem("Save All", "Ctrl + Shift + S"))
				{
					SaveAll();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Select All", "Ctrl + A"))
				{
					SelectAll();
				}

				if (ImGui::MenuItem("Clear Selection", "Ctrl + Shift + A"))
				{
					SelectNone();
				}

				if (ImGui::MenuItem("Undo", "Ctrl + Z", false, viewer.first_undo))
				{
					DoUndo();
				}

				if (ImGui::MenuItem("Redo", "Ctrl + Y", false, viewer.first_redo))
				{
					DoRedo();
				}

				if (ImGui::MenuItem("Discard Changes", "Ctrl + Shift + R", false, viewer.selected_material_count > 0))
				{
					ResetDefaultValues();
				}

				ImGui::MenuItem("Show Undo/Redo Debug", NULL, &viewer.show_undo_redo_debug);

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Highlight"))
			{
				ImGui::Checkbox("Filter on Highlighted", &viewer.filter_on_highlighted);
				ImGui::Checkbox("Highlight: Is Blackbody", &viewer.highlight_blackbodies);
				ImGui::Checkbox("Highlight: Not casting shadows", &viewer.highlight_no_casting_shadows);
				ImGui::Checkbox("Highlight: Is Light", &viewer.highlight_lights);
				ImGui::Checkbox("Highlight: FSR2 reactive mask", &viewer.highlight_fsr2_reactive_mask);
				ImGui::Checkbox("Highlight: Has Normal Map", &viewer.highlight_has_normal_map);
				ImGui::Checkbox("Highlight: Has Metalness Map", &viewer.highlight_has_metalness_map);
				ImGui::Checkbox("Highlight: Has Roughness Map", &viewer.highlight_has_roughness_map);
				ImGui::Checkbox("Highlight: Has Emissive Map", &viewer.highlight_has_emissive_map);
				ImGui::EndMenu();
			}

			if (viewer.textures_reloaded_timer > 0.0f)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.85f, 1.0f));
				ImGui::Text("Reloaded %d Textures", viewer.textures_reloaded_count);
				ImGui::PopStyleColor();
				viewer.textures_reloaded_timer -= dt;
			}

			if (viewer.materials_saved_timer > 0.0f)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 1.0f, 1.0f));
				ImGui::Text("Saved %d Materials", viewer.materials_saved_count);
				ImGui::PopStyleColor();
				viewer.materials_saved_timer -= dt;

				if (viewer.materials_saved_timer <= 0.0f)
				{
					viewer.materials_saved_count = 0;
				}
			}

			ImGui::EndMenuBar();
		} 

		ImGui::BeginChild("Material Viewer", ImVec2(ImGui::GetContentRegionAvail().x, 260), true);
		{
			ImDrawList *draw_list = ImGui::GetWindowDrawList();

			float max_offset_x = ImGui::GetContentRegionAvail().x - 64;
			float offset_x = 0;
			float recenter_y = FLT_MAX;

			bool first_drawn_texture = true;

			for (uint16_t bm_index = 0; bm_index < MAX_BITMAP_FILES; bm_index++)
			{
				grs_bitmap *bitmap = &GameBitmaps[bm_index];

				if (bitmap->bm_w == 0 ||
					bitmap->bm_h == 0)
				{
					continue;
				}

				char bitmap_name[13];
				piggy_get_bitmap_name(bm_index, bitmap_name);

				RT_Material *material = &g_rt_materials[bm_index];

				bool rejected_by_filter = false;

				bool is_blackbody = (material->flags & RT_MaterialFlag_BlackbodyRadiator);
				bool is_no_casting_shadow = (material->flags & RT_MaterialFlag_NoCastingShadow);
				bool is_light = (material->flags & RT_MaterialFlag_Light);
				bool is_fsr2_reactive_mask = (material->flags & RT_MaterialFlag_Fsr2ReactiveMask);
				bool has_normal = RT_RESOURCE_HANDLE_VALID(material->normal_texture);
				bool has_metalness = RT_RESOURCE_HANDLE_VALID(material->metalness_texture);
				bool has_roughness = RT_RESOURCE_HANDLE_VALID(material->roughness_texture);
				bool has_emissive = RT_RESOURCE_HANDLE_VALID(material->emissive_texture);

				if (viewer.filter_on_highlighted)
				{
					if (viewer.highlight_blackbodies && !is_blackbody) rejected_by_filter = true;
					if (viewer.highlight_no_casting_shadows && !is_no_casting_shadow) rejected_by_filter = true;
					if (viewer.highlight_fsr2_reactive_mask && !is_fsr2_reactive_mask) rejected_by_filter = true;
					if (viewer.highlight_lights && !is_light) rejected_by_filter = true;
					if (viewer.highlight_has_normal_map && !has_normal) rejected_by_filter = true;
					if (viewer.highlight_has_metalness_map && !has_metalness) rejected_by_filter = true;
					if (viewer.highlight_has_roughness_map && !has_roughness) rejected_by_filter = true;
					if (viewer.highlight_has_emissive_map && !has_emissive) rejected_by_filter = true;
				}

				if (viewer.texture_filter[0])
				{
					if (viewer.texture_filter[0] == '#')
					{
						char *number_start = &viewer.texture_filter[1];

						uint32_t number = strtoul(number_start, NULL, 10);

						if (number > 0 && number < MAX_BITMAP_FILES)
						{
							if (bm_index != number)
								rejected_by_filter = true;
						}
					}
					else
					{
						if (!strstr(bitmap_name, viewer.texture_filter))
							rejected_by_filter = true;
					}
				}

				if (rejected_by_filter)
					continue;

				offset_x += 64 + style.ItemSpacing.x;

				if (!first_drawn_texture && offset_x <= max_offset_x)
				{
					ImGui::SameLine();
				}
				else
				{
					offset_x = 0;
				}

				ImVec2 image_cursor_pos = ImGui::GetCursorPos();

				float width = 64.0f;
				float height = 64.0f;
				if (bitmap->bm_w > bitmap->bm_h)
				{
					float aspect = (float)bitmap->bm_h / (float)bitmap->bm_w;
					height *= aspect;
				}
				else
				{
					float aspect = (float)bitmap->bm_w / (float)bitmap->bm_h;
					height *= aspect;
				}

				RT_RenderImGuiTexture(material->albedo_texture, 64, height);
				ImGui::SetCursorPos(image_cursor_pos);

				if (RT_RESOURCE_HANDLE_VALID(material->emissive_texture))
				{
					RT_RenderImGuiTexture(material->emissive_texture, 64, height);
					ImGui::SetCursorPos(image_cursor_pos);
				}

				if (ImGui::InvisibleButton(RT_ArenaPrintF(temp, "material_button_%d", bm_index), ImVec2(64, 64)))
				{
					if (io.KeyCtrl)
					{
						ToggleSelectMaterial(bm_index);
					}
					else if (io.KeyShift)
					{
						SelectMaterial(bm_index);

						uint16_t min_bm_index = viewer.selected_materials[0];
						uint16_t max_bm_index = viewer.selected_materials[viewer.selected_material_count - 1];

						viewer.selected_material_count = 0;
						for (uint16_t i = min_bm_index;
							 i <= max_bm_index;
							 i++)
						{
							viewer.selected_materials[viewer.selected_material_count++] = i;
						}
					}
					else
					{
						if (viewer.selected_material_count > 1)
						{
							viewer.selected_material_count = 0;
							SelectMaterial(bm_index);
						}
						else
						{
							if (MaterialIsSelected(bm_index))
							{
								DeselectMaterial(bm_index);
							}
							else
							{
								viewer.selected_material_count = 0;
								SelectMaterial(bm_index);
							}
						}
					}
				}

				if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
				{
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
					ImGui::Text("%s (#%hu)", bitmap_name, bm_index);
					ImGui::PopTextWrapPos();
					ImGui::EndTooltip();
				}

				bool is_selected = false;
				for (size_t selected_index = 0;
					 selected_index < viewer.selected_material_count;
					 selected_index++)
				{
					if (viewer.selected_materials[selected_index] == bm_index)
					{
						is_selected = true;
						break;
					}
				}

				ImVec2 img_min = ImGui::GetItemRectMin();
				ImVec2 img_max = ImGui::GetItemRectMax();

				if (is_selected)
				{
					draw_list->AddRect(img_min, img_max, ImColor(1.0f, 0.5f, 1.0f, 1.0f));
				}

				if (viewer.highlight_blackbodies && is_blackbody)
				{
					draw_list->AddRect(img_min, img_max, ImColor(0.5f, 1.0f, 1.0f, 0.25f));
				}

				if (viewer.highlight_no_casting_shadows && is_no_casting_shadow)
				{
					draw_list->AddRect(img_min, img_max, ImColor(0.5f, 1.0f, 1.0f, 0.25f));
				}

				if (viewer.highlight_lights && is_light)
				{
					draw_list->AddRect(img_min, img_max, ImColor(1.0f, 1.0f, 0.0f, 0.25f));
				}

				if (viewer.highlight_fsr2_reactive_mask && is_fsr2_reactive_mask)
				{
					draw_list->AddRect(img_min, img_max, ImColor(1.0f, 1.0f, 0.0f, 0.25f));
				}

				if (viewer.highlight_has_normal_map && has_normal)
				{
					draw_list->AddRect(img_min, img_max, ImColor(0.5f, 0.5f, 1.0f, 0.25f));
				}

				if (viewer.highlight_has_metalness_map && has_metalness)
				{
					draw_list->AddRect(img_min, img_max, ImColor(1.0f, 0.5f, 0.5f, 0.25f));
				}

				if (viewer.highlight_has_roughness_map && has_roughness)
				{
					draw_list->AddRect(img_min, img_max, ImColor(0.5f, 1.0f, 0.5f, 0.25f));
				}

				if (viewer.highlight_has_emissive_map && has_emissive)
				{
					draw_list->AddRect(img_min, img_max, ImColor(1.0f, 1.0f, 0.5f, 0.25f));
				}

				RT_MaterialMeta *meta = &viewer.meta[bm_index];

				if (meta->undo_flash > 0.0f)
				{
					draw_list->AddRect(img_min, img_max, ImColor(0.0f, 1.0f, 1.0f, meta->undo_flash));
					meta->undo_flash -= dt;
				}

				if (meta->redo_flash > 0.0f)
				{
					draw_list->AddRect(img_min, img_max, ImColor(1.0f, 1.0f, 0.0f, meta->redo_flash));
					meta->redo_flash -= dt;
				}

				if (viewer.recenter_on_selection && is_selected)
				{
					float y = ImGui::GetCursorPosY();
					if (recenter_y > y)
						recenter_y = y;
				}

				first_drawn_texture = false;
			}

			if (viewer.recenter_on_selection && viewer.selected_material_count > 0)
			{
				ImGui::SetScrollFromPosY(ImGui::GetCursorStartPos().y + recenter_y);
				viewer.recenter_on_selection = false;
			}
		} ImGui::EndChild();

		viewer.recenter_on_selection |= ImGui::InputText("Filter Textures", viewer.texture_filter, sizeof(viewer.texture_filter));
		viewer.suppress_hotkeys = ImGui::IsItemActive();
		
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted("Type #[number] where number is some index to find a material with a specific index.");
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
		if (ImGui::Button("Recenter Selection"))
		{
			viewer.recenter_on_selection = true;
		}
		ImGui::Separator();

		if (viewer.selected_material_count == 1)
		{
			char bitmap_name[13] = "Invalid";
			piggy_get_bitmap_name(viewer.selected_materials[0], bitmap_name);

			ImGui::Text("Bitmap #%hu: %s", viewer.selected_materials[0], bitmap_name);
		}
		else if (viewer.selected_material_count > 1)
		{
			uint16_t first_bitmap_index = viewer.selected_materials[0];

			char first_bitmap[13] = "Invalid";
			piggy_get_bitmap_name(first_bitmap_index, first_bitmap);

			uint16_t last_bitmap_index = viewer.selected_materials[viewer.selected_material_count - 1];

			char last_bitmap[13] = "Invalid";
			piggy_get_bitmap_name(last_bitmap_index, last_bitmap);

			ImGui::Text("Bitmap #%hu..#%hu: %s .. %s", first_bitmap_index, last_bitmap_index, first_bitmap, last_bitmap);
		}
		else
		{
			ImGui::Text("Bitmap: None");
		}


		bool flags_equal = true;
		bool metalness_equal = true;
		bool roughness_equal = true;
		bool emissive_color_equal = true;
		bool emissive_strength_equal = true;

		// NOTE(daniel): There must be a better way to write this code, but I
		// am going for dumb code duplication for simplicity.
		bool active = false;

		if (viewer.selected_material_count > 0)
		{
			RT_Material *first_material = &g_rt_materials[viewer.selected_materials[0]];

			for (size_t selection_index = 1; 
				 selection_index < viewer.selected_material_count;
				 selection_index++)
			{
				uint16_t material_index = viewer.selected_materials[selection_index];
				RT_Material *material = &g_rt_materials[material_index];

				if (material->flags != first_material->flags) flags_equal = false;
				if (material->metalness != first_material->metalness) metalness_equal = false;
				if (material->roughness != first_material->roughness) roughness_equal = false;
				if (!RT_Vec3AreEqual(material->emissive_color, first_material->emissive_color, 0.0f)) emissive_color_equal = false;
				if (material->emissive_strength != first_material->emissive_strength) emissive_strength_equal = false;
			}

			RT_MaterialModifiedFlags modified = GetMaterialChangesFromLastSave(viewer.selected_materials[0]);

			// ------------------------------------------------------------------
			// -Editable Controls------------------------------------------------
			// ------------------------------------------------------------------

			ImVec4 modified_color           = ImVec4(0.85f, 1.0f, 0.5f, 1.0f);
			ImVec4 multiple_selection_color = ImVec4(1.0f, 0.25f, 0.35f, 1.0f);

			if (modified & RT_MaterialModifiedFlags_Flags)
				ImGui::PushStyleColor(ImGuiCol_Text, modified_color);

			if (!flags_equal)
				ImGui::PushStyleColor(ImGuiCol_Text, multiple_selection_color);

			bool blackbody = first_material->flags & RT_MaterialFlag_BlackbodyRadiator;
			bool blackbody_changed = ImGui::Checkbox("Blackbody Radiator", &blackbody);
			active |= ImGui::IsItemActive();
			ImGui::SameLine();
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
			{
				ImGui::BeginTooltip();
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
				ImGui::TextUnformatted("A Blackbody Radiator is a material that is entirely emissive and reflects no light that hits it. Selecting this option makes the renderer treat the base color as the emissive color and disables any kind of shading for the material.");
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}

			bool no_casting_shadow = first_material->flags & RT_MaterialFlag_NoCastingShadow;
			bool no_casting_shadow_changed = ImGui::Checkbox("No casting shadow", &no_casting_shadow);
			active |= ImGui::IsItemActive();
			ImGui::SameLine();
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
			{
				ImGui::BeginTooltip();
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
				ImGui::TextUnformatted("Determines if the mesh using this material should cast a shadow.");
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}

			bool is_light = first_material->flags & RT_MaterialFlag_Light;
			bool is_light_changed = ImGui::Checkbox("Is Light", &is_light);
			active |= ImGui::IsItemActive();
			ImGui::SameLine();
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
			{
				ImGui::BeginTooltip();
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
				ImGui::TextUnformatted("Whether this material is represented in the scene as an actual area light.");
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}

			bool is_fsr2_reactive_mask = first_material->flags & RT_MaterialFlag_Fsr2ReactiveMask;
			bool is_fsr2_reactive_changed = ImGui::Checkbox("FSR2 Reactive Mask", &is_fsr2_reactive_mask);
			active |= ImGui::IsItemActive();
			ImGui::SameLine();
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
			{
				ImGui::BeginTooltip();
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
				ImGui::TextUnformatted("Whether this material should write to the FSR2 reactive mask.");
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}

			if (!flags_equal)
				ImGui::PopStyleColor();

			if (modified & RT_MaterialModifiedFlags_Flags)
				ImGui::PopStyleColor();

			// ------------------------------------------------------------------

			if (modified & RT_MaterialModifiedFlags_Metalness)
				ImGui::PushStyleColor(ImGuiCol_Text, modified_color);

			if (!metalness_equal)
				ImGui::PushStyleColor(ImGuiCol_Text, multiple_selection_color);

			float metalness = first_material->metalness;
			bool metalness_changed = ImGui::SliderFloat("Metalness Factor", &metalness, 0.0f, 1.0f);
			active |= ImGui::IsItemActive();

			if (!metalness_equal)
				ImGui::PopStyleColor();

			if (modified & RT_MaterialModifiedFlags_Metalness)
				ImGui::PopStyleColor();

			// ------------------------------------------------------------------

			if (modified & RT_MaterialModifiedFlags_Roughness)
				ImGui::PushStyleColor(ImGuiCol_Text, modified_color);

			if (!roughness_equal)
				ImGui::PushStyleColor(ImGuiCol_Text, multiple_selection_color);

			float roughness = first_material->roughness;
			bool roughness_changed = ImGui::SliderFloat("Roughness Factor", &roughness, 0.0f, 1.0f);
			active |= ImGui::IsItemActive();

			if (!roughness_equal)
				ImGui::PopStyleColor();

			if (modified & RT_MaterialModifiedFlags_Roughness)
				ImGui::PopStyleColor();

			// ------------------------------------------------------------------

			if (modified & RT_MaterialModifiedFlags_EmissiveStrength)
				ImGui::PushStyleColor(ImGuiCol_Text, modified_color);

			if (!emissive_strength_equal)
				ImGui::PushStyleColor(ImGuiCol_Text, multiple_selection_color);

			ImGui::SetNextItemWidth(0.33f*ImGui::GetContentRegionMax().x);

			float emissive_strength = first_material->emissive_strength;
			bool emissive_changed = ImGui::DragFloat("Emissive Strength", &emissive_strength, 0.1f, 0.0f, 1000.0f); 
			active |= ImGui::IsItemActive();

			if (!emissive_strength_equal)
				ImGui::PopStyleColor();

			if (modified & RT_MaterialModifiedFlags_EmissiveStrength)
				ImGui::PopStyleColor();

			// ------------------------------------------------------------------

			if (modified & RT_MaterialModifiedFlags_EmissiveColor)
				ImGui::PushStyleColor(ImGuiCol_Text, modified_color);

			if (!emissive_color_equal)
				ImGui::PushStyleColor(ImGuiCol_Text, multiple_selection_color);

			RT_Vec3 emissive_color = first_material->emissive_color;

			ImGui::SameLine();
			ImGui::SetNextItemWidth(0.33f*ImGui::GetContentRegionMax().x);
			emissive_changed |= ImGui::ColorEdit3("Color", &emissive_color.x);
			active |= ImGui::IsItemActive();

			if (!emissive_color_equal)
				ImGui::PopStyleColor();

			if (modified & RT_MaterialModifiedFlags_EmissiveColor)
				ImGui::PopStyleColor();

			// ------------------------------------------------------------------
			// ------------------------------------------------------------------

			if (ImGui::BeginCombo("Texture Slot", g_rt_texture_slot_names[viewer.viewed_texture_slot]))
			{
				for (int i = 0; i < RT_MaterialTextureSlot_COUNT; i++)
				{
					bool is_selected = (viewer.viewed_texture_slot == i);
					ImGuiSelectableFlags flags = 0;
					if (viewer.selected_material_count == 1)
					{
						if (!RT_RESOURCE_HANDLE_VALID(first_material->textures[i]))
						{
							flags |= ImGuiSelectableFlags_Disabled;
						}
					}
					char *name = g_rt_texture_slot_names[i];
					if (viewer.selected_material_count == 1)
					{
						RT_MaterialPaths *first_paths = &g_rt_material_paths[viewer.selected_materials[0]];
						name = RT_ArenaPrintF(temp, "%s (%s)", g_rt_texture_slot_names[i], first_paths->textures[i]);
					}
					if (ImGui::Selectable(name, is_selected, flags))
					{
						viewer.viewed_texture_slot = i;
					}
					if (is_selected)
					{
						ImGui::SetItemDefaultFocus(); // What does this do? Took it from the imgui demo.
					}
				}
				ImGui::EndCombo();
			}

			if (viewer.selected_material_count == 1)
			{
				RT_ResourceHandle texture = first_material->textures[viewer.viewed_texture_slot];
				RT_RenderImGuiTexture(texture, 256, 256);
			}
			else if (viewer.selected_material_count > 1)
			{
				ImGui::TextDisabled("Multiple Materials Selected");
			}
			else
			{
				ImGui::TextDisabled("No Materials Selected");
			}

			if (blackbody_changed ||
				no_casting_shadow_changed ||
				is_light_changed ||
				is_fsr2_reactive_changed ||
				metalness_changed ||
				roughness_changed ||
				emissive_changed)
			{
				// ------------------------------------------------------------------
				// Apply changes

				if (!viewer.editing)
				{
					BeginUndoBatch();
				}

				viewer.editing = true;

				for (size_t selection_index = 0;
					 selection_index < viewer.selected_material_count;
					 selection_index++)
				{
					uint16_t material_index = viewer.selected_materials[selection_index];

					RT_Material *material = &g_rt_materials[material_index];

					if (blackbody_changed)
						material->flags = RT_SET_FLAG(material->flags, RT_MaterialFlag_BlackbodyRadiator, blackbody);

					if (no_casting_shadow_changed)
						material->flags = RT_SET_FLAG(material->flags, RT_MaterialFlag_NoCastingShadow, no_casting_shadow);

					if (is_light_changed)
						material->flags = RT_SET_FLAG(material->flags, RT_MaterialFlag_Light, is_light);

					if (is_fsr2_reactive_changed)
						material->flags = RT_SET_FLAG(material->flags, RT_MaterialFlag_Fsr2ReactiveMask, is_fsr2_reactive_mask);

					if (metalness_changed)
					{
						material->metalness = metalness;
					}

					if (roughness_changed)
					{
						material->roughness = roughness;
					}

					if (emissive_changed)
					{
						material->emissive_strength = emissive_strength;
						material->emissive_color = emissive_color;
					}

					RT_UpdateMaterial(material_index, material);
				}
			}
			else
			{
				if (viewer.editing)
				{
					if (!active)
					{
						EndUndoBatch();
						viewer.editing = false;
					}
				}
			}
		}

		ImGui::Separator();
		ImGui::Text("3D Preview");
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted("Hold LMB to rotate the plane, RMB to move it left/right and forward/back.");
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
		ImGui::Checkbox("Show 3D Preview", &viewer.show_3d_preview);
		ImGui::SliderFloat("Animation Speed", &viewer.render_next_material_speed, 1.0f, 60.0f);

		if (viewer.show_undo_redo_debug)
		{
			ImGui::Separator();
			ImGui::Text("Undo/Redo Debug");

			ImGui::Text("Editing: %d", viewer.editing);
			ImGui::Text("Active:  %d", active);
			ImGui::Text("");

			int free_node_count = 0;
			for (RT_MaterialUndoNode *node = viewer.first_free_undo_node; node; node = node->next)
			{
				free_node_count++;
			}

			ImGui::Text("Free nodes: %d", free_node_count);

			ImGui::BeginChild("Undo Stack", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 260.0f), false);
			ImGui::Text("Undo");

			int node_index = 0;
			for (RT_MaterialUndoNode *node = viewer.first_undo; node; node = node->next)
			{
				if (ImGui::TreeNode(node, RT_ArenaPrintF(temp, "Node %d", node_index)))
				{
					ImGui::Text("Group: %llu", node->group_id);
					ImGui::Text("Material Index: %hu", node->material_index);
					ImGui::Text("Flags: %u", node->material.flags);
					ImGui::Text("Metalness: %f", node->material.metalness);
					ImGui::Text("Roughness: %f", node->material.roughness);
					ImGui::Text("Emissive Color: %f, %f, %f", 
								node->material.emissive_color.x, 
								node->material.emissive_color.y, 
								node->material.emissive_color.z);
					ImGui::Text("Emissive Strength: %f", node->material.emissive_strength);

					ImGui::TreePop();
				}

				node_index++;
			}

			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("Redo Stack", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 260.0f), false);
			ImGui::Text("Redo");

			node_index = 0;
			for (RT_MaterialUndoNode *node = viewer.first_redo; node; node = node->next)
			{
				if (ImGui::TreeNode(node, RT_ArenaPrintF(temp, "Node %d", node_index)))
				{
					ImGui::Text("Group: %llu", node->group_id);
					ImGui::Text("Material Index: %hu", node->material_index);
					ImGui::Text("Flags: %u", node->material.flags);
					ImGui::Text("Metalness: %f", node->material.metalness);
					ImGui::Text("Roughness: %f", node->material.roughness);
					ImGui::Text("Emissive Color: %f, %f, %f", 
								node->material.emissive_color.x, 
								node->material.emissive_color.y, 
								node->material.emissive_color.z);
					ImGui::Text("Emissive Strength: %f", node->material.emissive_strength);

					ImGui::TreePop();
				}

				node_index++;
			}

			ImGui::EndChild();
		}
	} ImGui::End();
}

void RT_RenderMaterialViewer()
{
	float dt = 1.0f / 60.0f;

	if (viewer.show_3d_preview && viewer.selected_material_count > 0)
	{
		int current_index = viewer.currently_rendering_material_index;

		if (viewer.render_next_material_timer <= 0.0f)
		{
			float time_per_material = 1.0f / viewer.render_next_material_speed;
			viewer.render_next_material_timer = time_per_material;
		}
		else
		{
			viewer.render_next_material_timer -= dt;

			if (viewer.render_next_material_timer <= 0.0f)
			{
				viewer.currently_rendering_material_index++;
				if (viewer.currently_rendering_material_index >= viewer.selected_material_count)
				{
					viewer.currently_rendering_material_index = 0;
				}
			}
		}

		grs_bitmap *bitmap = &GameBitmaps[viewer.selected_materials[current_index]];

		float width  = 5.0f;
		float height = 5.0f;
		if (bitmap->bm_w > bitmap->bm_h)
		{
			float aspect = (float)bitmap->bm_h / (float)bitmap->bm_w;
			height *= aspect;
		}
		else
		{
			float aspect = (float)bitmap->bm_w / (float)bitmap->bm_h;
			height *= aspect;
		}

		ImGuiIO &io = ImGui::GetIO();

		if (!io.WantCaptureMouse)
		{
			if (io.MouseDown[1])
			{
				viewer.model_distance -= 0.001f*io.MouseDelta.y*viewer.model_distance;
				viewer.model_offset_x += 0.01f*io.MouseDelta.x;
			}
			else if (io.MouseDown[0])
			{
				viewer.model_rotation_dy -= io.MouseDelta.x;
				viewer.model_rotation_dx += io.MouseDelta.y;
			}
		}

		viewer.model_rotation_x += dt*viewer.model_rotation_dx;
		viewer.model_rotation_y += dt*viewer.model_rotation_dy;
		viewer.model_rotation_dx *= 0.95f;
		viewer.model_rotation_dy *= 0.95f;

		if (viewer.model_rotation_x < -180.0f)
			viewer.model_rotation_x += 360.0f;

		if (viewer.model_rotation_x >  180.0f)
			viewer.model_rotation_x -= 360.0f;

		if (viewer.model_rotation_y < -180.0f)
			viewer.model_rotation_y += 360.0f;

		if (viewer.model_rotation_y >  180.0f)
			viewer.model_rotation_y -= 360.0f;

		viewer.model_distance = RT_CLAMP(viewer.model_distance, 10.0f, 200.0f);

		RT_Vec3 view_p = RT_Vec3Make(f2fl(View_position.x), f2fl(View_position.y), f2fl(View_position.z));
		RT_Vec3 view_f = RT_Vec3Normalize(RT_Vec3Make(f2fl(View_matrix.fvec.x), f2fl(View_matrix.fvec.y), f2fl(View_matrix.fvec.z)));
		RT_Vec3 view_u = RT_Vec3Normalize(RT_Vec3Make(f2fl(View_matrix.uvec.x), f2fl(View_matrix.uvec.y), f2fl(View_matrix.uvec.z)));
		RT_Vec3 view_r = RT_Vec3Normalize(RT_Vec3Make(f2fl(View_matrix.rvec.x), f2fl(View_matrix.rvec.y), f2fl(View_matrix.rvec.z)));
		RT_Mat4 T = RT_Mat4FromTranslation(view_p + viewer.model_distance*view_f + viewer.model_offset_x*view_r);
		RT_Mat4 basis = RT_Mat4FromBasisVectors(view_r, view_u, view_f);
		RT_Mat4 Rx = RT_Mat4FromXRotation(RT_RadiansFromDegrees(viewer.model_rotation_x));
		RT_Mat4 Ry = RT_Mat4FromYRotation(RT_RadiansFromDegrees(viewer.model_rotation_y + 180.0f - viewer.spin_offset));
		RT_Mat4 Rz = RT_Mat4FromZRotation(RT_RadiansFromDegrees(viewer.model_rotation_z));
		RT_Mat4 R = basis*Ry*Rx*Rz;
		RT_Mat4 S = RT_Mat4FromScale(RT_Vec3Make(width, height, 1.0f));
		RT_Mat4 transform = T*R*S;

		RT_RenderMeshParams params = {};
		params.mesh_handle       = RT_GetBillboardMesh();
		params.color             = 0xFFFFFFFF;
		params.key.value         = 0x800815;
		params.material_override = viewer.selected_materials[current_index];
		params.transform         = &transform;
		params.instance_mask = RT_RENDER_MASK_OBJECTS;
		RT_RaytraceMeshEx(&params);

		viewer.spin_offset += 45.0f*viewer.spin_speed / 60.0f; // Assumes hardcoded 60 fps. Silly but it's a debug tool, don't care to figure out where frametime is kept.
		if (viewer.spin_offset > 360.0f)
		{
			viewer.spin_offset -= 360.0f;
		}
	}
}
