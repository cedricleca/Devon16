#pragma once

#include "devon.h"

using namespace Devon;

class DevonMMU;

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
		uWORD w = 0;
		struct {uWORD MTUA_Run:1, MTUB_Run:1;} flags;
	};

	DevonMMU * MMU;
	Devon::CPU * CPU;
	MTU MTUA;
	MTU MTUB;

public:
	ControlRegiser Control;

	MTUChip(DevonMMU * InMMU, Devon::CPU * InCPU);
	void Tick();
	void HardReset();
};