define EntryPoint 0x100
define CartSize 0x10002
define CartBase 0x20000

romexport

include "SystemRam.das"
include "ControlRegisters.das"

;VectorTable
org $0x0
ResetPC:			long EntryPoint
ResetStack:			long DefaultStack
BusError:			long DefaultBusError
BadInstruction:		long DefaultInstructionError
DivideByZero:		long DefaultZeroDivideError

org $0x20
Interrupt0:			long DefaultInterrupt_DSKDMA
Interrupt1:			long DefaultInterrupt_MTUB
Interrupt2:			long DefaultInterrupt_MTUA
Interrupt3:			long DefaultInterrupt_TimerB
Interrupt4:			long DefaultInterrupt_TimerA
Interrupt5:			long DefaultInterrupt_GFXPos
Interrupt6:			long DefaultInterrupt_HBlank
Interrupt7:			long DefaultInterrupt_VBlank

org $0x40
Trap0:				long DefaultTrap0
Trap1:				long DefaultTrap1
Trap2:				long DefaultTrap2
Trap3:				long DefaultTrap3
Trap4:				long DefaultTrap4
Trap5:				long DefaultTrap5
Trap6:				long DefaultTrap6
Trap7:				long DefaultTrap7
Trap8:				long DefaultTrap8
Trap9:				long DefaultTrap9
TrapA:				long DefaultTrapA
TrapB:				long DefaultTrapB
TrapC:				long DefaultTrapC
TrapD:				long DefaultTrapD
TrapE:				long DefaultTrapE
TrapF:				long DefaultTrapF

Trap0_FUNC_Table:
	long	FUNC_Memcpy
	long	FUNC_Memcpy_L
	long	FUNC_ResetCorticoRegs
	long	FUNC_ResetDefaultClut
	long	FUNC_ResetDefaultFont
	long	FUNC_DrawChar

DefaultClut:
	word	0x000, 0xAAA, 0xFFF, 0xC33, 0xE79, 0x543, 0xA62, 0xF93, 0xFE6, 0x244, 0x481, 0xAC2, 0x123, 0x058, 0x3AF, 0xBDE
	word	0x000, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF

org EntryPoint
	mov		#RAM_VBase, r3
	mov		#!0, r2
	mov		#0x80, r4
	jsr		#FUNC_Memcpy_L

	vbase	#RAM_VBase
	intmask	#%00000000		; enable VBL only

	mov		#!0x0, r0
	mov.w	r0, $RAM_Writer_YOffset
	mov.w	r0, $RAM_Writer_XOffset
	movi	#40, r0
	mov.w	r0, $RAM_Writer_Width

	jsr		#FUNC_ResetCorticoRegs	; Reset Cortico registers
	jsr		#FUNC_ResetDefaultClut	; Reset Clut
	jsr		#FUNC_ResetDefaultFont	; Restore original font data

	mov.w	$CartSize, r0	; get the Catridge size
	ext16	r0
	add		#!0, r0			; test it
	jmp.np	#CartBase		; if present -> jmp


	mov		#NoCartMsg, r2
	jsr		#FUNC_Print
	halt

DefaultBusError:
	MOV #!0, r0
	HALT

DefaultInstructionError:
	MOV #!1, r0
	HALT

DefaultZeroDivideError:
	MOV #!2, r0
	HALT

DefaultInterrupt_VBlank:
	mov #0xabcde, r5
	rte

DefaultInterrupt_HBlank:
	MOV #!1, r0
	RTE
DefaultInterrupt_GFXPos:
	MOV #!2, r0
	RTE
DefaultInterrupt_TimerA:
	MOV #!3, r0
	RTE
DefaultInterrupt_TimerB:
	MOV #!4, r0
	RTE
DefaultInterrupt_MTUA:
	MOV #!5, r0
	RTE
DefaultInterrupt_MTUB:
	MOV #!6, r0
	RTE
DefaultInterrupt_DSKDMA:
	MOV #!7, r0
	RTE

DefaultTrap0:
	; r0 = function Id
	; r1 is wiped
	mov	#Trap0_FUNC_Table, r1
	add	r0, r0
	add r0, r1
	jsr (r1)
	rte

DefaultTrap1:
	MOV #!0x1, r0
	RTE
DefaultTrap2:
	MOV #!0x2, r0
	RTE
DefaultTrap3:
	MOV #!0x3, r0
	RTE
DefaultTrap4:
	MOV #!0x4, r0
	RTE
DefaultTrap5:
	MOV #!0x5, r0
	RTE
DefaultTrap6:
	MOV #!0x6, r0
	RTE
DefaultTrap7:
	MOV #!0x7, r0
	RTE
DefaultTrap8:
	MOVI #0x8, r0
	RTE
DefaultTrap9:
	MOVI #0x9, r0
	RTE
DefaultTrapA:
	MOVI #0xA, r0
	RTE
DefaultTrapB:
	MOVI #0xB, r0
	RTE
DefaultTrapC:
	MOVI #0xC, r0
	RTE
DefaultTrapD:
	MOVI #0xD, r0
	RTE
DefaultTrapE:
	MOVI #0xE, r0
	RTE
DefaultTrapF:
	MOVI #0xf, r0
	RTE

FUNC_ResetCorticoRegs:
	mov		#Cort_H, r0
	add		#!4, r0
	movi	#0xF, r1
	mov.w	r1, (r0)+		; BPLEnable 
	
	mov		#0x40000, r1
	mov		r1, (r0)+		; BPL0_Base
	add		#!2, r0
	movi	#16, r2
	mov.w	r2, (r0)+		; BPL0_Shift
	mov		#!0, r3
	mov.w	r3, (r0)+		; BPL0_Stride
	movi	#20, r4
	mov.w	r4, (r0)+		; Cort_VStart
	mov		#220, r5
	mov.w	r5, (r0)+		; Cort_VEnd
	movi	#23, r6
	ssave	#%01000000
	mov		#!3, r6
	mov.w	r6, (r0)+		; Cort_HStart
	sswap	#%01000000
	mov.w	r6, (r0)+		; Cort_HEnd

	add		#0xFA0, r1		; BPL1
	mov		r1, (r0)+		
	add		#!2, r0
	mov.w	r2, (r0)+		; BPL1_Shift
	mov.w	r3, (r0)+		; BPL1_Stride
	mov.w	r4, (r0)+		; Cort_VStart
	mov.w	r5, (r0)+		; Cort_VEnd
	sswap	#%01000000
	mov.w	r6, (r0)+		; Cort_HStart
	sswap	#%01000000
	mov.w	r6, (r0)+		; Cort_HEnd

	add		#0xFA0, r1		; BPL2
	mov		r1, (r0)+		
	add		#!2, r0
	mov.w	r2, (r0)+		; BPL2_Shift
	mov.w	r3, (r0)+		; BPL2_Stride
	mov.w	r4, (r0)+		; Cort_VStart
	mov.w	r5, (r0)+		; Cort_VEnd
	sswap	#%01000000
	mov.w	r6, (r0)+		; Cort_HStart
	sswap	#%01000000
	mov.w	r6, (r0)+		; Cort_HEnd

	add		#0xFA0, r1		; BPL3
	mov		r1, (r0)+		
	add		#!2, r0
	mov.w	r2, (r0)+		; BPL3_Shift
	mov.w	r3, (r0)+		; BPL3_Stride
	mov.w	r4, (r0)+		; Cort_VStart
	mov.w	r5, (r0)+		; Cort_VEnd
	sswap	#%01000000
	mov.w	r6, (r0)+		; Cort_HStart
	sswap	#%01000000
	mov.w	r6, (r0)+		; Cort_HEnd

	rts

; r2 -> r3, Wsize = r4
FUNC_Memcpy:
	mov.w	(r2)+, r5
	mov.w	r5, (r3)+
	sub		#!1, r4
	bra.pn	#!FUNC_Memcpy
	rts

FUNC_Memcpy_L:
	mov		(r2)+, r5
	mov		r5, (r3)+
	sub		#!1, r4
	bra.pn	#!FUNC_Memcpy_L
	rts

FUNC_ResetDefaultClut:
	mov		#Cort_Clut00, r3
	mov		#DefaultClut, r2
	movi	#16, r4
	jsr		#FUNC_Memcpy_L
	rts

FUNC_ResetDefaultFont:
	mov		#RAM_Font, r3
	mov		#DefaultFontData, r2
	mov		#423, r4
	jsr		#FUNC_Memcpy
	rts

FUNC_DrawChar:
	mov		r2, r0
	mov.w	$Cort_BPL0_HEnd, r5
	mov.w	$Cort_BPL0_HStart, r6
	sub		r6, r5
	mov.w	$Cort_BPL0_Stride, r6
	ext16	r6
	add		r6, r5
	add		r5, r5

	mov		#RAM_Font, r6
	
	mov		$Cort_BPL0_BaseAdd_H, r2
	add		r2, r2
	mov.w	$RAM_Writer_YOffset, r3
	add		r3, r2
	mov.w	$RAM_Writer_XOffset, r3
	add		r3, r2
	
	sub		#'!', r0
	mov		r0, r1
	lsri	#1, r1
	mul		#9, r1
	add		r6, r1
	add		r1, r1
	and		#!1, r0
	add		r0, r1

	movb	(r1), r4
	movb	r4, (r2)
	add		r5, r2
	add		#!2, r1

	movb	(r1), r4
	movb	r4, (r2)
	add		r5, r2
	add		#!2, r1

	movb	(r1), r4
	movb	r4, (r2)
	add		r5, r2
	add		#!2, r1

	movb	(r1), r4
	movb	r4, (r2)
	add		r5, r2
	add		#!2, r1

	movb	(r1), r4
	movb	r4, (r2)
	add		r5, r2
	add		#!2, r1

	movb	(r1), r4
	movb	r4, (r2)
	add		r5, r2
	add		#!2, r1

	movb	(r1), r4
	movb	r4, (r2)
	add		r5, r2
	add		#!2, r1

	movb	(r1), r4
	movb	r4, (r2)
	add		r5, r2
	add		#!2, r1

	movb	(r1), r4
	movb	r4, (r2)

	mov.w	$RAM_Writer_XOffset, r2
	add		#!1, r2
	mov.w	$RAM_Writer_Width, r3
	cmp		r3, r2
	bra.n	#DrawChar_NoWrap
	
	mov		r5, r3
	mul		#9, r3
	mov.w	$RAM_Writer_YOffset, r4
	add		r3, r4
	mov.w	r4, $RAM_Writer_YOffset
	mov		#!0, r2

DrawChar_NoWrap:
	mov.w	r2, $RAM_Writer_XOffset
	
	rts
	
FUNC_Print:
	mov		r2, r0
	add		r0, r0
FUNC_Print_Loop:
	movb	(r0)+, r2
	add		#!0, r2
	bra.z	#FUNC_Print_Ret
	ssave	#%00000001
	jsr		#FUNC_DrawChar
	sload	#%00000001
	bra		#FUNC_Print_Loop
FUNC_Print_Ret:
	rts
	
NoCartMsg:
	byte "No Cartridge detected.", 0
	
DefaultFontData:
	incbin	"Font.bin"
