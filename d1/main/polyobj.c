/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Hacked-in polygon objects
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DRIVE
#include "drive.h"
#else
#include "inferno.h"
#endif
#include "polyobj.h"
#include "vecmat.h"
#include "3d.h"
#include "dxxerror.h"
#include "u_mem.h"
#include "args.h"
#ifndef DRIVE
#include "texmap.h"
#include "bm.h"
#include "textures.h"
#include "object.h"
#include "lighting.h"
#include "piggy.h"
#endif
#include "byteswap.h"
#include "render.h"
#ifdef OGL
#include "ogl_init.h"
#endif
#include "logger.h"

#ifdef RT_DX12
#include "RTgr.h"
#include "RTmaterials.h"
#include "Core/MiniMath.h"
#endif //RT_DX12

polymodel Polygon_models[MAX_POLYGON_MODELS];	// = {&bot11,&bot17,&robot_s2,&robot_b2,&bot11,&bot17,&robot_s2,&robot_b2};

int N_polygon_models = 0;

#define MAX_POLYGON_VECS 1000
g3s_point robot_points[MAX_POLYGON_VECS];

#define PM_COMPATIBLE_VERSION 6
#define PM_OBJFILE_VERSION 8

int	Pof_file_end;
int	Pof_addr;

#define	MODEL_BUF_SIZE	32768

void _pof_cfseek(int len, int type)
{
	switch (type) {
	case SEEK_SET:	Pof_addr = len;	break;
	case SEEK_CUR:	Pof_addr += len;	break;
	case SEEK_END:
		Assert(len <= 0);	//	seeking from end, better be moving back.
		Pof_addr = Pof_file_end + len;
		break;
	}

	if (Pof_addr > MODEL_BUF_SIZE)
		Int3();
}

#define pof_cfseek(_buf,_len,_type) _pof_cfseek((_len),(_type))

int pof_read_int(ubyte* bufp)
{
	int i;

	i = *((int*)&bufp[Pof_addr]);
	Pof_addr += 4;
	return INTEL_INT(i);

	//	if (PHYSFS_read(f,&i,sizeof(i),1) != 1)
	//		RT_LOGF(RT_LOGSERVERITY_HIGH, "Unexpected end-of-file while reading object");
	//
	//	return i;
}

size_t pof_cfread(void* dst, size_t elsize, size_t nelem, ubyte* bufp)
{
	if (Pof_addr + nelem * elsize > Pof_file_end)
		return 0;

	memcpy(dst, &bufp[Pof_addr], elsize * nelem);

	Pof_addr += elsize * nelem;

	if (Pof_addr > MODEL_BUF_SIZE)
		Int3();

	return nelem;
}

// #define new_read_int(i,f) PHYSFS_read((f),&(i),sizeof(i),1)
#define new_pof_read_int(i,f) pof_cfread(&(i),sizeof(i),1,(f))

short pof_read_short(ubyte* bufp)
{
	short s;

	s = *((short*)&bufp[Pof_addr]);
	Pof_addr += 2;
	return INTEL_SHORT(s);
	//	if (PHYSFS_read(f,&s,sizeof(s),1) != 1)
	//		RT_LOGF(RT_LOGSERVERITY_HIGH, "Unexpected end-of-file while reading object");
	//
	//	return s;
}

void pof_read_string(char* buf, int max, ubyte* bufp)
{
	int	i;

	for (i = 0; i < max; i++) {
		if ((*buf++ = bufp[Pof_addr++]) == 0)
			break;
	}

	//	while (max-- && (*buf=PHYSFSX_fgetc(f)) != 0) buf++;

}

void pof_read_vecs(vms_vector* vecs, int n, ubyte* bufp)
{
	int i;
	//	PHYSFS_read(f,vecs,sizeof(vms_vector),n);

	for (i = 0; i < n; i++)
	{
		vecs[i].x = pof_read_int(bufp);
		vecs[i].y = pof_read_int(bufp);
		vecs[i].z = pof_read_int(bufp);
	}
}

void pof_read_angvecs(vms_angvec* vecs, int n, ubyte* bufp)
{
	int i;
	//	PHYSFS_read(f,vecs,sizeof(vms_vector),n);

	for (i = 0; i < n; i++)
	{
		vecs[i].p = pof_read_short(bufp);
		vecs[i].b = pof_read_short(bufp);
		vecs[i].h = pof_read_short(bufp);
	}
}

#define ID_OHDR 0x5244484f // 'RDHO'  //Object header
#define ID_SOBJ 0x4a424f53 // 'JBOS'  //Subobject header
#define ID_GUNS 0x534e5547 // 'SNUG'  //List of guns on this object
#define ID_ANIM 0x4d494e41 // 'MINA'  //Animation data
#define ID_IDTA 0x41544449 // 'ATDI'  //Interpreter data
#define ID_TXTR 0x52545854 // 'RTXT'  //Texture filename list

#ifdef DRIVE
#define robot_info void
#else
vms_angvec anim_angs[N_ANIM_STATES][MAX_SUBMODELS];

//set the animation angles for this robot.  Gun fields of robot info must
//be filled in.
void robot_set_angles(robot_info* r, polymodel* pm, vms_angvec angs[N_ANIM_STATES][MAX_SUBMODELS]);
#endif

#ifdef WORDS_NEED_ALIGNMENT
ubyte* old_dest(chunk o) // return where chunk is (in unaligned struct)
{
	return o.old_base + INTEL_SHORT(*((short*)(o.old_base + o.offset)));
}

ubyte* new_dest(chunk o) // return where chunk is (in aligned struct)
{
	return o.new_base + INTEL_SHORT(*((short*)(o.old_base + o.offset))) + o.correction;
}

/*
 * find chunk with smallest address
 */
int get_first_chunks_index(chunk* chunk_list, int no_chunks)
{
	int i, first_index = 0;
	Assert(no_chunks >= 1);
	for (i = 1; i < no_chunks; i++)
		if (old_dest(chunk_list[i]) < old_dest(chunk_list[first_index]))
			first_index = i;
	return first_index;
}
#define SHIFT_SPACE 500 // increase if insufficent

void align_polygon_model_data(polymodel* pm)
{
	int i, chunk_len;
	int total_correction = 0;
	ubyte* cur_old, * cur_new;
	chunk cur_ch;
	chunk ch_list[MAX_CHUNKS];
	int no_chunks = 0;
	int tmp_size = pm->model_data_size + SHIFT_SPACE;
	ubyte* tmp = d_malloc(tmp_size); // where we build the aligned version of pm->model_data

	Assert(tmp != NULL);
	//start with first chunk (is always aligned!)
	cur_old = pm->model_data;
	cur_new = tmp;
	chunk_len = get_chunks(cur_old, cur_new, ch_list, &no_chunks);
	memcpy(cur_new, cur_old, chunk_len);
	while (no_chunks > 0) {
		int first_index = get_first_chunks_index(ch_list, no_chunks);
		cur_ch = ch_list[first_index];
		// remove first chunk from array:
		no_chunks--;
		for (i = first_index; i < no_chunks; i++)
			ch_list[i] = ch_list[i + 1];
		// if (new) address unaligned:
		if ((u_int32_t)new_dest(cur_ch) % 4L != 0) {
			// calculate how much to move to be aligned
			short to_shift = 4 - (u_int32_t)new_dest(cur_ch) % 4L;
			// correct chunks' addresses
			cur_ch.correction += to_shift;
			for (i = 0; i < no_chunks; i++)
				ch_list[i].correction += to_shift;
			total_correction += to_shift;
			Assert((u_int32_t)new_dest(cur_ch) % 4L == 0);
			Assert(total_correction <= SHIFT_SPACE); // if you get this, increase SHIFT_SPACE
		}
		//write (corrected) chunk for current chunk:
		*((short*)(cur_ch.new_base + cur_ch.offset))
			= INTEL_SHORT(cur_ch.correction
				+ INTEL_SHORT(*((short*)(cur_ch.old_base + cur_ch.offset))));
		//write (correctly aligned) chunk:
		cur_old = old_dest(cur_ch);
		cur_new = new_dest(cur_ch);
		chunk_len = get_chunks(cur_old, cur_new, ch_list, &no_chunks);
		memcpy(cur_new, cur_old, chunk_len);
		//correct submodel_ptr's for pm, too
		for (i = 0; i < MAX_SUBMODELS; i++)
			if (pm->model_data + pm->submodel_ptrs[i] >= cur_old
				&& pm->model_data + pm->submodel_ptrs[i] < cur_old + chunk_len)
				pm->submodel_ptrs[i] += (cur_new - tmp) - (cur_old - pm->model_data);
	}
	d_free(pm->model_data);
	pm->model_data_size += total_correction;
	pm->model_data =
		d_malloc(pm->model_data_size);
	Assert(pm->model_data != NULL);
	memcpy(pm->model_data, tmp, pm->model_data_size);
	d_free(tmp);
}
#endif //def WORDS_NEED_ALIGNMENT


//reads a binary file containing a 3d model
polymodel* read_model_file(polymodel* pm, char* filename, robot_info* r)
{
	PHYSFS_file* ifile;
	short version;
	int id, len, next_chunk;
	ubyte	model_buf[MODEL_BUF_SIZE];

	if ((ifile = PHYSFSX_openReadBuffered(filename)) == NULL)
		RT_LOGF(RT_LOGSERVERITY_HIGH, "Can't open file <%s>", filename);

	Assert(PHYSFS_fileLength(ifile) <= MODEL_BUF_SIZE);

	Pof_addr = 0;
	Pof_file_end = PHYSFS_read(ifile, model_buf, 1, PHYSFS_fileLength(ifile));
	PHYSFS_close(ifile);

	id = pof_read_int(model_buf);

	if (id != 0x4f505350) /* 'OPSP' */
		RT_LOGF(RT_LOGSERVERITY_HIGH, "Bad ID in model file <%s>", filename);

	version = pof_read_short(model_buf);

	if (version < PM_COMPATIBLE_VERSION || version > PM_OBJFILE_VERSION)
		RT_LOGF(RT_LOGSERVERITY_HIGH, "Bad version (%d) in model file <%s>", version, filename);

	while (new_pof_read_int(id, model_buf) == 1) {
		id = INTEL_INT(id);
		//id  = pof_read_int(model_buf);
		len = pof_read_int(model_buf);
		next_chunk = Pof_addr + len;

		switch (id) {

		case ID_OHDR: {		//Object header
			vms_vector pmmin, pmmax;

			pm->n_models = pof_read_int(model_buf);
			pm->rad = pof_read_int(model_buf);

			Assert(pm->n_models <= MAX_SUBMODELS);

			pof_read_vecs(&pmmin, 1, model_buf);
			pof_read_vecs(&pmmax, 1, model_buf);

			break;
		}

		case ID_SOBJ: {		//Subobject header
			int n;

			n = pof_read_short(model_buf);

			Assert(n < MAX_SUBMODELS);

			pm->submodel_parents[n] = pof_read_short(model_buf);

			pof_read_vecs(&pm->submodel_norms[n], 1, model_buf);
			pof_read_vecs(&pm->submodel_pnts[n], 1, model_buf);
			pof_read_vecs(&pm->submodel_offsets[n], 1, model_buf);

			pm->submodel_rads[n] = pof_read_int(model_buf);		//radius

			pm->submodel_ptrs[n] = pof_read_int(model_buf);	//offset

			break;

		}

#ifndef DRIVE
		case ID_GUNS: {		//List of guns on this object

			if (r) {
				int i;
				vms_vector gun_dir;

				r->n_guns = pof_read_int(model_buf);

				Assert(r->n_guns <= MAX_GUNS);

				for (i = 0; i < r->n_guns; i++) {
					int id;

					id = pof_read_short(model_buf);
					r->gun_submodels[id] = pof_read_short(model_buf);
					Assert(r->gun_submodels[id] != 0xff);
					pof_read_vecs(&r->gun_points[id], 1, model_buf);

					if (version >= 7)
						pof_read_vecs(&gun_dir, 1, model_buf);
				}
			}
			else
				pof_cfseek(model_buf, len, SEEK_CUR);

			break;
		}

		case ID_ANIM:		//Animation data
			if (r) {
				int n_frames, f, m;

				n_frames = pof_read_short(model_buf);

				Assert(n_frames == N_ANIM_STATES);

				for (m = 0; m < pm->n_models; m++)
					for (f = 0; f < n_frames; f++)
						pof_read_angvecs(&anim_angs[f][m], 1, model_buf);

				robot_set_angles(r, pm, anim_angs);

			}
			else
				pof_cfseek(model_buf, len, SEEK_CUR);

			break;
#endif

		case ID_TXTR: {		//Texture filename list
			int n;
			char name_buf[128];

			n = pof_read_short(model_buf);
			while (n--) {
				pof_read_string(name_buf, 128, model_buf);
			}

			break;
		}

		case ID_IDTA:		//Interpreter data
			pm->model_data = d_malloc(len);
			pm->model_data_size = len;

			pof_cfread(pm->model_data, 1, len, model_buf);

			break;

		default:
			pof_cfseek(model_buf, len, SEEK_CUR);
			break;

		}
		if (version >= 8)		// Version 8 needs 4-byte alignment!!!
			pof_cfseek(model_buf, next_chunk, SEEK_SET);
	}

#ifdef WORDS_NEED_ALIGNMENT
	align_polygon_model_data(pm);
#endif
#ifdef WORDS_BIGENDIAN
	swap_polygon_model_data(pm->model_data);
#endif

	return pm;
}

//reads the gun information for a model
//fills in arrays gun_points & gun_dirs, returns the number of guns read
int read_model_guns(char* filename, vms_vector* gun_points, vms_vector* gun_dirs, int* gun_submodels)
{
	PHYSFS_file* ifile;
	short version;
	int id, len;
	int n_guns = 0;
	ubyte	model_buf[MODEL_BUF_SIZE];

	if ((ifile = PHYSFSX_openReadBuffered(filename)) == NULL)
		RT_LOGF(RT_LOGSERVERITY_HIGH, "Can't open file <%s>", filename);

	Assert(PHYSFS_fileLength(ifile) <= MODEL_BUF_SIZE);

	Pof_addr = 0;
	Pof_file_end = PHYSFS_read(ifile, model_buf, 1, PHYSFS_fileLength(ifile));
	PHYSFS_close(ifile);

	id = pof_read_int(model_buf);

	if (id != 0x4f505350) /* 'OPSP' */
		RT_LOGF(RT_LOGSERVERITY_HIGH, "Bad ID in model file <%s>", filename);

	version = pof_read_short(model_buf);

	Assert(version >= 7);		//must be 7 or higher for this data

	if (version < PM_COMPATIBLE_VERSION || version > PM_OBJFILE_VERSION)
		RT_LOGF(RT_LOGSERVERITY_HIGH, "Bad version (%d) in model file <%s>", version, filename);

	while (new_pof_read_int(id, model_buf) == 1) {
		id = INTEL_INT(id);
		//id  = pof_read_int(model_buf);
		len = pof_read_int(model_buf);

		if (id == ID_GUNS) {		//List of guns on this object

			int i;

			n_guns = pof_read_int(model_buf);

			for (i = 0; i < n_guns; i++) {
				int id, sm;

				id = pof_read_short(model_buf);
				sm = pof_read_short(model_buf);
				if (gun_submodels)
					gun_submodels[id] = sm;
				else if (sm != 0)
					RT_LOGF(RT_LOGSERVERITY_HIGH, "Invalid gun submodel in file <%s>", filename);
				pof_read_vecs(&gun_points[id], 1, model_buf);

				pof_read_vecs(&gun_dirs[id], 1, model_buf);
			}

		}
		else
			pof_cfseek(model_buf, len, SEEK_CUR);

	}

	return n_guns;
}

//free up a model, getting rid of all its memory
void free_model(polymodel* po)
{
	d_free(po->model_data);
}

grs_bitmap* texture_list[MAX_POLYOBJ_TEXTURES];
bitmap_index texture_list_index[MAX_POLYOBJ_TEXTURES];

//draw a polygon model

void draw_polygon_model(_RT_DRAW_POLY vms_vector* pos, vms_matrix* orient, vms_angvec* anim_angles, int model_num, int flags, g3s_lrgb light, fix* glow_values, bitmap_index alt_textures[])
{
	polymodel* po;
	int i;

	if (model_num < 0)
		return;

	Assert(model_num < N_polygon_models);

	po = &Polygon_models[model_num];

	//check if should use simple model
	if (po->simpler_model)					//must have a simpler model
		if (flags == 0)							//can't switch if this is debris
			//!!if (!alt_textures) {				//alternate textures might not match
			//alt textures might not match, but in the one case we're using this
			//for on 11/14/94, they do match.  So we leave it in.
		{
			int cnt = 1;
			fix depth;

			depth = g3_calc_point_depth(pos);		//gets 3d depth

			while (po->simpler_model && depth > cnt++ * Simple_model_threshhold_scale * po->rad)
				po = &Polygon_models[po->simpler_model - 1];
		}

	if (alt_textures)
		// TODO(daniel): When do these alt textures get used? Our renderer doesn't currently support anything like this.
		for (i = 0; i < po->n_textures; i++) {
			texture_list_index[i] = alt_textures[i];
			texture_list[i] = &GameBitmaps[alt_textures[i].index];
		}
	else
		for (i = 0; i < po->n_textures; i++) {
			texture_list_index[i] = ObjBitmaps[ObjBitmapPtrs[po->first_texture + i]];
			texture_list[i] = &GameBitmaps[ObjBitmaps[ObjBitmapPtrs[po->first_texture + i]].index];
		}

	// Make sure the textures for this object are paged in...
	piggy_page_flushed = 0;
	for (i = 0; i < po->n_textures; i++)
		PIGGY_PAGE_IN(texture_list_index[i]);
	// Hmmm... cache got flushed in the middle of paging all these in,
	// so we need to reread them all in.
	if (piggy_page_flushed) {
		piggy_page_flushed = 0;
		for (i = 0; i < po->n_textures; i++)
			PIGGY_PAGE_IN(texture_list_index[i]);
	}
	// Make sure that they can all fit in memory.
	Assert(piggy_page_flushed == 0);

	g3_start_instance_matrix(pos, orient);

	g3_set_interp_points(robot_points);

	if (flags == 0)		//draw entire object
	{
#ifndef RT_DX12
		g3_draw_polygon_model(po->model_data, texture_list, anim_angles, light, glow_values);
#else
		//RT_DrawPolyModel(model_num, objNum, object_type, pos, orient);

		RT_DrawPolyModelTree(model_num, objNum, object_type, pos, orient, anim_angles);
#endif //RT_DX12
	}
	else {
		int i;

		for (i = 0; flags; flags >>= 1, i++)
			if (flags & 1) {
				vms_vector ofs;

				Assert(i < po->n_models);

				//if submodel, rotate around its center point, not pivot point

				vm_vec_avg(&ofs, &po->submodel_mins[i], &po->submodel_maxs[i]);
				vm_vec_negate(&ofs);
				g3_start_instance_matrix(&ofs, NULL);
#ifndef RT_DX12
				g3_draw_polygon_model(&po->model_data[po->submodel_ptrs[i]], texture_list, anim_angles, light, glow_values);
#else
				// Get matrix from local position offset
				const vms_vector offset_vms = *pos;
				const RT_Vec3 offset_vec3 = RT_Vec3Fromvms_vector(&offset_vms);
				RT_Mat4 offset_mat4 = RT_Mat4FromTranslation(offset_vec3);

				// Get matrix from rotation offset
				RT_Mat4 rotation_mat4 = RT_Mat4Fromvms_matrix(orient);

				// Combine them into one big matrix
				RT_Mat4 combined_matrix = RT_Mat4Mul(offset_mat4, rotation_mat4);
				RT_Mat4 prev_transform = g_rt_prev_submodel_transforms[objNum].transforms[i];
				RT_DrawSubPolyModel(po->submodel[i], &combined_matrix, &prev_transform);
				g_rt_prev_submodel_transforms[objNum].transforms[i] = combined_matrix;
				g_rt_prev_submodel_transforms[objNum].last_frame_updated[i] = g_rt_frame_index;
#endif //RT_DX12

				g3_done_instance();
			}
	}

	g3_done_instance();

}

void free_polygon_models()
{
	int i;

	for (i = 0; i < N_polygon_models; i++) {
		free_model(&Polygon_models[i]);
	}

}

void polyobj_find_min_max(polymodel* pm)
{
	ushort nverts;
	vms_vector* vp;
	ushort* data, type;
	int m;
	vms_vector* big_mn, * big_mx;

	big_mn = &pm->mins;
	big_mx = &pm->maxs;

	for (m = 0; m < pm->n_models; m++) {
		vms_vector* mn, * mx, * ofs;

		mn = &pm->submodel_mins[m];
		mx = &pm->submodel_maxs[m];
		ofs = &pm->submodel_offsets[m];

		data = (ushort*)&pm->model_data[pm->submodel_ptrs[m]];

		type = *data++;

		Assert(type == 7 || type == 1);

		nverts = *data++;

		if (type == 7)
			data += 2;		//skip start & pad

		vp = (vms_vector*)data;

		*mn = *mx = *vp++; nverts--;

		if (m == 0)
			*big_mn = *big_mx = *mn;

		while (nverts--) {
			if (vp->x > mx->x) mx->x = vp->x;
			if (vp->y > mx->y) mx->y = vp->y;
			if (vp->z > mx->z) mx->z = vp->z;

			if (vp->x < mn->x) mn->x = vp->x;
			if (vp->y < mn->y) mn->y = vp->y;
			if (vp->z < mn->z) mn->z = vp->z;

			if (vp->x + ofs->x > big_mx->x) big_mx->x = vp->x + ofs->x;
			if (vp->y + ofs->y > big_mx->y) big_mx->y = vp->y + ofs->y;
			if (vp->z + ofs->z > big_mx->z) big_mx->z = vp->z + ofs->z;

			if (vp->x + ofs->x < big_mn->x) big_mn->x = vp->x + ofs->x;
			if (vp->y + ofs->y < big_mn->y) big_mn->y = vp->y + ofs->y;
			if (vp->z + ofs->z < big_mn->z) big_mn->z = vp->z + ofs->z;

			vp++;
		}
	}
}

char Pof_names[MAX_POLYGON_MODELS][13];

//returns the number of this model
#ifndef DRIVE
int load_polygon_model(char* filename, int n_textures, int first_texture, robot_info* r)
#else
int load_polygon_model(char* filename, int n_textures, grs_bitmap*** textures)
#endif
{
#ifdef DRIVE
#define r NULL
#endif

	Assert(N_polygon_models < MAX_POLYGON_MODELS);
	Assert(n_textures < MAX_POLYOBJ_TEXTURES);

	Assert(strlen(filename) <= 12);
	strcpy(Pof_names[N_polygon_models], filename);

	read_model_file(&Polygon_models[N_polygon_models], filename, r);

	polyobj_find_min_max(&Polygon_models[N_polygon_models]);

#ifndef RT_DX12
	g3_init_polygon_model(Polygon_models[N_polygon_models].model_data);
#else
	//Note: Stan
	//Still have to call this function to render the polygon models correclty.
	//Sam will look at this later.
	g3_init_polygon_model(Polygon_models[N_polygon_models].model_data);
	
	// NOTE(daniel): What's this call? Why are we initializing poly models in multiple places?
	//RT_InitPolyModel(N_polygon_models, robot_points, Polygon_models[N_polygon_models].model_data, NULL, 0);
#endif //RT_DX12

	if (highest_texture_num + 1 != n_textures)
		RT_LOGF(RT_LOGSERVERITY_HIGH, "Model <%s> references %d textures but specifies %d.", filename, highest_texture_num + 1, n_textures);

	Polygon_models[N_polygon_models].n_textures = n_textures;
	Polygon_models[N_polygon_models].first_texture = first_texture;
	Polygon_models[N_polygon_models].simpler_model = 0;

	//Assert(polygon_models[N_polygon_models]!=NULL);

	N_polygon_models++;
	return N_polygon_models - 1;
}


void init_polygon_models()
{
	N_polygon_models = 0;
}

//compare against this size when figuring how far to place eye for picture
#define BASE_MODEL_SIZE 0x28000

#define DEFAULT_VIEW_DIST 0x60000

//draws the given model in the current canvas.  The distance is set to
//more-or-less fill the canvas.  Note that this routine actually renders
//into an off-screen canvas that it creates, then copies to the current
//canvas.
void draw_model_picture(int mn, vms_angvec* orient_angles)
{
	vms_vector	temp_pos = ZERO_VECTOR;
	vms_matrix	temp_orient = IDENTITY_MATRIX;
	g3s_lrgb	lrgb = { f1_0, f1_0, f1_0 };

	Assert(mn >= 0 && mn < N_polygon_models);


	gr_clear_canvas(BM_XRGB(0, 0, 0));
	g3_start_frame();
	g3_set_view_matrix(&temp_pos, &temp_orient, 0x9000);


#ifndef RT_DX12
	if (Polygon_models[mn].rad != 0)
		temp_pos.z = fixmuldiv(DEFAULT_VIEW_DIST, Polygon_models[mn].rad, BASE_MODEL_SIZE);
	else
		temp_pos.z = DEFAULT_VIEW_DIST;

	vm_angles_2_matrix(&temp_orient, orient_angles);
	draw_polygon_model(&temp_pos, &temp_orient, NULL, mn, 0, lrgb, NULL, NULL, OBJ_NONE);
#else

	// TODO(Justin): This needs to be fixed once we got blitting - enemy briefing
	//if (Polygon_models[mn].rad != 0)
	//	temp_pos.z = fixmuldiv(DEFAULT_VIEW_DIST, Polygon_models[mn].rad, BASE_MODEL_SIZE);
	//else
	//	temp_pos.z = DEFAULT_VIEW_DIST;
	//vm_angles_2_matrix(&temp_orient, orient_angles);

	//// Alright, let's create a struct that holds a raster triangle + calculated depth, for sorting later
	//float tri_avg_depths[2048];
	//RT_RasterTriVertex triangles[2048][3];
	//int new_tri_list_size = 0;

	//// For each triangle of the model
	//for (int tri_i = 0; tri_i < meshVerticesRawHack[mn].triangle_count; ++tri_i) {
	//	// Get the triangle - it should already be in view space
	//	RT_Triangle* tri = &meshVerticesRawHack[mn].triangles[tri_i];

	//	// Transform to NDC space - static variable so we don't have to recalculate the matrix but I'm too lazy to put it in a more sane spot
	//	static RT_Mat4 proj_mat;
	//	static bool proj_mat_inited = false;
	//	if (!proj_mat_inited) {
	//		proj_mat = RT_Mat4Perspective(90.f * (3.14159265359f / 180.f), 16.0f / 9.0f, 0.1, 500.f);
	//	}

	//	// Get the position
	//	RT_Vec4 p0 = { tri->pos0.x, tri->pos0.y, tri->pos0.z, 1.0f };
	//	RT_Vec4 p1 = { tri->pos1.x, tri->pos1.y, tri->pos1.z, 1.0f };
	//	RT_Vec4 p2 = { tri->pos2.x, tri->pos2.y, tri->pos2.z, 1.0f };

	//	// Apply rotation
	//	RT_Mat4 rot_mat = RT_Mat4Fromvms_matrix(&temp_orient);
	//	p0 = RT_Mat4TransformVec4(rot_mat, p0);
	//	p1 = RT_Mat4TransformVec4(rot_mat, p1);
	//	p2 = RT_Mat4TransformVec4(rot_mat, p2);

	//	// Apply view
	//	p0.z += f2fl(temp_pos.z);
	//	p1.z += f2fl(temp_pos.z);
	//	p2.z += f2fl(temp_pos.z);

	//	// Perspective projection
	//	p0 = RT_Mat4TransformVec4(proj_mat, p0);
	//	p1 = RT_Mat4TransformVec4(proj_mat, p1);
	//	p2 = RT_Mat4TransformVec4(proj_mat, p2);

	//	// Perspective divide
	//	p0.x /= p0.w;
	//	p1.x /= p1.w;
	//	p2.x /= p2.w;
	//	p0.y /= p0.w;
	//	p1.y /= p1.w;
	//	p2.y /= p2.w;
	//	p0.z /= p0.w;
	//	p1.z /= p1.w;
	//	p2.z /= p2.w;

	//	// Move it over scuffedly
	//	p0.x = p0.x * 2.5f + 0.2f;
	//	p1.x = p1.x * 2.5f + 0.2f;
	//	p2.x = p2.x * 2.5f + 0.2f;
	//	p0.y = p0.y * 2.5f - 0.3f;
	//	p1.y = p1.y * 2.5f - 0.3f;
	//	p2.y = p2.y * 2.5f - 0.3f;

	//	// Create vertex
	//	RT_Vertex v0 = { p0.xyz, tri->uv0, tri->normal0, tri->tangent0.xyz, {0, 0, 0} };
	//	RT_Vertex v1 = { p2.xyz, tri->uv2, tri->normal2, tri->tangent2.xyz, {0, 0, 0} };
	//	RT_Vertex v2 = { p1.xyz, tri->uv1, tri->normal1, tri->tangent1.xyz, {0, 0, 0} };

	//	// Create raster triangle from it
	//	RT_RasterTriVertex raster_tri_v0 = { p0.xyz, v0.uv, {1,1,1,1}, tri->material_edge_index };
	//	RT_RasterTriVertex raster_tri_v1 = { p1.xyz, v1.uv, {1,1,1,1}, tri->material_edge_index };
	//	RT_RasterTriVertex raster_tri_v2 = { p2.xyz, v2.uv, {1,1,1,1}, tri->material_edge_index };
	//	float avg_depth = v0.pos.z + v1.pos.z + v2.pos.z; // I would divide by 3 here but it's for sorting only, we don't care

	//	// Depth culling
	//	if (avg_depth < 0.0f || avg_depth > 3.0f) {
	//		continue;
	//	}

	//	// Add it to the list
	//	triangles[new_tri_list_size][0] = raster_tri_v0;
	//	triangles[new_tri_list_size][1] = raster_tri_v1;
	//	triangles[new_tri_list_size][2] = raster_tri_v2;
	//	tri_avg_depths[new_tri_list_size] = avg_depth;

	//	// Move to next entry
	//	new_tri_list_size++;
	//}

	// Sort the triangles - bubble sort
	//int curr_max = new_tri_list_size;
	//while (--curr_max) {
	//	// Loop over all unsorted elements
	//	for (int i = 0; i < curr_max; ++i) {
	//		// Swap triangles if this one pair's depths are the wrong way around
	//		if (tri_avg_depths[i] > tri_avg_depths[i + 1]) {
	//			// Swap depth
	//			{
	//				float tmp = tri_avg_depths[i];
	//				tri_avg_depths[i] = tri_avg_depths[i + 1];
	//				tri_avg_depths[i + 1] = tmp;
	//			}
	//			// Swap triangle
	//			for (int j = 0; j < 3; ++j)
	//			{
	//				RT_RasterTriVertex tmp = triangles[i][j];
	//				triangles[i][j] = triangles[i + 1][j];
	//				triangles[i + 1][j] = tmp;
	//			}
	//		}
	//	}
	//}

	// Note(Justin): This could be wrong, but I do not care, since we will replace this with raytraced models anyways
	/*RT_RasterTrianglesParams raster_tri_params = { 0 };
	raster_tri_params.texture_handle = g_rt_materials[triangles[0]->texture_index].albedo_texture;
	raster_tri_params.num_vertices = new_tri_list_size;
	raster_tri_params.vertices = triangles;

	RT_RasterTriangles(&raster_tri_params, 1);*/
	// Draw each triangle of the model
	//for (size_t i = 0; i < new_tri_list_size; ++i) {
	//	// Draw triangles
	//	RT_RasterTriangle(materials[triangles[i][0].texture_index].albedo_texture, triangles[i]);
	//}
#endif //RT_DX12

	g3_end_frame();
}


/*
 * reads n polymodel structs from a PHYSFS_file
 */
extern int polymodel_read_n(polymodel* pm, int n, PHYSFS_file* fp)
{
	int i, j;

	for (i = 0; i < n; i++) {
		pm[i].n_models = PHYSFSX_readInt(fp);
		pm[i].model_data_size = PHYSFSX_readInt(fp);
		pm[i].model_data = (ubyte*)(size_t)PHYSFSX_readInt(fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			pm[i].submodel_ptrs[j] = PHYSFSX_readInt(fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			PHYSFSX_readVector(&(pm[i].submodel_offsets[j]), fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			PHYSFSX_readVector(&(pm[i].submodel_norms[j]), fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			PHYSFSX_readVector(&(pm[i].submodel_pnts[j]), fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			pm[i].submodel_rads[j] = PHYSFSX_readFix(fp);
		PHYSFS_read(fp, pm[i].submodel_parents, MAX_SUBMODELS, 1);
		for (j = 0; j < MAX_SUBMODELS; j++)
			PHYSFSX_readVector(&(pm[i].submodel_mins[j]), fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			PHYSFSX_readVector(&(pm[i].submodel_maxs[j]), fp);
		PHYSFSX_readVector(&(pm[i].mins), fp);
		PHYSFSX_readVector(&(pm[i].maxs), fp);
		pm[i].rad = PHYSFSX_readFix(fp);
		pm[i].n_textures = PHYSFSX_readByte(fp);
		pm[i].first_texture = PHYSFSX_readShort(fp);
		pm[i].simpler_model = PHYSFSX_readByte(fp);
	}
	return i;
}

/*
 * routine which allocates, reads, and inits a polymodel's model_data
 */
void polygon_model_data_read(polymodel* pm, PHYSFS_file* fp)
{
	pm->model_data = d_malloc(pm->model_data_size);
	Assert(pm->model_data != NULL);
	PHYSFS_read(fp, pm->model_data, sizeof(ubyte), pm->model_data_size);
#ifdef WORDS_NEED_ALIGNMENT
	align_polygon_model_data(pm);
#endif
#ifdef WORDS_BIGENDIAN
	swap_polygon_model_data(pm->model_data);
#endif
}