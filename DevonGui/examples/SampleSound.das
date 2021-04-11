
include "..\ControlRegisters.das"
include "..\ROMFuncDef.das"

org $0x20000

define	Var1			0x85000
define	SampleCursor	0x85004

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
	mov		#7400, r1
	mov		r0, $Cort_BPL0_BaseAdd_H
	add		r1, r0
	mov		r0, $Cort_BPL1_BaseAdd_H
	add		r1, r0
	mov		r0, $Cort_BPL2_BaseAdd_H
	add		r1, r0
	mov		r0, $Cort_BPL3_BaseAdd_H
	add		r1, r0

	mov		#3880, r1
	mov		r0, $Cort_BPL4_BaseAdd_H
	add		r1, r0
	mov		r0, $Cort_BPL5_BaseAdd_H
	add		r1, r0
	mov		r0, $Cort_BPL6_BaseAdd_H
	add		r1, r0
	mov		r0, $Cort_BPL7_BaseAdd_H

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

	mov		#0xF, r0
	mov.w	r0, $Cort_Control
		
	mov		#VBL, r0
	mov		r0, $0x8002E ; VBL

	mov		#SampleInterrupt, r0
	mov		r0, $0x80028 ; TimerA interrput routine address
	
	mov		#!3, r0 ; Timers : RunA, LoopA
	mov.w	r0, $TimersControl
	
	mov		#1461, r0 ; -> 16MHz / 11025Hz = 1461
	mov		r0, $TimerA_Base_H
	mov		r0, $TimerA_Value_H

	mov		#SampleData, r0
	add		r0, r0 ; *2 because we use movb to get 8bit samples
	mov		r0, $SampleCursor
	
	intmask	#%10010000 ; VBL + TimerA

	mov		#!0, r0 ; no oscillation
	mov		r0, $JKev_R_A_OSC0_OSCSTEP_HI
	mov		r0, $JKev_L_A_OSC0_OSCSTEP_HI
	mov		r0, $JKev_R_A_OSC1_OSCSTEP_HI
	mov		r0, $JKev_L_A_OSC1_OSCSTEP_HI
	mov		r0, $JKev_R_B_OSC0_OSCSTEP_HI
	mov		r0, $JKev_L_B_OSC0_OSCSTEP_HI
	mov		r0, $JKev_R_B_OSC1_OSCSTEP_HI
	mov		r0, $JKev_L_B_OSC1_OSCSTEP_HI
	mov		#0xff, r0
	mov.w	r0, $JKev_R_A_OSC0_WAVEAMPLITUDE
	mov.w	r0, $JKev_L_A_OSC0_WAVEAMPLITUDE
	mov.w	r0, $JKev_R_B_OSC0_WAVEAMPLITUDE
	mov.w	r0, $JKev_L_B_OSC0_WAVEAMPLITUDE
	mov		#0x00fa, r0 ; InvM8 Damp4 Drive4
	mov.w	r0, $JKev_R_A_FILTER
	mov.w	r0, $JKev_L_A_FILTER
	mov.w	r0, $JKev_R_B_FILTER
	mov.w	r0, $JKev_L_B_FILTER

stop:
	bra	#!stop
	
SampleInterrupt:
	mov		$SampleCursor, r0	
	movb	(r0)+, r1 ; get the current sample and advance to the next
	ext8	r1 ; recover the sign of the sample
	mov.w	r1, $JKev_R_A_PREMODOFFSET ; dispatch the sample to all voices
	mov.w	r1, $JKev_L_A_PREMODOFFSET
	mov.w	r1, $JKev_R_B_PREMODOFFSET
	mov.w	r1, $JKev_L_B_PREMODOFFSET
	
	mov		#SampleData_End, r1 ; check if we reached the end of the sample data
	add		r1, r1
	cmp		r1, r0
	bra.np	#SampleInterrupt_NoLoop
	
	mov		#SampleData, r0 ; loop back to the start of the data
	add		r0, r0
	
SampleInterrupt_NoLoop:
	mov		r0, $SampleCursor
	rte
	
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
	add		#!1, r5
	cmp		#200, r5
	bra.pn	#startline
	rts

VBL:
	ssave	#%11 ; save r0 and r1
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

	sload	#%11
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
	
	sload	#%11
	rte

SampleData:
	incbin "HumanRace_8b_11KHz.bin"
SampleData_End:
	
