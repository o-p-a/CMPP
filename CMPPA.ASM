IDEAL
;
;	プリンタ用紙節約プログラムＣＭＰＰ
;		　　アセンブラ部
;		 By Ken/ichiro(OPA)
;
ORGINAL_SEG	EQU	-1

INCLUDE "STDIO.ASH"
INCLUDE "CONDX.ASH"

	GROUP	CGROUP	TEXT

		PUBLIC	prnout_P2_
		PUBLIC	prnout_E2_
		PUBLIC	prnout_E4_
		EXTRN	bufprn_:BYTE
		EXTRN	_pputc_:NEAR

	SEGMENT	TEXT BYTE PUBLIC 'CODE'
	ASSUME	CS:TEXT

	PROC	prnout_P2_	NEAR
		push	cx
		push	dx
		push	si
		push	di

		mov	cx,ax
		and	cl,7
		shr	ax,3
		mov	si,48
		mul	si
		mov	si,ax
		add	si,aof bufprn_
		mov	dh,080h
		shr	dh,cl

		mov	cx,bx
		mov	[cs:dsbuf],ds
		mov	ax,seg bufprn_
		mov	[cs:xsbuf],ax
		mov	ds,ax

		test	cx,cx
@@l:
		jz	@@q

		mov	bx,si
		xor	di,di

		@FOR	AL,1,8
		shr	dl,1
		test	[di+bx],dh
		@IF	NZ
		or	dl,80h
		@ENDIF
		inc	di
		@NEXT
		mov	[cs:buf0],dl

		@FOR	AL,1,8
		shr	dl,1
		test	[di+bx],dh
		@IF	NZ
		or	dl,80h
		@ENDIF
		inc	di
		@NEXT
		mov	[cs:buf1],dl

		@FOR	AL,1,8
		shr	dl,1
		test	[di+bx],dh
		@IF	NZ
		or	dl,80h
		@ENDIF
		inc	di
		@NEXT
		mov	[cs:buf2],dl

		mov	ds,[cs:dsbuf]
		mov	al,[cs:buf0]
		call	_pputc_
		mov	al,[cs:buf1]
		call	_pputc_
		mov	al,[cs:buf2]
		call	_pputc_
		mov	ds,[cs:xsbuf]

		shr	dh,1
		@IF	C
		mov	dh,080h
		add	si,48
		@ENDIF

		dec	cx
		jmp	@@l
@@q:
		mov	ds,[cs:dsbuf]
		pop	di
		pop	si
		pop	dx
		pop	cx
		RET
	ENDP

	PROC	prnout_E2_	NEAR
		push	cx
		push	dx
		push	si
		push	di

		mov	cx,ax
		and	cl,7
		shr	ax,3
		mov	si,48
		mul	si
		mov	si,ax
		add	si,aof bufprn_
		mov	dh,080h
		shr	dh,cl

		mov	cx,bx
		mov	[cs:dsbuf],ds
		mov	ax,seg bufprn_
		mov	[cs:xsbuf],ax
		mov	ds,ax

		test	cx,cx
@@l:
		jz	@@q

		mov	bx,si
		xor	di,di

		@FOR	AL,1,8
		shl	dl,1
		test	[di+bx],dh
		@IF	NZ
		inc	dl
		@ENDIF
		inc	di
		@NEXT
		mov	[cs:buf0],dl

		@FOR	AL,1,8
		shl	dl,1
		test	[di+bx],dh
		@IF	NZ
		inc	dl
		@ENDIF
		inc	di
		@NEXT
		mov	[cs:buf1],dl

		@FOR	AL,1,8
		shl	dl,1
		test	[di+bx],dh
		@IF	NZ
		inc	dl
		@ENDIF
		inc	di
		@NEXT
		mov	[cs:buf2],dl

		mov	ds,[cs:dsbuf]
		mov	al,[cs:buf0]
		call	_pputc_
		mov	al,[cs:buf1]
		call	_pputc_
		mov	al,[cs:buf2]
		call	_pputc_
		mov	ds,[cs:xsbuf]

		shr	dh,1
		@IF	C
		mov	dh,080h
		add	si,48
		@ENDIF

		dec	cx
		jmp	@@l
@@q:
		mov	ds,[cs:dsbuf]
		pop	di
		pop	si
		pop	dx
		pop	cx
		RET
	ENDP

	PROC	prnout_E4_	NEAR
		push	cx
		push	dx
		push	si
		push	di

		mov	cx,ax
		and	cl,7
		shr	ax,3
		mov	si,48
		mul	si
		mov	si,ax
		add	si,aof bufprn_
		mov	dh,080h
		shr	dh,cl

		mov	cx,bx
		mov	[cs:dsbuf],ds
		mov	ax,seg bufprn_
		mov	[cs:xsbuf],ax
		mov	ds,ax

		test	cx,cx
@@l:
		jz	@@q

		mov	bx,si
		xor	di,di

		@FOR	AL,1,8
		shl	dl,1
		test	[di+bx],dh
		@IF	NZ
		inc	dl
		@ENDIF
		inc	di
		@NEXT
		mov	[cs:buf0],dl

		@FOR	AL,1,8
		shl	dl,1
		test	[di+bx],dh
		@IF	NZ
		inc	dl
		@ENDIF
		inc	di
		@NEXT
		mov	[cs:buf1],dl

		@FOR	AL,1,8
		shl	dl,1
		test	[di+bx],dh
		@IF	NZ
		inc	dl
		@ENDIF
		inc	di
		@NEXT
		mov	[cs:buf2],dl

		@FOR	AL,1,8
		shl	dl,1
		test	[di+bx],dh
		@IF	NZ
		inc	dl
		@ENDIF
		inc	di
		@NEXT
		mov	[cs:buf3],dl

		@FOR	AL,1,8
		shl	dl,1
		test	[di+bx],dh
		@IF	NZ
		inc	dl
		@ENDIF
		inc	di
		@NEXT
		mov	[cs:buf4],dl

		@FOR	AL,1,8
		shl	dl,1
		test	[di+bx],dh
		@IF	NZ
		inc	dl
		@ENDIF
		inc	di
		@NEXT
		mov	[cs:buf5],dl

		mov	ds,[cs:dsbuf]
		mov	al,[cs:buf0]
		call	_pputc_
		mov	al,[cs:buf1]
		call	_pputc_
		mov	al,[cs:buf2]
		call	_pputc_
		mov	al,[cs:buf3]
		call	_pputc_
		mov	al,[cs:buf4]
		call	_pputc_
		mov	al,[cs:buf5]
		call	_pputc_
		mov	ds,[cs:xsbuf]

		shr	dh,1
		@IF	C
		mov	dh,080h
		add	si,48
		@ENDIF

		dec	cx
		jmp	@@l
@@q:
		mov	ds,[cs:dsbuf]
		pop	di
		pop	si
		pop	dx
		pop	cx
		RET
	ENDP

dsbuf	dw	?
xsbuf	dw	?
buf0	db	?
buf1	db	?
buf2	db	?
buf3	db	?
buf4	db	?
buf5	db	?

	ENDS
	END
