;THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
;SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
;END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
;ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
;IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
;SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
;FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
;CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
;AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
;COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
;
; $Source: /cvsroot/dxx-rebirth/d1x-rebirth/texmap/tmapfade.asm,v $
; $Revision: 1.1.1.1 $
; $Author: zicodxx $
; $Date: 2006/03/17 19:46:06 $
;
; .
;
; $Log: tmapfade.asm,v $
; Revision 1.1.1.1  2006/03/17 19:46:06  zicodxx
; initial import
;
; Revision 1.1.1.1  1999/06/14 22:13:53  donut
; Import of d1x 1.37 source.
;
; Revision 1.6  1995/02/20  18:23:01  john
; Put all the externs in the assembly modules into tmap_inc.asm.
; Also, moved all the C versions of the inner loops into a new module,
; scanline.c.
;
; Revision 1.5  1995/02/20  17:09:15  john
; Added code so that you can build the tmapper with no assembly!
;
; Revision 1.4  1994/12/02  23:29:36  mike
; change jb/ja to jl/jg.
;
; Revision 1.3  1994/11/30  00:57:36  mike
; *** empty log message ***
;
; Revision 1.2  1994/10/06  18:38:49  john
; Added the ability to fade a scanline by calling gr_upoly_tmap
; with Gr_scanline_darkening_level with a value < MAX_FADE_LEVELS.
;
; Revision 1.1  1994/10/06  18:04:42  john
; Initial revision
;
;
[BITS 32]
global  _asm_tmap_scanline_shaded
global  asm_tmap_scanline_shaded

[SECTION .data]

%include        "tmap_inc.asm"
_loop_count		dd	0

[SECTION .text]

; --------------------------------------------------------------------------------------------------
; Enter:
;	_xleft	fixed point left x coordinate
;	_xright	fixed point right x coordinate
;	_y	fixed point y coordinate
;**;	_pixptr	address of source pixel map

;   for (x = (int) xleft; x <= (int) xright; x++) {
;      _setcolor(read_pixel_from_tmap(srcb,((int) (u/z)) & 63,((int) (v/z)) & 63));
;      _setpixel(x,y);
;
;      z += dz_dx;
;   }
align 4

_asm_tmap_scanline_shaded:
asm_tmap_scanline_shaded:
;        push fs
	pusha

;        mov     fs, [_gr_fade_table_selector] ; DPH: No selectors in windows

; Setup for loop:	_loop_count  iterations = (int) xright - (int) xleft
;	edi	initial row pointer = y*320+x

; set edi = address of first pixel to modify
	mov	edi,[_fx_y]
 cmp edi,_window_bottom
 jae near _none_to_do

	imul	edi,[_bytes_per_row]
	mov	eax,[_fx_xleft]
	test	eax,eax
	jns	eax_ok
	sub	eax,eax
eax_ok:
	add	edi,eax
	add	edi,[_write_buffer]

; set _loop_count = # of iterations
	mov	eax,[_fx_xright]

	cmp	eax,[_window_right]
	jl	eax_ok1
	mov	eax,[_window_right]
eax_ok1:	cmp	eax,[_window_left]
	jg	eax_ok2
	mov	eax,[_window_left]
eax_ok2:
	mov	ebx,[_fx_xleft]
	sub	eax,ebx
	js	near _none_to_do
	cmp	eax,[_window_width]
	jbe	_ok_to_do
	mov	eax,[_window_width]
_ok_to_do:
	mov	[_loop_count], eax

	mov	ecx, 0
	mov	ch,[_tmap_flat_shade_value]
	and	ch, 31

;_size = (_end1 - _start1)/num_iters
	mov	eax,num_iters-1
	sub	eax,[_loop_count]
	jns	j_eax_ok1
	inc	eax	; sort of a hack, but we can get -1 here and want to be graceful
	jns	j_eax_ok1	; if we jump, we had -1, which is kind of ok, if not, we int 3
	int	3	; oops, going to jump behind _start1, very bad...
	sub	eax,eax	; ok to continue
j_eax_ok1:	imul	eax,eax,(_end1 - _start1)/num_iters
	add	eax, _start1        ;originally offset _start1
	jmp	eax

	align	4
_start1:

%rep num_iters
	mov	cl, [edi]		; get pixel
        mov     al, [_gr_fade_table + ecx]            ; darken pixel
	mov	[edi], al		; write pixel
	inc	edi			; goto next pixel
%endrep
_end1:

_none_to_do:	popa
;                pop     fs
	ret

