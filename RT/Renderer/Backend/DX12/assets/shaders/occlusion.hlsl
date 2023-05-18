#include "include/common.hlsl"

struct OcclusionRayPayload
{
	bool visible;
};

void TraceOcclusionRay(RayDesc ray, inout OcclusionRayPayload payload)
{
    TraceRay(g_scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
        ~0, 1, 0, 1, ray, payload);
}

[shader("anyhit")]
void OcclusionAnyhit(inout OcclusionRayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    if (IsHitTransparent(InstanceIndex(), PrimitiveIndex(), attr.barycentrics))
    {
        IgnoreHit();
    }
}

[shader("miss")]
void OcclusionMiss(inout OcclusionRayPayload payload)
{
    payload.visible = true;
}
