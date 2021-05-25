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
	long	FUNC_Memclr
	long	FUNC_Memset
	long	FUNC_DrawChar_Color
	long	FUNC_PrintColor

DefaultClut:
	word	0x000, 0xAAA, 0xFFF, 0xC33, 0xE79, 0x543, 0xA62, 0xF93, 0xFE6, 0x244, 0x481, 0xAC2, 0x123, 0x058, 0x3AF, 0xBDE
	word	0x000, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF

ROM_EntryPoint:
	mov		#RAM_VBase, r3
	mov		#!0, r2
	mov		#0x80, r4
	jsr		#FUNC_Memcpy_L

	vbase	#RAM_VBase

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
	
	movi	#14, r3
	mov		#WelcomeMsg, r2
	jsr		#FUNC_PrintColor

	mov		#NoCartMsg, r2
	jsr		#FUNC_Print

	; ********* RAM Test *********
	mov		#0x10000, r1
	mov		$MMU_MemLayout, r0
	btsti	#1, r0 ; check if we have cpu ran extension
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
	
	; ********* GFX RAM Test *********
	mov		#0x10000, r1
	mov		$MMU_MemLayout, r0
	btsti	#0, r0 ; check if we have gfx ran extension
	mov		#0x40000, r0
	bra.z	#GFXRAMTest
	lsli	#1, r1
GFXRAMTest:
	mov		(r0), r2
	mov		r2, (r0)+
	sub		#!1, r1
	bra.p	#GFXRAMTest
	
	mov		#GfxRamTestOK, r2
	jsr		#FUNC_Print

	movi	#2, r3
	mov		#ReadyMsg, r2
	jsr		#FUNC_PrintColor

	mov		#!0, r1
	mov		#RAM_ConsoleString, r2
	mov		r1, (r2) ; null terminate the string
	add		r2, r2 ; set to byte space
	mov		r2, $RAM_ConsoleStringCsr
	mov		r2, $RAM_ConsoleString_ByteSpace
	
	bsr		#StoreCursorBack
	intmask	#%10000000		; enable VBL only

StopCPU:
	bra		#!StopCPU

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
	mov		#!1, r1
	mov.w	r1, $RAM_SendKeyToWriter

	mov.w	$Cort_BPL0_HEnd, r5
	mov.w	$Cort_BPL0_HStart, r6
	sub		r6, r5
	mov.w	$Cort_BPL0_Stride, r6
	ext16	r6
	add		r6, r5
	mov.w	r5, $RAM_Writer_LineWidth

	mov.w	$RAM_Writer_CursorTimer, r1
	sub		#!1, r1
	
	bra.p	#VBL_PopKeyb_NoCursorSwitch
	
	mov.w	$RAM_Writer_CursorState, r1
	ext16	r1
	not		r1
	mov.w	r1, $RAM_Writer_CursorState
	
	bsr.np	#DrawCursor
	bsr.z	#RemoveCursor

	movi	#15, r1	

VBL_PopKeyb_NoCursorSwitch:
	mov.w	r1, $RAM_Writer_CursorTimer

VBL_PopKeyb:
	mov.w	$KeyB_Pop, r0
	mov		r0, r1

	and		#0x80FF, r0
	add		#!0, r0
	bra.z	#VBL_PopKeyb_End

	lsri	#8, r1
	and		#0x7F, r1
	mov.w	r1, $RAM_SpecialKeyToggles

	ext16	r0
	bra.n	#VBL_PopKeyb

	mov.w	$RAM_SendKeyToWriter, r1
	add		#!0, r1
	bra.z	#VBL_PopKeyb

	and		#0xFF, r0
	cmp		#0x10, r0		; shift
	bra.z	#VBL_PopKeyb
	cmp		#0x11, r0		; control
	bra.z	#VBL_PopKeyb
	cmp		#0x12, r0		; Alt
	bra.z	#VBL_PopKeyb
	cmp		#0x13, r0		; Pause
	bra.z	#VBL_PopKeyb
	cmp		#0x14, r0		; CapsLock
	bra.z	#VBL_PopKeyb
	cmp		#0x1b, r0		; Esc
	bra.z	#VBL_PopKeyb

	cmp		#0x70, r0		; F1
	bra.z	#VBL_PopKeyb
	cmp		#0x71, r0		; F2
	bra.z	#VBL_PopKeyb
	cmp		#0x72, r0		; F3
	bra.z	#VBL_PopKeyb
	cmp		#0x73, r0		; F4
	bra.z	#VBL_PopKeyb
	cmp		#0x74, r0		; F5
	bra.z	#VBL_PopKeyb
	cmp		#0x75, r0		; F6
	bra.z	#VBL_PopKeyb

	cmp		#0x08, r0		; BackSpace
	bra.z	#VBL_PopKeyb_PressBackspace

	cmp		#0x09, r0		; Tab
	bra.z	#VBL_PopKeyb_PressTab

	cmp		#0x0D, r0		; Return
	bra.z	#VBL_PopKeyb_PressReturn

	cmp		#0x25, r0		; Left
	bra.z	#VBL_PopKeyb_PressLeft

	cmp		#0x26, r0		; Up
	bra.z	#VBL_PopKeyb_PressUp

	cmp		#0x27, r0		; Right
	bra.z	#VBL_PopKeyb_PressRight

	cmp		#0x28, r0		; Down
	bra.z	#VBL_PopKeyb_PressDown

	cmp		#0x2e, r0		; Delete
	bra.z	#VBL_PopKeyb_PressDelete

	cmp		#0xDE, r0		; '
	bra.np	#VBL_PopKeyb_NoPressQuote
	mov		#'\'', r0

VBL_PopKeyb_NoPressQuote:
	bsr		#QWERTY_ShiftChar
	mov		r0, r2
	
	mov		$RAM_ConsoleStringCsr, r0
	movb	r2, (r0)+
	mov		r0, $RAM_ConsoleStringCsr
	
	bsr		#FUNC_DrawChar
	bsr		#StoreCursorBack

	movi	#1, r0
	mov.w	r0, $RAM_Writer_CursorTimer	
	movi	#0, r0
	mov.w	r0, $RAM_Writer_CursorState
	
	bra		#VBL_PopKeyb
	
VBL_PopKeyb_PressDelete:
	mov		#' ', r2
	mov		$RAM_ConsoleStringCsr, r0
	movb	r2, (r0)+
	mov		r0, $RAM_ConsoleStringCsr

	bsr		#RemoveCursor
	mov		#' ', r2
	bsr		#FUNC_DrawChar
	bsr		#StoreCursorBack
	bra		#VBL_PopKeyb
	
VBL_PopKeyb_PressBackspace:
	mov		$RAM_ConsoleStringCsr, r0
	sub		#!1, r0
	mov		#' ', r3
	movb	r3, (r0)
	
	bsr		#RemoveCursor

	mov.w	$RAM_Writer_XOffset, r3
	sub		#!1, r3
	mov.w	r3, $RAM_Writer_XOffset	

	mov		#' ', r2
	bsr		#FUNC_DrawChar
	bsr		#StoreCursorBack
	
VBL_PopKeyb_PressLeft:
	mov		#!0, r1
	mov		$RAM_ConsoleStringCsr, r0
	add		#!-1, r0
	cmp		$RAM_ConsoleString_ByteSpace, r0
	bra.p	#!VBL_PopKeyb_PressLeft_1
	mov		$RAM_ConsoleString_ByteSpace, r0
	mov		#!1, r1 ; beginning of the buffer
VBL_PopKeyb_PressLeft_1:
	mov		r0, $RAM_ConsoleStringCsr
	ssave	#%10
	bsr		#RemoveCursor
	sload	#%10
	mov.w	$RAM_Writer_XOffset, r0
	add		#!-1, r0
	bra.pz	#VBL_PopKeyb_PressLeft_2
	add		#!0, r1
	bra.p	#VBL_PopKeyb_PressLeft_3
	mov.w	$RAM_Writer_Width, r0
	add		#!-1, r0
	mov.w	$RAM_Writer_LineWidth, r2
	mul		#18, r2
	mov.w	$RAM_Writer_YOffset, r1
	sub		r2, r1
	mov.w	r1, $RAM_Writer_YOffset
	bra		#!VBL_PopKeyb_PressLeft_2
VBL_PopKeyb_PressLeft_3:
	mov		#!0, r0
VBL_PopKeyb_PressLeft_2:
	mov.w	r0, $RAM_Writer_XOffset	
	bsr		#StoreCursorBack
	movi	#1, r0
	mov.w	r0, $RAM_Writer_CursorTimer	
	movi	#0, r0
	mov.w	r0, $RAM_Writer_CursorState
	bra		#VBL_PopKeyb

VBL_PopKeyb_PressUp:
;	bsr		#RemoveCursor
;	mov.w	$RAM_Writer_LineWidth, r5
;	mul		#18, r5
;	mov.w	$RAM_Writer_YOffset, r3
;	sub		r5, r3
;	mov.w	r3, $RAM_Writer_YOffset	
;	bsr		#StoreCursorBack
	bra		#VBL_PopKeyb

VBL_PopKeyb_PressRight:
	mov		$RAM_ConsoleStringCsr, r0
	add		#!1, r0
	cmp		$RAM_ConsoleString_ByteSpace, r0
	bra.p	#!VBL_PopKeyb_PressRight_1
	mov		$RAM_ConsoleString_ByteSpace, r0
VBL_PopKeyb_PressRight_1:
	mov		r0, $RAM_ConsoleStringCsr
	ssave	#%10
	bsr		#RemoveCursor
	sload	#%10
	mov.w	$RAM_Writer_XOffset, r0
	add		#!1, r0
	
	; TODO : check buffer overflow
	
	mov.w	$RAM_Writer_Width, r2
	cmp		r2, r0
	bra.n	#VBL_PopKeyb_PressRight_2
	mov		#!0, r0
	mov.w	$RAM_Writer_LineWidth, r2
	mul		#18, r2
	mov.w	$RAM_Writer_YOffset, r1
	add		r2, r1
	mov.w	r1, $RAM_Writer_YOffset
VBL_PopKeyb_PressRight_2:
	mov.w	r0, $RAM_Writer_XOffset	
	bsr		#StoreCursorBack
	movi	#1, r0
	mov.w	r0, $RAM_Writer_CursorTimer	
	movi	#0, r0
	mov.w	r0, $RAM_Writer_CursorState
	bra		#VBL_PopKeyb

VBL_PopKeyb_PressDown:
;	bsr		#RemoveCursor
;	mov.w	$RAM_Writer_LineWidth, r5
;	mul		#18, r5
;	mov.w	$RAM_Writer_YOffset, r3
;	add		r5, r3
;	mov.w	r3, $RAM_Writer_YOffset	
;	bsr		#StoreCursorBack
	bra		#VBL_PopKeyb

VBL_PopKeyb_PressTab:
	mov		$RAM_ConsoleStringCsr, r0
	add		#!4, r0
	and		#0xFFFC, r0
	mov		r0, $RAM_ConsoleStringCsr

	bsr		#RemoveCursor
	mov.w	$RAM_Writer_XOffset, r3
	add		#!4, r3
	and		#0xFFFC, r3
	mov.w	r3, $RAM_Writer_XOffset
	bsr		#StoreCursorBack
	bra		#VBL_PopKeyb

VBL_PopKeyb_PressReturn:
	bsr		#RemoveCursor
	mov.w	#!0, r3
	mov.w	r3, $RAM_Writer_XOffset
	mov.w	$RAM_Writer_LineWidth, r5
	mul		#18, r5
	mov.w	$RAM_Writer_YOffset, r3
	add		r5, r3
	mov.w	r3, $RAM_Writer_YOffset
	bsr		#StoreCursorBack
	
	mov		$RAM_ConsoleString_ByteSpace, r2
	mov		r2, $RAM_ConsoleStringCsr
	mov		#RAM_ConsoleString, r2
	mov		#0x100, r4
	jsr		#FUNC_Memclr
	bra		#VBL_PopKeyb

VBL_PopKeyb_End:
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
	mov		#Cort_Control, r0
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

; baseAdd = r2, Wsize = r4, filldata = r3
FUNC_Memclr:
	mov		#!0, r3
FUNC_Memset:
FUNC_Memclr_Loop:
	mov.w	r3, (r2)+
	sub		#!1, r4
	bra.pn	#!FUNC_Memclr_Loop
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
	
; r2 = char, r3 = color
FUNC_DrawChar_Color:
	movi	#0, r0
	mov.w 	r0, $RAM_Writer_AdvanceCursor
FUNC_DrawChar_Color_Loop:
	btst	r0, r3
	bra.z	#FUNC_DrawChar_Color_Continue
	mov		r0, (r7)+
	mov		r2, (r7)+
	mov		r3, (r7)+
	mov		r0, r3 ; pass in the BPL #
	bsr		#FUNC_DrawChar_wBPLOffset
	mov		-(r7), r3
	mov		-(r7), r2
	mov		-(r7), r0
FUNC_DrawChar_Color_Continue:
	add		#!1, r0
	btsti	#3, r0
	bra.z	#FUNC_DrawChar_Color_Loop	
	movi	#1, r0
	mov.w 	r0, $RAM_Writer_AdvanceCursor
	bra		#FUNC_DrawChar_AdvanceCursor

; r2 = char, r3 = BPL #
FUNC_DrawChar:
	mov		#!0, r3
	mov		#!1, r0
	mov.w	r0, $RAM_Writer_AdvanceCursor
FUNC_DrawChar_wBPLOffset:
	mov		r2, r0
	mul		#10, r3
	mov		#Cort_BPL0_HEnd, r1
	add		r3, r1
	mov.w	(r1), r5
	mov		#Cort_BPL0_HStart, r1
	add		r3, r1
	mov.w	(r1), r6
	sub		r6, r5
	mov		#Cort_BPL0_Stride, r1
	add		r3, r1
	mov.w	(r1), r6
	ext16	r6
	add		r6, r5
	add		r5, r5

	mov		#Cort_BPL0_BaseAdd_H, r1
	add		r3, r1
	mov		(r1), r2
	add		r2, r2
	mov.w	$RAM_Writer_YOffset, r6
	add		r6, r2
	mov.w	$RAM_Writer_XOffset, r6
	add		r6, r2
	
	sub		#'!', r0
	mov		r0, r1
	lsri	#1, r1
	mul		#9, r1
	add		#RAM_Font, r1
	add		r1, r1
	and		#!1, r0
	add		r0, r1
	
	movi	#8, r6	
FUNC_DrawChar_Loop:
	movb	(r1), r4
	movb	r4, (r2)
	add		r5, r2
	add		#!2, r1
	sub		#!1, r6
	bra.pn	#!FUNC_DrawChar_Loop
	movb	(r1), r4
	movb	r4, (r2)

	mov.w 	$RAM_Writer_AdvanceCursor, r2
	ext16	r2
	add		#!0, r2
	bra.np	#!FUNC_DrawChar_AdvanceCursor
	rts
	
FUNC_DrawChar_AdvanceCursor:
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

FUNC_PrintColor:
	mov		r2, r0
	add		r0, r0
FUNC_PrintColor_Loop:
	movb	(r0)+, r2
	add		#!0, r2
	bra.z	#FUNC_Print_Ret
	cmp		#'\n', r2
	bra.np	#FUNC_PrintColor_DrawChar
	
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
	jmp		#FUNC_PrintColor_Loop
	
FUNC_PrintColor_DrawChar:
	ssave	#%00001001
	jsr		#FUNC_DrawChar_Color
	sload	#%00001001
	bra		#FUNC_PrintColor_Loop
FUNC_PrintColor_Ret:
	rts

; unrecoverable error : r2=Error message to display
KrakenStrike:
	ssave	#%00000100
	jsr		#FUNC_ResetCorticoRegs	; Reset Cortico registers
	jsr		#FUNC_ResetDefaultClut	; Reset Clut
	jsr		#FUNC_ClearScreen
	mov		#0xf40, r2
	mov.w	r2, $Cort_Clut01
	mov		#!0, r2
	mov		r2, $RAM_Writer_YOffset ; resets X & Y
	mov		#KrakenStrike_Title, r2
	jsr		#FUNC_Print	
	sload	#%00000100
	jsr		#FUNC_Print	
	mov		#KrakenStrike_Text, r2
	jsr		#FUNC_Print	
	rts

; console related functions
StoreCursorBack:
	mov.w	$RAM_Writer_LineWidth, r5
	mov.w	$RAM_Writer_YOffset, r2
	mov.w	$RAM_Writer_XOffset, r3
	add		r3, r2
	lsri	#1, r2
	add		#0x40000, r2
	mov		r2, $RAM_Writer_BkSavAdd
	mov		#RAM_Writer_BkSav, r3
	movi	#7, r0
StoreCursorBack_Loop:
	mov.w	(r2), r1
	mov.w	r1, (r3)+
	add		r5, r2
	sub		#!1, r0
	bra.p	#!StoreCursorBack_Loop
	rts

RemoveCursor:
	mov.w	$RAM_Writer_LineWidth, r5
	mov		$RAM_Writer_BkSavAdd, r2
	mov		#RAM_Writer_BkSav, r3
	movi	#7, r0
RemoveCursor_Loop:
	mov.w	(r3)+, r1
	mov.w	r1, (r2)
	add		r5, r2
	sub		#!1, r0
	bra.p	#!RemoveCursor_Loop
	rts

DrawCursor:
	mov.w	$RAM_Writer_LineWidth, r5
	mov.w	$RAM_Writer_YOffset, r2
	mov.w	$RAM_Writer_XOffset, r3
	add		r3, r2
	lsri	#1, r2
	add		#0x40000, r2

	mov		#0xE0, r4
	btsti	#0, r3
	bra.np	#DrawCursor_	
	lsli	#8, r4
DrawCursor_:
	or.w	r4, (r2)
	add		r5, r2
	or.w	r4, (r2)
	add		r5, r2
	or.w	r4, (r2)
	add		r5, r2
	or.w	r4, (r2)
	add		r5, r2
	or.w	r4, (r2)
	add		r5, r2
	or.w	r4, (r2)
	add		r5, r2
	or.w	r4, (r2)
	rts

QWERTY_ShiftChar:
	mov.w	$RAM_SpecialKeyToggles, r1
	btsti	#0, r1	; caps lock
	bra.z	#!QWERTY_ShiftChar_CheckShift	
	bnoti	#4, r1
	
QWERTY_ShiftChar_CheckShift:
	btsti	#4, r1	; shift
	bra.z	#QWERTY_ShiftChar_Ret

	mov		#QWERTY_ShiftChar_LookUp, r1
QWERTY_ShiftOn_Loop:
	mov.w	(r1)+, r2
	mov		r2, r3
	lsri	#8, r3	
	cmp		r3, r0
	bra.np	#!QWERTY_ShiftChar_NoMatch
	and		#0xff, r2
	mov		r2, r0
	rts
	
QWERTY_ShiftChar_NoMatch:
	cmp		#QWERTY_ShiftChar_LookUp_End, r1
	bra.np	#QWERTY_ShiftOn_Loop

	cmp		#'A', r0
	bra.n	#!QWERTY_ShiftChar_Ret
	cmp		#'Z', r0
	bra.p	#!QWERTY_ShiftChar_Ret
	add		#32, r0
QWERTY_ShiftChar_Ret:
	rts

	align	1	
QWERTY_ShiftChar_LookUp:
	byte '1', '!'
	byte '2', '@'
	byte '3', '#'
	byte '4', '$'
	byte '5', '%'
	byte '6', '^'
	byte '7', '&'
	byte '8', '*'
	byte '9', '('
	byte '0', ')'
	byte '-', '_'
	byte '=', '+'
	byte '\\', '|'
	byte '[', '{'
	byte ']', '}'
	byte ',', '<'
	byte '.', '>'
	byte '/', '?'
	byte ';', ':'
	byte '\'', '"'
	byte '`', '~'
QWERTY_ShiftChar_LookUp_End:

KrakenStrike_Title:				byte "Kraken strike: ", 0
KrakenStrike_Text:				byte "\nThis is an unrecoverable event\nPlease reset", 0
KrakenStrike_BusError:			byte "Bus error", 0
KrakenStrike_InstructionError:	byte "Bad instruction", 0
KrakenStrike_DivideByZero:		byte "Divide by zero", 0

WelcomeMsg:		byte "Devon16 prototype, welcome.\n", 0
ReadyMsg:		byte "Ready\n", 0
CpuRamTestOK:	byte "CPU RAM Check OK.\n", 0
GfxRamTestOK:	byte "GFX RAM Check OK.\n", 0
NoCartMsg:		byte "No Cartridge detected.\n", 0
	
DefaultFontData:	incbin	"Font.bin"

