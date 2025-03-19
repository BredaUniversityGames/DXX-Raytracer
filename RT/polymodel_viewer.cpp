// I'll save myself the trouble and just use C++ for this one.

#include <imgui.h>

// FIXME(daniel): C/C++ interop jank problem:
// I have to wrap those game includes in extern "C", but those includes also like 
// to include our own headers (which is not the best in general), and they have 
// optional C++ stuff in them with the #ifdef __cplusplus guard. 
// Which means that C++ stuff will get the extern "C" specifier, which is not 
// allowed for example for templates. 
// So the solution? Just include our headers _first_, so that the include guard 
// (or pragma once) stops them from being included again.
#include "Renderer.h"

// Ideally, we never would've included our own headers all over the place in game
// headers. But too late for that now.

extern "C"
{
	#include "polyobj.h"
	#include "globvars.h"
}

#include "polymodel_viewer.h"

// FIXME(daniel): Terrible trashy way to get access to this variable... 
// BUT THERE YOU GO. CODE ORGANIZATION, AMIRITE.
extern "C" RT_ResourceHandle mesh_handles[MAX_POLYGON_MODELS];

struct RT_PolymodelViewer
{
	bool render_model;
	int current_model_index;

	float model_distance = 20.0;
	float model_rotation_x;
	float model_rotation_dx;
	float model_rotation_y;
	float model_rotation_dy;
	float model_rotation_z;

	float spin_speed;
	float spin_offset;

	RT_Mat4 prev_transform;
};

static RT_PolymodelViewer viewer;

void RT_DoPolymodelViewerMenus()
{
	if (ImGui::Begin("Polymodel Viewer"))
	{
		if (ImGui::ArrowButton("Prev Model", ImGuiDir_Left))
		{
			viewer.current_model_index = RT_MAX(0, viewer.current_model_index - 1);
		}
		ImGui::SameLine();
		ImGui::Text("polymodel index: %d", viewer.current_model_index);
		ImGui::SameLine();
		if (ImGui::ArrowButton("Next Model", ImGuiDir_Right))
		{
			viewer.current_model_index = RT_MIN(N_polygon_models - 1, viewer.current_model_index + 1);
		}

		polymodel *pm = &Polygon_models[viewer.current_model_index];
		ImGui::Checkbox("Render Current Model", &viewer.render_model);
		if (ImGui::Button("X##ModelDistance")) { viewer.model_distance = 20.0f; } ImGui::SameLine();
		ImGui::SliderFloat("Model Distance", &viewer.model_distance, 10.0f, 200.0f);
		if (ImGui::Button("X##ModelRotationX")) { viewer.model_rotation_x = 0.0f; viewer.model_rotation_dx = 0.0f; } ImGui::SameLine();
		ImGui::SliderFloat("Model Rotation X", &viewer.model_rotation_x, -180.0f, 180.0f);
		if (ImGui::Button("X##ModelRotationY")) { viewer.model_rotation_y = 0.0f; viewer.model_rotation_dy = 0.0f; } ImGui::SameLine();
		ImGui::SliderFloat("Model Rotation Y", &viewer.model_rotation_y, -180.0f, 180.0f);
		if (ImGui::Button("X##ModelRotationZ")) { viewer.model_rotation_z = 0.0f; } ImGui::SameLine();
		ImGui::SliderFloat("Model Rotation Z", &viewer.model_rotation_z, -180.0f, 180.0f);
		if (ImGui::Button("X##Spin")) { viewer.spin_speed = 0.0f; } ImGui::SameLine();
		ImGui::SliderFloat("Spin", &viewer.spin_speed, -1.0f, 1.0f);

		ImGui::Text("Model info:");
		ImGui::Text("n_models:        %d", pm->n_models);
		ImGui::Text("model_data_size: %d", pm->model_data_size);
		ImGui::Text("n_textures:      %d", pm->n_textures);
		ImGui::Text("first_texture:   %d", pm->first_texture);
		ImGui::Text("simpler_model:   %u", pm->simpler_model);
	} ImGui::End();
}

void RT_RenderPolyModelViewer()
{
	if (viewer.render_model)
	{
		ImGuiIO &io = ImGui::GetIO();

		if (!io.WantCaptureMouse)
		{
			if (io.MouseDown[1])
			{
				viewer.model_distance -= 0.1f*io.MouseDelta.y;
			}
			else if (io.MouseDown[0])
			{
				viewer.model_rotation_dy -= io.MouseDelta.x;
				viewer.model_rotation_dx += io.MouseDelta.y;
			}
		}

		float dt = 1.0f / 60.0f;
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

		RT_ResourceHandle handle = mesh_handles[viewer.current_model_index];
		if (RT_RESOURCE_HANDLE_VALID(handle))
		{
			RT_Vec3 view_p = RT_Vec3Make(f2fl(View_position.x), f2fl(View_position.y), f2fl(View_position.z));
			RT_Vec3 view_f = RT_Vec3Normalize(RT_Vec3Make(f2fl(View_matrix.fvec.x), f2fl(View_matrix.fvec.y), f2fl(View_matrix.fvec.z)));
			RT_Vec3 view_u = RT_Vec3Normalize(RT_Vec3Make(f2fl(View_matrix.uvec.x), f2fl(View_matrix.uvec.y), f2fl(View_matrix.uvec.z)));
			RT_Vec3 view_r = RT_Vec3Normalize(RT_Vec3Make(f2fl(View_matrix.rvec.x), f2fl(View_matrix.rvec.y), f2fl(View_matrix.rvec.z)));
			RT_Mat4 T = RT_Mat4FromTranslation(view_p + viewer.model_distance*view_f);
			RT_Mat4 basis = RT_Mat4FromBasisVectors(view_r, view_u, view_f);
			RT_Mat4 Rx = RT_Mat4FromXRotation(RT_RadiansFromDegrees(viewer.model_rotation_x));
			RT_Mat4 Ry = RT_Mat4FromYRotation(RT_RadiansFromDegrees(viewer.model_rotation_y + 180.0f - viewer.spin_offset));
			RT_Mat4 Rz = RT_Mat4FromZRotation(RT_RadiansFromDegrees(viewer.model_rotation_z));
			RT_Mat4 R = basis*Ry*Rx*Rz;
			RT_Mat4 transform = T*R;
			RT_RaytraceMesh(handle, &transform, &viewer.prev_transform, RT_RENDER_MASK_OBJECTS);
			viewer.prev_transform = transform;

			viewer.spin_offset += 45.0f*viewer.spin_speed / 60.0f; // Assumes hardcoded 60 fps. Silly but it's a debug tool, don't care to figure out where frametime is kept.
			if (viewer.spin_offset > 360.0f)
			{
				viewer.spin_offset -= 360.0f;
			}
		}
	}
}
