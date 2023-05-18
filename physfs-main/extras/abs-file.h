/*
 * stdio/physfs abstraction layer 2003-04-02
 *
 * Adam D. Moss <adam@gimp.org> <aspirin@icculus.org>
 *
 * These wrapper macros and functions are designed to allow a program
 * to perform file I/O with identical semantics and syntax regardless
 * of whether PhysicsFS is being used or not.
 */
#ifndef _ABS_FILE_H
#define _ABS_FILE_H
/*
PLEASE NOTE: This license applies to abs-file.h ONLY (to make it clear that
you may embed this wrapper code within commercial software); PhysicsFS itself
is (at the time of writing) released under a different license with
additional restrictions.

Copyright (C) 2002-2003 Adam D. Moss (the "Author").  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is fur-
nished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FIT-
NESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CON-
NECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the Author of the
Software shall not be used in advertising or otherwise to promote the sale,
use or other dealings in this Software without prior written authorization
from the Author.
*/

#include <stdlib.h>
#include <stdio.h>

/*
 * API:
 *
 * Macro/function       use like stdio equivalent...
 * --------------       ----------------------------
 * MY_FILETYPE          FILE
 * MY_OPEN_FOR_READ     fopen(..., "rb")
 * MY_READ              fread(...)
 * MY_CLOSE             fclose(...)
 * MY_GETC              fgetc(...)
 * MY_GETS              fgets(...)
 * MY_ATEOF             feof(...)
 * MY_TELL              ftell(...)
 * MY_SEEK              fseek(..., SEEK_SET)
 * MY_REWIND            rewind(...)
 * MY_SETBUFFER         (not a standard for stdio, does nothing there)
 */

/*
 * Important DEFINEs:
 *   It is important to define these consistantly across the various
 *   compilation modules of your program if you wish to exchange file
 *   handles between them.
 *
 *   USE_PHYSFS: Define USE_PHYSFS if PhysicsFS is being used; note that if
 *     you do intend to use PhysicsFS then you will still need to initialize
 *     PhysicsFS yourself and set up its search-paths.
 *
 * Optional DEFINEs:
 *
 *   PHYSFS_DEFAULT_READ_BUFFER <bytes>: If set then abs-file.h sets the
 *     PhysicsFS buffer size to this value whenever you open a file.  You
 *     may over-ride this on a per-filehandle basis by using the
 *     MY_SETBUFFER() macro (which simply does nothing when not using
 *     PhysicsFS).  If you have not defined this value explicitly then
 *     abs-file.h will default to the same default buffer size as used by
 *     stdio if it can be determined, or 8192 bytes otherwise.
 */
#ifndef PHYSFS_DEFAULT_READ_BUFFER
#ifdef BUFSIZ
#define PHYSFS_DEFAULT_READ_BUFFER BUFSIZ
#else
#define PHYSFS_DEFAULT_READ_BUFFER 8192
#endif
#endif

#ifdef USE_PHYSFS

#include <physfs.h>
#define MY_FILETYPE PHYSFS_File
#define MY_SETBUFFER(fp,size) PHYSFS_setBuffer(fp,size)
#define MY_READ(p,s,n,fp) PHYSFS_read(fp,p,s,n)
#if PHYSFS_DEFAULT_READ_BUFFER
static MY_FILETYPE* MY_OPEN_FOR_READ(const char *const filename)
{
  MY_FILETYPE *const file = PHYSFS_openRead(filename);
  if (file) {
    MY_SETBUFFER(file, PHYSFS_DEFAULT_READ_BUFFER);
  }
  return file;
}
#else
#define MY_OPEN_FOR_READ(fn) PHYSFS_openRead(fn)
#endif
static int MY_GETC(MY_FILETYPE *const fp) {
  unsigned char c;
  /*if (PHYSFS_eof(fp)) {
    return EOF;
  }
  MY_READ(&c, 1, 1, fp);*/
  if (MY_READ(&c, 1, 1, fp) != 1) {
    return EOF;
  }
  return c;
}
static char * MY_GETS(char * const str, const int size, 
                      MY_FILETYPE *const fp) {
  int i = 0;
  int c;
  do {
    if (i == size-1) {
      break;
    }
    c = MY_GETC(fp);
    if (c == EOF) {
      break;
    }
    str[i++] = c;
  } while (c != '\0' && 
      c != -1 && 
      c != '\n');
  str[i] = '\0';
  if (i == 0) {
    return NULL;
  }
  return str;
}
#define MY_CLOSE(fp) PHYSFS_close(fp)
#define MY_ATEOF(fp) PHYSFS_eof(fp)
#define MY_TELL(fp) PHYSFS_tell(fp)
#define MY_SEEK(fp,o) PHYSFS_seek(fp,o)
#define MY_REWIND(fp) MY_SEEK(fp,0)

#else

#define MY_FILETYPE FILE
#define MY_READ(p,s,n,fp) fread(p,s,n,fp)
#define MY_OPEN_FOR_READ(n) fopen(n, "rb")
#define MY_GETC(fp) fgetc(fp)
#define MY_GETS(str,size,fp) fgets(str,size,fp)
#define MY_CLOSE(fp) fclose(fp)
#define MY_ATEOF(fp) feof(fp)
#define MY_TELL(fp) ftell(fp)
#define MY_SEEK(fp,o) fseek(fp,o, SEEK_SET)
#define MY_REWIND(fp) rewind(fp)
/*static void MY_SETBUFFER(const MY_FILETYPE *const file, const int num) { }*/
#define MY_SETBUFFER(fp,size)
#endif

#endif
