define CartBase 0x20000

romexport

include "SystemRam.das"
include "ControlRegisters.das"

;VectorTable
org $0x0
ResetPC:			long ROM_EntryPoint
ResetStack:			long DefaultStack
BusError:			long DefaultBusError
BadInstruction:		long DefaultInstructionError
DivideByZero:		long DefaultZeroDivideError

org $0x20
Interrupt0:			long DefaultInterrupt_DSKDMA	 ; 0x20
Interrupt1:			long DefaultInterrupt_MTUB		 ; 0x22
Interrupt2:			long DefaultInterrupt_MTUA		 ; 0x24
Interrupt3:			long DefaultInterrupt_TimerB	 ; 0x26
Interrupt4:			long DefaultInterrupt_TimerA	 ; 0x28
Interrupt5:			long DefaultInterrupt_GFXPos	 ; 0x2a
Interrupt6:			long DefaultInterrupt_HBlank	 ; 0x2c
Interrupt7:			long DefaultInterrupt_VBlank	 ; 0x2e

org $0x40
Trap0:				long DefaultTrap0
Trap1:				long DefaultTrap_Unused
Trap2:				long DefaultTrap_Unused
Trap3:				long DefaultTrap_Unused
Trap4:				long DefaultTrap_Unused
Trap5:				long DefaultTrap_Unused
Trap6:				long DefaultTrap_Unused
Trap7:				long DefaultTrap_Unused
Trap8:				long DefaultTrap_Unused
Trap9:				long DefaultTrap_Unused
TrapA:				long DefaultTrap_Unused
TrapB:				long DefaultTrap_Unused
TrapC:				long DefaultTrap_Unused
TrapD:				long DefaultTrap_Unused
TrapE:				long DefaultTrap_Unused
TrapF:				long DefaultTrap_Unused

Trap0_FUNC_Table:
	long	FUNC_Memcpy
	long	FUNC_Memcpy_L
	long	FUNC_ResetCorticoRegs
	long	FUNC_ResetDefaultClut
	long	FUNC_ResetDefaultFont
	long	FUNC_DrawChar
	long	FUNC_Print
	long	FUNC_ClearScreen

DefaultClut:
	word	0x000, 0xAAA, 0xFFF, 0xC33, 0xE79, 0x543, 0xA62, 0xF93, 0xFE6, 0x244, 0x481, 0xAC2, 0x123, 0x058, 0x3AF, 0xBDE
	word	0x000, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF

ROM_EntryPoint:
	mov		#RAM_VBase, r3
	mov		#!0, r2
	mov		#0x80, r4
	jsr		#FUNC_Memcpy_L

	vbase	#RAM_VBase
	intmask	#%00000000		; enable VBL only

	mov		#!0, r0
	mov		r0, $RAM_Writer_YOffset ; also resets XOffset
	movi	#40, r0
	mov.w	r0, $RAM_Writer_Width

	jsr		#FUNC_ResetCorticoRegs	; Reset Cortico registers
	jsr		#FUNC_ResetDefaultClut	; Reset Clut
	jsr		#FUNC_ResetDefaultFont	; Restore original font data

	mov		$MMU_CartridgeSize_HI, r0	; get the Catridge size
	add		#!0, r0			; test it
	jmp.np	#CartBase		; if present -> jmp

	jsr		#FUNC_ClearScreen	
	
	mov		#NoCartMsg, r2
	jsr		#FUNC_Print

	; ********* RAM Test *********
	mov		#0x10000, r1
	mov		$MMU_MemLayout, r0
	btsti	#1, r0
	mov		#0x80000, r0
	bra.z	#RAMTest
	lsli	#1, r1
RAMTest:
	mov		(r0), r2
	mov		r2, (r0)+
	sub		#!1, r1
	bra.p	#RAMTest
	
	mov		#CpuRamTestOK, r2
	jsr		#FUNC_Print	
	halt

DefaultBusError:
	mov		#KrakenStrike_BusError, r2
	jsr		#KrakenStrike	
	halt

DefaultInstructionError:
	mov		#KrakenStrike_InstructionError, r2
	jsr		#KrakenStrike	
	halt

DefaultZeroDivideError:
	mov		#KrakenStrike_DivideByZero, r2
	jsr		#KrakenStrike	
	halt

DefaultInterrupt_VBlank:
	rte

DefaultInterrupt_HBlank:
DefaultInterrupt_GFXPos:
DefaultInterrupt_TimerA:
DefaultInterrupt_TimerB:
DefaultInterrupt_MTUA:
DefaultInterrupt_MTUB:
DefaultInterrupt_DSKDMA:
	rte

DefaultTrap0:
	; r0 = function Id
	; r1 is wiped
	mov	#Trap0_FUNC_Table, r1
	add	r0, r0
	add r0, r1
	jsr (r1)
	rte

DefaultTrap_Unused:
	rte

FUNC_ResetCorticoRegs:
	mov		#Cort_H, r0
	add		#!4, r0
	movi	#0xF, r1
	mov.w	r1, (r0)+		; BPLEnable 

	mov		#!7, r1
	ssave	#%10	
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

ResetCorticoRegs_Loop:
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
	
	sswap	#%10
	sub		#!1, r1
	sswap	#%10
	bra.p	#ResetCorticoRegs_Loop
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

FUNC_ClearScreen:
	mov.w	$Cort_BPL0_HEnd, r5
	mov.w	$Cort_BPL0_HStart, r6
	sub		r6, r5
	mov.w	$Cort_BPL0_Stride, r6
	ext16	r6
	add		r6, r5
	mov.w	$Cort_BPL0_VEnd, r4
	mov.w	$Cort_BPL0_VStart, r6
	sub		r6, r4
	mul		r5, r4
	
	mov		$Cort_BPL0_BaseAdd_H, r2
	mov		#!0, r1
FUNC_ClearScreen_Loop:
	mov		r1, (r2)+
	sub		#!1, r4
	bra.p	#!FUNC_ClearScreen_Loop
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
	cmp		#'\n', r2
	bra.np	#FUNC_Print_DrawChar
	
	mov		#!0, r2
	mov.w	r2, $RAM_Writer_XOffset

	mov.w	$Cort_BPL0_HEnd, r5
	mov.w	$Cort_BPL0_HStart, r6
	sub		r6, r5
	mov.w	$Cort_BPL0_Stride, r6
	ext16	r6
	add		r6, r5
	mul		#18, r5

	mov.w	$RAM_Writer_YOffset, r2
	add		r5, r2
	mov.w	r2, $RAM_Writer_YOffset
	jmp		#FUNC_Print_Loop
	
FUNC_Print_DrawChar:
	ssave	#%00000001
	jsr		#FUNC_DrawChar
	sload	#%00000001
	bra		#FUNC_Print_Loop
FUNC_Print_Ret:
	rts

; unrecoverable error : r2=Error message to display
KrakenStrike:
	ssave	#%00000100
	jsr		#FUNC_ResetCorticoRegs	; Reset Cortico registers
	jsr		#FUNC_ResetDefaultClut	; Reset Clut
	jsr		#FUNC_ClearScreen
	mov		#!0, r2
	mov		r2, $RAM_Writer_YOffset ; resets X & Y
	mov		#KrakenStrike_Title, r2
	jsr		#FUNC_Print	
	sload	#%00000100
	jsr		#FUNC_Print	
	mov		#KrakenStrike_Text, r2
	jsr		#FUNC_Print	
	rts

KrakenStrike_Title:				byte "Kraken strike: ", 0
KrakenStrike_Text:				byte "\nThis is an unrecoverable event\nPlease reset", 0
KrakenStrike_BusError:			byte "Bus error", 0
KrakenStrike_InstructionError:	byte "Bad instruction", 0
KrakenStrike_DivideByZero:		byte "Divide by zero", 0
		
CpuRamTestOK:	byte "RAM Check OK.", 0
NoCartMsg:		byte "No Cartridge detected.\n", 0
	
DefaultFontData:	incbin	"Font.bin"

