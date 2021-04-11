
include "..\ControlRegisters.das"
include "..\ROMFuncDef.das"

org $0x20000

	mov		#0x40000, r0
	mov		#3880, r1
	mov		r0, $Cort_BPL4_BaseAdd_H
	add		r1, r0
	mov		r0, $Cort_BPL5_BaseAdd_H
	add		r1, r0
	mov		r0, $Cort_BPL6_BaseAdd_H
	add		r1, r0
	mov		r0, $Cort_BPL7_BaseAdd_H

	mov		#0xF0, r0
	mov.w	r0, $Cort_Control

	mov		#VBL, r0
	mov		r0, $0x8002E
	intmask	#%10000000

	mov		#CLUTData, r2
	mov		#Cort_Clut10, r3
	movi	#16, r4
	mov		#!FUNC_Memcpy, r0
	trap	#!0

	mov		#BPLData, r2
	mov		#0x40000, r3
	mov		#7760, r4
	mov		#!FUNC_Memcpy_L, r0
	trap	#!0

stop:
	bra	#!stop
	
VBL:
	rte

CLUTData:
	include "D16PCXPal.das"
BPLData:
	incbin "D16PCX.bin"
	
