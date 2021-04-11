
include "..\ControlRegisters.das"
include "..\ROMFuncDef.das"

org $0x20000

	mov		#0x40000, r0
	mov		r0, $Cort_BPL0_BaseAdd_H
	add		#4000, r0
	mov		r0, $Cort_BPL1_BaseAdd_H
	add		#4000, r0
	mov		r0, $Cort_BPL2_BaseAdd_H

	mov		#!7, r0
	mov.w	r0, $Cort_Control

	mov		#VBL, r0
	mov		r0, $0x8002E
	intmask	#%10000000

	mov		#CLUTData, r2
	mov		#Cort_Clut00, r3
	movi	#8, r4
	mov		#!FUNC_Memcpy, r0
	trap	#!0

	mov		#BPLData, r2
	mov		#0x40000, r3
	mov		#6000, r4
	mov		#!FUNC_Memcpy_L, r0
	trap	#!0

stop:
	bra	#!stop
	
VBL:
	rte

CLUTData:
	include "TestPCXPal.das"
BPLData:
	incbin "TestPCX.bin"
	
