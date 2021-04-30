
include "..\ControlRegisters.das"
include "..\SystemRam.das"
include "..\ROMFuncDef.das"

org $0x20000
	mov		#0x40000, r0
	mov		r0, $Cort_BPL0_BaseAdd_H
	add		#4000, r0
	mov		r0, $Cort_BPL1_BaseAdd_H
	add		#4000, r0
	mov		r0, $Cort_BPL2_BaseAdd_H

	mov		#!1, r0
	mov.w	r0, $Cort_Control
	
	mov		#VBL, r0
	mov		r0, $0x8002E
	intmask	#%10000000

	mov		#!1, r1
	mov.w	r1, $RAM_SendKeyToWriter

	mov		#!0, r1
CharLoop:
	mov		r1, r2
	add		#'!', r2
	ssave	#%11111111
	mov		#!FUNC_DrawChar, r0 ; call DrawChar
	trap	#!0
	sload	#%11111111
	add		#!1, r1
	cmp		#94, r1
	bra.pn	#CharLoop

	bsr		#StoreCursorBack

stop:
	bra		#!stop

VBL:
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

	bra.z	#VBL_PopKeyb_CursorSwitchOff
	bsr		#DrawCursor
	bra		#VBL_PopKeyb_CursorSwitchEnd

VBL_PopKeyb_CursorSwitchOff:
	bsr		#RemoveCursor

VBL_PopKeyb_CursorSwitchEnd:
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

	cmp		#0xDE, r0		; '
	bra.np	#VBL_PopKeyb_NoPressQuote
	mov		#'\'', r0

VBL_PopKeyb_NoPressQuote:
	bsr		#QWERTY_ShiftChar

	bra		#VBL_PopKeyb_SendToWriter

VBL_PopKeyb_PressLeft:
	bsr		#RemoveCursor
	mov.w	$RAM_Writer_XOffset, r3
	sub		#!1, r3
	mov.w	r3, $RAM_Writer_XOffset	
	bsr		#StoreCursorBack
	bra		#VBL_PopKeyb

VBL_PopKeyb_PressUp:
	bsr		#RemoveCursor
	mov.w	$RAM_Writer_LineWidth, r5
	mul		#18, r5
	mov.w	$RAM_Writer_YOffset, r3
	sub		r5, r3
	mov.w	r3, $RAM_Writer_YOffset	
	bsr		#StoreCursorBack
	bra		#VBL_PopKeyb

VBL_PopKeyb_PressRight:
	bsr		#RemoveCursor
	mov.w	$RAM_Writer_XOffset, r3
	add		#!1, r3
	mov.w	r3, $RAM_Writer_XOffset	
	bsr		#StoreCursorBack
	bra		#VBL_PopKeyb

VBL_PopKeyb_PressDown:
	bsr		#RemoveCursor
	mov.w	$RAM_Writer_LineWidth, r5
	mul		#18, r5
	mov.w	$RAM_Writer_YOffset, r3
	add		r5, r3
	mov.w	r3, $RAM_Writer_YOffset	
	bsr		#StoreCursorBack
	bra		#VBL_PopKeyb

VBL_PopKeyb_PressTab:
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
	bra		#VBL_PopKeyb

VBL_PopKeyb_SendToWriter:
	mov		r0, r2
	mov		#!FUNC_DrawChar, r0 ; call DrawChar
	trap	#!0
	bsr		#StoreCursorBack

	movi	#1, r0
	mov.w	r0, $RAM_Writer_CursorTimer	
	movi	#0, r0
	mov.w	r0, $RAM_Writer_CursorState
	bra		#VBL_PopKeyb
	
VBL_PopKeyb_End:
	rte

StoreCursorBack:
	mov.w	$RAM_Writer_LineWidth, r5
	mov.w	$RAM_Writer_YOffset, r2
	mov.w	$RAM_Writer_XOffset, r3
	add		r3, r2
	lsri	#1, r2
	add		#0x40000, r2
	mov		r2, $RAM_Writer_BkSavAdd
	mov		#RAM_Writer_BkSav, r3

	mov.w	(r2), r1
	mov.w	r1, (r3)+
	add		r5, r2
	mov.w	(r2), r1
	mov.w	r1, (r3)+
	add		r5, r2
	mov.w	(r2), r1
	mov.w	r1, (r3)+
	add		r5, r2
	mov.w	(r2), r1
	mov.w	r1, (r3)+
	add		r5, r2
	mov.w	(r2), r1
	mov.w	r1, (r3)+
	add		r5, r2
	mov.w	(r2), r1
	mov.w	r1, (r3)+
	add		r5, r2
	mov.w	(r2), r1
	mov.w	r1, (r3)+
	rts

RemoveCursor:
	mov		$RAM_Writer_BkSavAdd, r2
	mov		#RAM_Writer_BkSav, r3
	mov.w	(r3)+, r1
	mov.w	r1, (r2)
	add		r5, r2
	mov.w	(r3)+, r1
	mov.w	r1, (r2)
	add		r5, r2
	mov.w	(r3)+, r1
	mov.w	r1, (r2)
	add		r5, r2
	mov.w	(r3)+, r1
	mov.w	r1, (r2)
	add		r5, r2
	mov.w	(r3)+, r1
	mov.w	r1, (r2)
	add		r5, r2
	mov.w	(r3)+, r1
	mov.w	r1, (r2)
	add		r5, r2
	mov.w	(r3)+, r1
	mov.w	r1, (r2)
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
	mov		#RAM_Writer_BkSav, r3
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
	btsti	#4, r1
	bra.np	#QWERTY_ShiftOn

	cmp		#'A', r0
	bra.n	#QWERTY_ShiftChar_Ret
	cmp		#'Z', r0
	bra.p	#QWERTY_ShiftChar_Ret
	add		#32, r0
	rts

QWERTY_ShiftOn:
	cmp		#'1', r0
	bra.np	#!QWERTY_ShiftChar_No_1
	mov		#'!', r0
QWERTY_ShiftChar_No_1:
	cmp		#'2', r0
	bra.np	#!QWERTY_ShiftChar_No_2
	mov		#'@', r0
QWERTY_ShiftChar_No_2:
	cmp		#'3', r0
	bra.np	#!QWERTY_ShiftChar_No_3
	mov		#'#', r0
QWERTY_ShiftChar_No_3:
	cmp		#'4', r0
	bra.np	#!QWERTY_ShiftChar_No_4
	mov		#'$', r0
QWERTY_ShiftChar_No_4:
	cmp		#'5', r0
	bra.np	#!QWERTY_ShiftChar_No_5
	mov		#'%', r0
QWERTY_ShiftChar_No_5:
	cmp		#'6', r0
	bra.np	#!QWERTY_ShiftChar_No_6
	mov		#'^', r0
QWERTY_ShiftChar_No_6:
	cmp		#'7', r0
	bra.np	#!QWERTY_ShiftChar_No_7
	mov		#'&', r0
QWERTY_ShiftChar_No_7:
	cmp		#'8', r0
	bra.np	#!QWERTY_ShiftChar_No_8
	mov		#'*', r0
QWERTY_ShiftChar_No_8:
	cmp		#'9', r0
	bra.np	#!QWERTY_ShiftChar_No_9
	mov		#'(', r0
QWERTY_ShiftChar_No_9:
	cmp		#'0', r0
	bra.np	#!QWERTY_ShiftChar_No_0
	mov		#')', r0
QWERTY_ShiftChar_No_0:
	cmp		#'-', r0
	bra.np	#!QWERTY_ShiftChar_No_Minus
	mov		#'_', r0
QWERTY_ShiftChar_No_Minus:
	cmp		#'=', r0
	bra.np	#!QWERTY_ShiftChar_No_Equal
	mov		#'+', r0
QWERTY_ShiftChar_No_Equal:
	cmp		#'\\', r0
	bra.np	#!QWERTY_ShiftChar_No_Slash
	mov		#'|', r0
QWERTY_ShiftChar_No_Slash:
	cmp		#'[', r0
	bra.np	#!QWERTY_ShiftChar_No_BracketOpen
	mov		#'{', r0
QWERTY_ShiftChar_No_BracketOpen:
	cmp		#']', r0
	bra.np	#!QWERTY_ShiftChar_No_BracketClose
	mov		#'}', r0
QWERTY_ShiftChar_No_BracketClose:
	cmp		#',', r0
	bra.np	#!QWERTY_ShiftChar_No_Comma
	mov		#'<', r0
QWERTY_ShiftChar_No_Comma:
	cmp		#'.', r0
	bra.np	#!QWERTY_ShiftChar_No_Dot
	mov		#'>', r0
QWERTY_ShiftChar_No_Dot:
	cmp		#'/', r0
	bra.np	#!QWERTY_ShiftChar_No_Slash2
	mov		#'?', r0
QWERTY_ShiftChar_No_Slash2:
	cmp		#';', r0
	bra.np	#!QWERTY_ShiftChar_No_SemiCol
	mov		#':', r0
QWERTY_ShiftChar_No_SemiCol:
	cmp		#'\'', r0
	bra.np	#!QWERTY_ShiftChar_No_Quote
	mov		#'"', r0
QWERTY_ShiftChar_No_Quote:
	cmp		#'`', r0
	bra.np	#!QWERTY_ShiftChar_No_BackQuote
	mov		#'~', r0
QWERTY_ShiftChar_No_BackQuote:

QWERTY_ShiftChar_Ret:
	rts

CLUTData:
	include "TestPCXPal.das"
BPLData:
	incbin "TestPCX.bin"



