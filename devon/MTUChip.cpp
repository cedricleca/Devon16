#pragma once

#include "MTUChip.h"
#include "devonMMU.h"
#include <cassert>

using namespace Devon;

MTUChip::MTUChip(DevonMMU * InMMU, Devon::CPU * InCPU) : MMU(InMMU), CPU(InCPU)
{
	assert(MMU && CPU);
}

void MTUChip::Tick()
{
	if(Control.flags.MTUA_Run)
	{
		switch(CycleCount32)
		{
		case 18:
			if(MTUA.bVRAMAccess)
				break;
		case 2:	
			MMU->ReadWord(MTUA.Buffer, MTUA.SrcPointer.LAddr);
			MTUA.SrcPointer.WAddr.SubPageAddr++;
			break;

		case 22:	
			if(MTUA.bVRAMAccess)
				break;
		case 6:	
			MMU->WriteWord(MTUA.Buffer, MTUA.DstPointer.LAddr);
			MTUA.DstPointer.WAddr.SubPageAddr++;
			MTUA.CurrentX++;
			MTUA.Counter++;
			if(MTUA.Counter == MTUA.Size)
			{
				MTUA.Counter = 0;
				Control.flags.MTUA_Run = 0;
				CPU->Interrupt(2);
			}
			if(MTUA.CurrentX == MTUA.Width)
			{
				MTUA.CurrentX = 0;
				MTUA.SrcPointer.WAddr.SubPageAddr += MTUA.SrcStride;
				MTUA.DstPointer.WAddr.SubPageAddr += MTUA.DstStride;
			}
			break;
		}
	}

	if(Control.flags.MTUB_Run)
	{
		switch(CycleCount32)
		{
		case 26:
			if(MTUB.bVRAMAccess)
				break;
		case 10:	
			MMU->ReadWord(MTUB.Buffer, MTUB.SrcPointer.LAddr);	
			MTUB.SrcPointer.WAddr.SubPageAddr++;
			break;

		case 30:
			if(MTUB.bVRAMAccess)
				break;
		case 14:	
			MMU->WriteWord(MTUB.Buffer, MTUB.DstPointer.LAddr);	
			MTUB.DstPointer.WAddr.SubPageAddr++;
			MTUB.CurrentX++;
			MTUB.Counter++;
			if(MTUB.Counter == MTUB.Size)
			{
				MTUB.Counter = 0;
				Control.flags.MTUB_Run = 0;
				CPU->Interrupt(1);
			}
			if(MTUB.CurrentX == MTUB.Width)
			{
				MTUB.CurrentX = 0;
				MTUB.SrcPointer.WAddr.SubPageAddr += MTUB.SrcStride;
				MTUB.DstPointer.WAddr.SubPageAddr += MTUB.DstStride;
			}
			break;
		}
	}

	CycleCount32 = (CycleCount32 + 2) & 0x1F;
}

void MTUChip::HardReset()
{
	Control.w = 0;
	CycleCount32 = 0;
	MTUA.Counter = 0;
	MTUB.Counter = 0;
	MTUA.CurrentX = 0;
	MTUB.CurrentX = 0;
}
