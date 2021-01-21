org $0x0

define EntryPoint 0x100
define RAM_VBase 0x80000
define DefaultStack 0x80100

;VectorTable
ResetPC:			long EntryPoint
ResetStack:			long DefaultStack
BusError:			long DefaultError
BadInstruction:		long DefaultError
DivideByZero:		long DefaultError

Interrupt0:			long DefaultInterrupt
Interrupt1:			long DefaultInterrupt
Interrupt2:			long DefaultInterrupt
Interrupt3:			long DefaultInterrupt
Interrupt4:			long DefaultInterrupt
Interrupt5:			long DefaultInterrupt
Interrupt6:			long DefaultInterrupt
Interrupt7:			long DefaultInterrupt

Trap0:				long DefaultTrap
Trap1:				long DefaultTrap
Trap2:				long DefaultTrap
Trap3:				long DefaultTrap
Trap4:				long DefaultTrap
Trap5:				long DefaultTrap
Trap6:				long DefaultTrap
Trap7:				long DefaultTrap
Trap8:				long DefaultTrap
Trap9:				long DefaultTrap
TrapA:				long DefaultTrap
TrapB:				long DefaultTrap
TrapC:				long DefaultTrap
TrapD:				long DefaultTrap
TrapE:				long DefaultTrap
TrapF:				long DefaultTrap

org EntryPoint
	mov #!0x7, r4
	neg r4

	mov #RAM_VBase, r0
	mov #!0x0, r1
	mov #0x80, r2
VBaseloop:
	mov.l (r1)+, r3
	mov.l r3, (r0)+
	sub #!1, r2
	bra.P #!VBaseloop
	vbase #RAM_VBase
	halt

DefaultError:
	halt

DefaultInterrupt:
	rte

DefaultTrap:
	rte
