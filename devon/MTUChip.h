#pragma once

#include "devon.h"

using namespace Devon;

class MTUChip
{
	friend class DevonMMU;

	union MTUPointer
	{
		uLONG LAddr;

		struct
		{
			 uWORD	Page;
			 uWORD	SubPageAddr;

		} WAddr;
	};

	struct MTU
	{
		 MTUPointer	SrcPointer;
		 MTUPointer	DstPointer;
		 uWORD SrcStride;
		 uWORD DstStride;
		 uWORD Width;
		 uWORD Size;

		 uWORD CurrentX;
		 uWORD Counter;
		 uWORD Buffer;

		 bool bVRAMAccess = false;
		 void SetSrcPage(uWORD InPage)
		 { 
			 SrcPointer.WAddr.Page = InPage; 
			 bVRAMAccess = (InPage >= 4 && InPage <= 7) || (DstPointer.WAddr.Page >= 4 && DstPointer.WAddr.Page <= 7);
		 }

		 void SetDstPage(uWORD InPage) 
		 { 
			 DstPointer.WAddr.Page = InPage; 
			 bVRAMAccess = (InPage >= 4 && InPage <= 7) || (SrcPointer.WAddr.Page >= 4 && SrcPointer.WAddr.Page <= 7);
		 }
	};

	union ControlRegiser
	{
		uWORD w;
		struct {uWORD MTUA_Run:1, MTUB_Run:1;} flags;
	};

	BaseMMU & MMU;
	Devon::CPU & CPU;
	uWORD CycleCount32;
	MTU MTUA;
	MTU MTUB;
	ControlRegiser Control;

public:
	MTUChip(BaseMMU & InMMU, Devon::CPU & InCPU) : MMU(InMMU), CPU(InCPU), CycleCount32(0)
	{
		Control.w = 0;
	}

	void Tick()
	{
		if(Control.flags.MTUA_Run)
		{
			switch(CycleCount32)
			{
			case 18:
				if(MTUA.bVRAMAccess)
					break;
			case 2:	
				MMU.ReadWord(MTUA.Buffer, MTUA.SrcPointer.LAddr);
				MTUA.SrcPointer.WAddr.SubPageAddr++;
				break;

			case 22:	
				if(MTUA.bVRAMAccess)
					break;
			case 6:	
				MMU.WriteWord(MTUA.Buffer, MTUA.DstPointer.LAddr);
				MTUA.DstPointer.WAddr.SubPageAddr++;
				MTUA.CurrentX++;
				MTUA.Counter++;
				if(MTUA.Counter == MTUA.Size)
				{
					MTUA.Counter = 0;
					Control.flags.MTUA_Run = 0;
					CPU.Interrupt(2);
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
				MMU.ReadWord(MTUB.Buffer, MTUB.SrcPointer.LAddr);	
				MTUB.SrcPointer.WAddr.SubPageAddr++;
				break;

			case 30:
				if(MTUB.bVRAMAccess)
					break;
			case 14:	
				MMU.WriteWord(MTUB.Buffer, MTUB.DstPointer.LAddr);	
				MTUB.DstPointer.WAddr.SubPageAddr++;
				MTUB.CurrentX++;
				MTUB.Counter++;
				if(MTUB.Counter == MTUB.Size)
				{
					MTUB.Counter = 0;
					Control.flags.MTUB_Run = 0;
					CPU.Interrupt(1);
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

	void HardReset()
	{
		Control.w = 0;
		CycleCount32 = 0;
		MTUA.Counter = 0;
		MTUB.Counter = 0;
		MTUA.CurrentX = 0;
		MTUB.CurrentX = 0;
	}
};