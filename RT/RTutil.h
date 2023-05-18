#pragma once

#include "ApiTypes.h"
#include "Core/MiniMath.h"

typedef struct vms_angvec vms_angvec;
typedef struct g3s_point g3s_point;
typedef struct vms_matrix vms_matrix;
typedef struct vms_vector vms_vector;

typedef struct RT_TriangleBuffer
{
	RT_Triangle *triangles;
	int count;
	int capacity;
} RT_TriangleBuffer;

static inline void RT_PushTriangle(RT_TriangleBuffer *buf, RT_Triangle tri)
{
	if (ALWAYS(buf->count < buf->capacity))
	{
		buf->triangles[buf->count++] = tri;
	}
}

static inline RT_Mat4 RT_Mat4Fromvms_matrix(const vms_matrix* matrix)
{
	RT_Mat4 returnMat = RT_Mat4Identity();

	returnMat.r0.x = f2fl(matrix->rvec.x);
	returnMat.r1.x = f2fl(matrix->rvec.y);
	returnMat.r2.x = f2fl(matrix->rvec.z);

	returnMat.r0.y = f2fl(matrix->uvec.x);
	returnMat.r1.y = f2fl(matrix->uvec.y);
	returnMat.r2.y = f2fl(matrix->uvec.z);

	returnMat.r0.z = f2fl(matrix->fvec.x);
	returnMat.r1.z = f2fl(matrix->fvec.y);
	returnMat.r2.z = f2fl(matrix->fvec.z);

	return returnMat;
}

static inline RT_Vec3 RT_Vec3Fromvms_vector(const vms_vector* vector)
{
	RT_Vec3 returnVector;

	returnVector.x = f2fl(vector->x);
	returnVector.y = f2fl(vector->y);
	returnVector.z = f2fl(vector->z);

	return returnVector;
}
