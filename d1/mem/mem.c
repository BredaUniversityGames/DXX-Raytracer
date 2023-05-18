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
 * Files for debugging memory allocator
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "physfsx.h"
#include "pstypes.h"
#include "args.h"
#include "console.h"
#include "logger.h"

#define MEMSTATS 0
#define FULL_MEM_CHECKING 1

#if defined(FULL_MEM_CHECKING) && !defined(NDEBUG)

#define CHECKSIZE 16
#define CHECKBYTE 0xFC

#define MAX_INDEX 10000

static void *MallocBase[MAX_INDEX];
static unsigned int MallocSize[MAX_INDEX];
static unsigned char Present[MAX_INDEX];
static char * Filename[MAX_INDEX];
static char * Varname[MAX_INDEX];
static int LineNum[MAX_INDEX];
static int BytesMalloced = 0;

static int free_list[MAX_INDEX];

static int num_blocks = 0;

static int Initialized = 0;

static int LargestIndex = 0;

int out_of_memory = 0;

void mem_display_blocks();

void mem_init()
{
	int i;

	Initialized = 1;

	for (i=0; i<MAX_INDEX; i++ )
	{
		free_list[i] = i;
		MallocBase[i] = 0;
		MallocSize[i] = 0;
		Present[i] = 0;
		Filename[i] = NULL;
		Varname[i] = NULL;
		LineNum[i] = 0;
	}

	num_blocks = 0;
	LargestIndex = 0;

	atexit(mem_display_blocks);
}

void PrintInfo( int id )
{
	RT_LOGF(RT_LOGSERVERITY_INFO, "\tBlock '%s' created in %s, line %d.\n", Varname[id], Filename[id], LineNum[id]);
}


void * mem_malloc( unsigned int size, char * var, char * filename, int line, int fill_zero )
{
	int i, id;
	void *ptr;
	char * pc;

	if (Initialized==0)
		mem_init();

#if MEMSTATS
	{
		unsigned long	theFreeMem = 0;
	
		if (sMemStatsFileInitialized)
		{
			theFreeMem = FreeMem();
		
			fprintf(sMemStatsFile,
					"\n%9u bytes free before attempting: MALLOC %9u bytes.",
					theFreeMem,
					size);
		}
	}
#endif	// end of ifdef memstats

	if ( num_blocks >= MAX_INDEX )	{
		RT_LOG(RT_LOGSERVERITY_ASSERT, "\nMEM_OUT_OF_SLOTS: Not enough space in mem.c to hold all the mallocs.\n");
		RT_LOGF(RT_LOGSERVERITY_ASSERT, "\tBlock '%s' created in %s, line %d.\n", var, filename, line);
		RT_LOG(RT_LOGSERVERITY_HIGH, "MEM_OUT_OF_SLOTS" );
	}

	id = free_list[ num_blocks++ ];

	if (id > LargestIndex ) LargestIndex = id;

	if (id==-1)
	{
		RT_LOG(RT_LOGSERVERITY_ASSERT, "\nMEM_OUT_OF_SLOTS: Not enough space in mem.c to hold all the mallocs.\n");
		// RT_LOGF(RT_LOGSERVERITY_ASSERT, "\tBlock '%s' created in %s, line %d.\n", Varname[id], Filename[id], LineNum[id] );
		RT_LOG(RT_LOGSERVERITY_HIGH, "MEM_OUT_OF_SLOTS" );
	}

	ptr = malloc( size+CHECKSIZE );

	if (ptr==NULL)
	{
		out_of_memory = 1;
		RT_LOG(RT_LOGSERVERITY_ASSERT, "\nMEM_OUT_OF_MEMORY: Malloc returned NULL\n");
		RT_LOGF(RT_LOGSERVERITY_ASSERT, "\tBlock '%s' created in %s, line %d.\n", Varname[id], Filename[id], LineNum[id]);
	}

	MallocBase[id] = ptr;
	MallocSize[id] = size;
	Varname[id] = var;
	Filename[id] = filename;
	LineNum[id] = line;
	Present[id]    = 1;

	pc = (char *)ptr;

	BytesMalloced += size;

	for (i=0; i<CHECKSIZE; i++ )
		pc[size+i] = CHECKBYTE;

	if (fill_zero)
		memset( ptr, 0, size );

	return ptr;

}

int mem_find_id( void * buffer )
{
	int i;

	for (i=0; i<=LargestIndex; i++ )
	  if (Present[i]==1)
	    if (MallocBase[i] == buffer )
	      return i;

	// Didn't find id.
	return -1;
}

int mem_check_integrity( int block_number )
{
	int i, ErrorCount;
	ubyte * CheckData;

	CheckData = (ubyte *)((char *)MallocBase[block_number] + MallocSize[block_number]);

	ErrorCount = 0;
			
	for (i=0; i<CHECKSIZE; i++ )
		if (CheckData[i] != CHECKBYTE ) {
			ErrorCount++;
			RT_LOGF(RT_LOGSERVERITY_INFO, "OA: %p ", &CheckData[i]);
		}

	if (ErrorCount &&  (!out_of_memory))	{
		RT_LOG(RT_LOGSERVERITY_INFO, "\nMEM_OVERWRITE: Memory after the end of allocated block overwritten.\n");
		PrintInfo( block_number );
		RT_LOGF(RT_LOGSERVERITY_INFO, "\t%d/%d check bytes were overwritten.\n", ErrorCount, CHECKSIZE);
		Int3();
	}

	return ErrorCount;

}

void mem_free( void * buffer )
{
	int id;

	if (Initialized==0)
		mem_init();

#if MEMSTATS
	{
		unsigned long	theFreeMem = 0;
	
		if (sMemStatsFileInitialized)
		{
			theFreeMem = FreeMem();
		
			fprintf(sMemStatsFile,
					"\n%9u bytes free before attempting: FREE", theFreeMem);
		}
	}
#endif	// end of ifdef memstats

	if (buffer==NULL  &&  (!out_of_memory))
	{
		RT_LOG(RT_LOGSERVERITY_INFO, "\nMEM_FREE_NULL: An attempt was made to free the null pointer.\n");
		RT_LOG(RT_LOGSERVERITY_INFO, "MEM: Freeing the NULL pointer!");
		Int3();
		return;
	}

	id = mem_find_id( buffer );

	if (id==-1 &&  (!out_of_memory))
	{
		RT_LOG(RT_LOGSERVERITY_ASSERT, "\nMEM_FREE_NOMALLOC: An attempt was made to free a ptr that wasn't\nallocated with mem.h included.\n");
		RT_LOG(RT_LOGSERVERITY_MEDIUM,  "MEM: Freeing a non-malloced pointer!" );
		Int3();
		return;
	}
	
	mem_check_integrity( id );
	
	BytesMalloced -= MallocSize[id];

	free( buffer );

	Present[id] = 0;
	MallocBase[id] = 0;
	MallocSize[id] = 0;

	free_list[ --num_blocks ] = id;
}

void *mem_realloc(void * buffer, unsigned int size, char * var, char * filename, int line)
{
	void *newbuffer;
	int id;

	if (Initialized==0)
		mem_init();

	if (size == 0) {
		mem_free(buffer);
		newbuffer = NULL;
	} else if (buffer == NULL) {
		newbuffer = mem_malloc(size, var, filename, line, 0);
	} else {
		newbuffer = mem_malloc(size, var, filename, line, 0);
		if (newbuffer != NULL) {
			id = mem_find_id(buffer);
			if (MallocSize[id] < size)
				size = MallocSize[id];
			memcpy(newbuffer, buffer, size);
			mem_free(buffer);
		}
	}
	return newbuffer;
}

void mem_display_blocks()
{
	int i, numleft;

	if (Initialized==0) return;
	
#if MEMSTATS
	{	
		if (sMemStatsFileInitialized)
		{
			unsigned long	theFreeMem = 0;

			theFreeMem = FreeMem();
		
			fprintf(sMemStatsFile,
					"\n%9u bytes free before closing MEMSTATS file.", theFreeMem);
			fprintf(sMemStatsFile, "\nMemory Stats File Closed.");
			fclose(sMemStatsFile);
		}
	}
#endif	// end of ifdef memstats

	numleft = 0;
	for (i=0; i<=LargestIndex; i++ )
	{
		if (Present[i]==1 &&  (!out_of_memory))
		{
			numleft++;
			if (GameArg.DbgShowMemInfo)	{
				RT_LOG(RT_LOGSERVERITY_ASSERT, "\nMEM_LEAKAGE: Memory block has not been freed.\n");
				PrintInfo( i );
			}
			mem_free( (void *)MallocBase[i] );
		}
	}

	if (numleft &&  (!out_of_memory))
	{
		RT_LOGF(RT_LOGSERVERITY_MEDIUM,  "MEM: %d blocks were left allocated!\n", numleft );
	}

}

void mem_validate_heap()
{
	int i;
	
	for (i=0; i<LargestIndex; i++  )
		if (Present[i]==1 )
			mem_check_integrity( i );
}

void mem_print_all()
{
	PHYSFS_file * ef;
	int i, size = 0;

	ef = PHYSFSX_openWriteBuffered( "DESCENT.MEM" );
	
	for (i=0; i<LargestIndex; i++  )
		if (Present[i]==1 )	{
			size += MallocSize[i];
			PHYSFSX_printf( ef, "%12d bytes in %s declared in %s, line %d\n", MallocSize[i], Varname[i], Filename[i], LineNum[i]  );
		}
	PHYSFSX_printf( ef, "%d bytes (%d Kbytes) allocated.\n", size, size/1024 ); 
	PHYSFS_close(ef);
}

#else

static int Initialized = 0;
static unsigned int SmallestAddress = 0xFFFFFFF;
static unsigned int LargestAddress = 0x0;
static unsigned int BytesMalloced = 0;

void mem_display_blocks();

#define CHECKSIZE 16
#define CHECKBYTE 0xFC

void mem_init()
{
	Initialized = 1;

	SmallestAddress = 0xFFFFFFF;
	LargestAddress = 0x0;

	atexit(mem_display_blocks);
}

void * mem_malloc( unsigned int size, char * var, char * filename, int line, int fill_zero )
{
	size_t base;
	void *ptr;
	int * psize;

	if (Initialized==0)
		mem_init();

#if MEMSTATS
	{
		unsigned long	theFreeMem = 0;
	
		if (sMemStatsFileInitialized)
		{
			theFreeMem = FreeMem();
		
			fprintf(sMemStatsFile,
					"\n%9u bytes free before attempting: MALLOC %9u bytes.",
					theFreeMem,
					size);
		}
	}
#endif	// end of ifdef memstats

	if (size==0)	{
		RT_LOG(RT_LOGSERVERITY_INFO, "\nMEM_MALLOC_ZERO: Attempting to malloc 0 bytes.\n");
		RT_LOGF(RT_LOGSERVERITY_INFO, "\tVar %s, file %s, line %d.\n", var, filename, line);
		RT_LOG(RT_LOGSERVERITY_HIGH, "MEM_MALLOC_ZERO" );
		Int3();
	}

	ptr = malloc( size + CHECKSIZE );

	if (ptr==NULL)	{
		RT_LOG(RT_LOGSERVERITY_INFO, "\nMEM_OUT_OF_MEMORY: Malloc returned NULL\n");
		RT_LOGF(RT_LOGSERVERITY_INFO, "\tVar %s, file %s, line %d.\n", var, filename, line);
		RT_LOG(RT_LOGSERVERITY_HIGH, "MEM_OUT_OF_MEMORY" );
		Int3();
	}

	base = (size_t)ptr;
	if ( base < SmallestAddress ) SmallestAddress = base;
	if ( (base+size) > LargestAddress ) LargestAddress = base+size;


	psize = (int *)ptr;
	psize--;
	BytesMalloced += *psize;

	if (fill_zero)
		memset( ptr, 0, size );

	return ptr;
}

void mem_free( void * buffer )
{
	int * psize = (int *)buffer;
	psize--;

	if (Initialized==0)
		mem_init();

#if MEMSTATS
	{
		unsigned long	theFreeMem = 0;
	
		if (sMemStatsFileInitialized)
		{
			theFreeMem = FreeMem();
		
			fprintf(sMemStatsFile,
					"\n%9u bytes free before attempting: FREE", theFreeMem);
		}
	}
#endif	// end of ifdef memstats

	if (buffer==NULL)	{
		RT_LOG(RT_LOGSERVERITY_INFO, "\nMEM_FREE_NULL: An attempt was made to free the null pointer.\n");
		RT_LOG(RT_LOGSERVERITY_MEDIUM,  "MEM: Freeing the NULL pointer!" );
		Int3();
		return;
	}

	BytesMalloced -= *psize;

	free( buffer );
}

void mem_display_blocks()
{
	if (Initialized==0) return;

#if MEMSTATS
	{	
		if (sMemStatsFileInitialized)
		{
			unsigned long	theFreeMem = 0;

			theFreeMem = FreeMem();
		
			fprintf(sMemStatsFile,
					"\n%9u bytes free before closing MEMSTATS file.", theFreeMem);
			fprintf(sMemStatsFile, "\nMemory Stats File Closed.");
			fclose(sMemStatsFile);
		}
	}
#endif	// end of ifdef memstats

	if (BytesMalloced != 0 )	{
		RT_LOGF(RT_LOGSERVERITY_ASSERT, "\nMEM_LEAKAGE: %d bytes of memory have not been freed.\n", BytesMalloced);
	}

	if (GameArg.DbgShowMemInfo)	{
		RT_LOG(RT_LOGSERVERITY_HIGH, "\n\nMEMORY USAGE:\n");
		RT_LOGF(RT_LOGSERVERITY_HIGH, "  %u Kbytes dynamic data\n", (LargestAddress - SmallestAddress + 512) / 1024);
		RT_LOGF(RT_LOGSERVERITY_HIGH, "  %u Kbytes code/static data.\n", (SmallestAddress - (4 * 1024 * 1024) + 512) / 1024);
		RT_LOG(RT_LOGSERVERITY_HIGH, "  ---------------------------\n");
		RT_LOGF(RT_LOGSERVERITY_ASSERT, "  %u Kbytes required.\n", (LargestAddress - (4 * 1024 * 1024) + 512) / 1024);
	}
}

void mem_validate_heap()
{
}

void mem_print_all()
{
}

#endif
