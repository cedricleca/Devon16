
include "ControlRegisters.das"

org $0x20000

define	Var1 0x85000

	mov		#!2, r0
	mov.w	r0, $Cort_BPL0_HStart
	mov.w	r0, $Cort_BPL1_HStart
	mov.w	r0, $Cort_BPL2_HStart
	mov.w	r0, $Cort_BPL3_HStart

	movi	#16, r0
	mov.w	r0, $Cort_BPL0_Stride
	mov.w	r0, $Cort_BPL1_Stride
	mov.w	r0, $Cort_BPL2_Stride
	mov.w	r0, $Cort_BPL3_Stride

	mov		#0x40000, r0
	mov		r0, $Cort_BPL0_BaseAdd_H
	add		#7400, r0
	mov		r0, $Cort_BPL1_BaseAdd_H
	add		#7400, r0
	mov		r0, $Cort_BPL2_BaseAdd_H
	add		#7400, r0
	mov		r0, $Cort_BPL3_BaseAdd_H

	mov		#!0, r2
	mov		r2, $Var1

	mov.w	#!-1, r2
	mov		#!0, r3

	mov		#!0, r6
	mov		#0x40000, r0
	bsr		#FillLine
	add		#!1, r6
	bsr		#FillLine
	add		#!1, r6
	bsr		#FillLine
	add		#!1, r6
	bsr		#FillLine

;	mov		#!1, r0
;	mov.w	r0, $Cort_BPLEnable
	mov		#VBL, r0
	mov		r0, $0x8002E
	intmask	#%10000000

	; sound test
;	mov		#40000, r0
;	mov		r0, $JKev_R_A_OSC0_OSCSTEP_HI
;	mov		r0, $JKev_L_A_OSC0_OSCSTEP_HI
;	mov		#40080, r0
;	mov		r0, $JKev_R_A_OSC1_OSCSTEP_HI
;	mov		r0, $JKev_L_A_OSC1_OSCSTEP_HI
;	mov		#0x1ff, r0
;	mov.w	r0, $JKev_R_A_OSC0_WAVEAMPLITUDE
;	mov.w	r0, $JKev_L_A_OSC0_WAVEAMPLITUDE
;	mov		#0x1f0, r0
;	mov.w	r0, $JKev_R_A_OSC1_WAVEAMPLITUDE
;	mov.w	r0, $JKev_L_A_OSC1_WAVEAMPLITUDE
;	movi	#0x50, r0
;	mov.w	r0, $JKev_R_A_PREMODOFFSET
;	mov.w	r0, $JKev_L_A_PREMODOFFSET
;	mov		#0x00f8, r0 ; InvM8 Damp4 Drive4
;	mov.w	r0, $JKev_R_A_FILTER
;	mov.w	r0, $JKev_L_A_FILTER

stop:
	bra	#!stop
	
FillLine:
	mov		#!0, r5
startline:
	movi	#37, r1
	mov		r5, r4
	lsri	#!4, r4
loop2:
	btst	r6, r4
	bra.z	#!cond1
	mov.w	r2, (r0)+
	bra		#!lookpback
cond1:
	mov.w	r3, (r0)+
lookpback:
	add		#!1, r4
	sub		#!1, r1
	bra.p	#loop2
continue:
	add		#!1, r5
	cmp		#200, r5
	bra.pn	#startline
	rts

VBL:
	mov		$Var1, r0
	add		#!1, r0
	and		#0xFF, r0
	bra.pn	#VBLNoLoop
	movi	#-16, r1
	add		r1, $Cort_BPL0_BaseAdd_H
	add		r1, $Cort_BPL1_BaseAdd_H
	add		r1, $Cort_BPL2_BaseAdd_H
	add		r1, $Cort_BPL3_BaseAdd_H
	add		r1, $Cort_BPL0_CurAdd_H
	add		r1, $Cort_BPL1_CurAdd_H
	add		r1, $Cort_BPL2_CurAdd_H
	add		r1, $Cort_BPL3_CurAdd_H
VBLNoLoop:
	mov		r0, $Var1
	mov		r0, r1
	and		#0xF, r0
	mov.w	r0, $Cort_BPL0_Shift
	mov.w	r0, $Cort_BPL1_Shift
	mov.w	r0, $Cort_BPL2_Shift
	mov.w	r0, $Cort_BPL3_Shift
	bra.z	#!VBLScroll
	rte
VBLScroll:
	mov		#!1, r1
	add		r1, $Cort_BPL0_BaseAdd_H
	add		r1, $Cort_BPL1_BaseAdd_H
	add		r1, $Cort_BPL2_BaseAdd_H
	add		r1, $Cort_BPL3_BaseAdd_H
	add		r1, $Cort_BPL0_CurAdd_H
	add		r1, $Cort_BPL1_CurAdd_H
	add		r1, $Cort_BPL2_CurAdd_H
	add		r1, $Cort_BPL3_CurAdd_H
	rte


