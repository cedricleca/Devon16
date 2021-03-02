
include "ControlRegisters.das"

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
	mov		#8, r4
	mov		#!0, r0	; Memcpy
	trap	#!0

	mov		#BPLData, r2
	mov		#0x40000, r3
	mov		#6000, r4
	mov		#!1, r0	; Memcpy_L
	trap	#!0

stop:
	bra	#!stop
	
VBL:
	rte

CLUTData:
	include "TestPCXPal.das"
BPLData:
	incbin "TestPCX.bin"
