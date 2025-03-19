#include "include/common.hlsl"
#include "portal_retrace_ray.hlsl"

struct OcclusionRayPayload
{
	bool visible;
	float hit_distance;
	int hit_segment;                           // what level segment (if any) does the hit belong to.  Could look this up with instance and primitive idx's, but simpler to just record it here.
	int start_segment;
};

void TraceOcclusionRay(RayDesc ray, inout OcclusionRayPayload payload, uint2 pixel_pos)
{
#if RT_DISPATCH_RAYS

    TraceRay(g_scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
        ~2, 1, 0, 1, ray, payload);

#elif RT_INLINE_RAYTRACING

    RayQuery<RAY_FLAG_CULL_BACK_FACING_TRIANGLES> ray_query;
	ray_query.TraceRayInline(
		g_scene,
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		~2,
		ray
	);

	while (ray_query.Proceed())
	{
		switch (ray_query.CandidateType())
		{
			case CANDIDATE_NON_OPAQUE_TRIANGLE:
			{
				uint instance_idx = ray_query.CandidateInstanceIndex();
				uint primitive_idx = ray_query.CandidatePrimitiveIndex();

				InstanceData instance_data = g_instance_data_buffer[instance_idx];
				RT_Triangle hit_triangle = GetHitTriangle(instance_data.triangle_buffer_idx, primitive_idx);
				float hit_distance = ray_query.CandidateTriangleRayT();

				bool valid_hit = true;

				if (tweak.retrace_rays && payload.start_segment != -1)  // retrace rays to handle intersecting level segments
				{
					// bool found = false;

					 // first check if the hit triangle is part of the segment we are looking for
					if (hit_triangle.segment != -1 && hit_triangle.segment != payload.start_segment)
					{
						// if not, setup a retrace ray that starts at the hit location and shoots back to the viewer
						float3 newOrigin = ray.Origin + (ray.Direction * hit_distance);
						RayDesc retrace_ray;
						retrace_ray.Origin = newOrigin;
						retrace_ray.Direction = ray.Direction * -1.0;
						retrace_ray.TMin = 0.0;
						retrace_ray.TMax = hit_distance + 1.0;	// add just a little bit to distance... it helps when the camera is close to a portal surface.

						// setup retrace to check if it passes through portal that leads to segment hit happened in.
						PortalRetraceRayPayload retrace_payload;
						retrace_payload.search_segment = hit_triangle.segment;
						retrace_payload.found = false;
						retrace_payload.hit_distance = RT_RAY_T_MAX;
						retrace_payload.next_segment = -1;

						TracePortalRetraceRay(retrace_ray, retrace_payload, pixel_pos, false);

						// retrace did pass through portal that leads to where the hit happened
						if (retrace_payload.found)
						{
							// does that portal lead to where the player ship is?
							if (retrace_payload.next_segment != payload.start_segment)
							{
								// if it does not, do one more retrace
								retrace_payload.search_segment = retrace_payload.next_segment;
								retrace_payload.found = false;
								retrace_payload.hit_distance = RT_RAY_T_MAX;
								retrace_payload.next_segment = -1;

								TracePortalRetraceRay(retrace_ray, retrace_payload, pixel_pos, false);
								if (!retrace_payload.found)
								{
									// failed second retrace, not valid hit
									valid_hit = false;
								}
							}

						}
						else
						{
							valid_hit = false;
						}
					}

				}

				Material hit_material;

				
				if (valid_hit && !IsHitTransparent(
					ray_query.CandidateInstanceIndex(),
					ray_query.CandidatePrimitiveIndex(),
					ray_query.CandidateTriangleBarycentrics(),
					pixel_pos,
					hit_material
				))
				{
					ray_query.CommitNonOpaqueTriangleHit();
				}
				
				break;
			}
		}
	}

	switch (ray_query.CommittedStatus())
	{
		case COMMITTED_TRIANGLE_HIT:
		{
			float hit_distance = ray_query.CommittedRayT();

			uint instance_idx = ray_query.CommittedInstanceIndex();
			uint primitive_idx = ray_query.CommittedPrimitiveIndex();

			InstanceData instance_data = g_instance_data_buffer[instance_idx];
			RT_Triangle hit_triangle = GetHitTriangle(instance_data.triangle_buffer_idx, primitive_idx);
		
			payload.visible = false;

			payload.hit_distance = hit_distance;
			payload.hit_segment = hit_triangle.segment;
		
			break;
		}
		case COMMITTED_NOTHING:
		{
			payload.visible = true;
			break;
		}
	}

#endif
}

[shader("anyhit")]
void OcclusionAnyhit(inout OcclusionRayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    Material hit_material;
    if (IsHitTransparent(InstanceIndex(), PrimitiveIndex(), attr.barycentrics, DispatchRaysIndex().xy, hit_material))
    {
        IgnoreHit();
    }

    if (hit_material.flags & RT_MaterialFlag_NoCastingShadow)
    {
        IgnoreHit();
    }
}

[shader("miss")]
void OcclusionMiss(inout OcclusionRayPayload payload)
{
    payload.visible = true;
}
