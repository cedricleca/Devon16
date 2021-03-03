#pragma once

#include "devon.h"
#include <string>

using namespace Devon;

static const int CorticoBPlaneNr = 8;

class CorticoChip
{
	friend class DevonMMU;

	BaseMMU & MMU;
	Devon::CPU & CPU;

	union MemPointer
	{
		uLONG l;

		struct
		{
			uWORD lsw;
			uWORD msw;
		} w;
	};

	struct ClutEntryWithCachedValue
	{
		union
		{
			uWORD uw = 0;
			struct {uWORD b:4, g:4, r:4;} rgb;
		} ClutEntry;

		unsigned int CachedValue = 0;

		ClutEntryWithCachedValue() {}
		ClutEntryWithCachedValue(uWORD _In)
		{
			ClutEntry.uw = _In;
			CachedValue = (ClutEntry.rgb.r<<4) | (ClutEntry.rgb.g<<12) | (ClutEntry.rgb.b<<20);
		}
	};

	union ControlRegiser
	{
		uWORD w;
		struct {uWORD BPLEnable:8, OverlayMode:1, OverlayBPL4Fill:1, HBL:1, VBL:1;} flags;
	};

	struct BPlaneControl
	{
		MemPointer BaseAdd;
		MemPointer CurAdd;
		uWORD InBuffer;
		uWORD Stride;
		uWORD Shift;
		uWORD VStart;
		uWORD VEnd;
		uWORD HStart;
		uWORD HEnd;

		bool Enabled;
		bool VActive;
		bool Reading;
		bool IsBKG;
	};

	uLONG OutBuffer[CorticoBPlaneNr];
	unsigned char Streaming;

	uWORD H;
	uWORD V;
	uWORD INT_H;
	uWORD INT_V;
	ControlRegiser Control;
	ClutEntryWithCachedValue Clut[32];
	BPlaneControl BPlane[CorticoBPlaneNr];

	uLONG CycleCount;
	unsigned char * OutputSurface = nullptr;

public:
	CorticoChip(BaseMMU & InMMU, Devon::CPU & InCPU) : MMU(InMMU), CPU(InCPU)
	{
	}

	void (CorticoChip::*Tick)() = nullptr;

	void Tick_PreFrame()
	{
		CycleCount += 1;
		if(CycleCount >= 19424/2)
		{
			Control.flags.VBL = 0;
			Tick = &CorticoChip::Tick_Frame;
		}
	}

	void Tick_Frame()
	{
		const int CurPack = ((CycleCount + 16) & 511)>>4; // starts 32 cycles before the new raster
		const int SubCycle = CycleCount & 0xf;

		if(SubCycle == 0)
		{
			Streaming = 0;
			for(int i = 0; i < CorticoBPlaneNr; i++)
			{
				BPlaneControl & BPL = BPlane[i];
				BPL.Reading = false;
				if(BPL.VActive)
				{
					if(CurPack >= BPL.HStart && CurPack <= BPL.HEnd)
					{
						BPL.Reading = CurPack < BPL.HEnd;
						if(CurPack > BPL.HStart)
						{
							OutBuffer[i] |= BPL.InBuffer << BPL.Shift;

							if(BPL.Shift > 15 || CurPack > BPL.HStart+1)	// Mask out the 1st Pack if Shift is used
								Streaming |= 1<<i;
						}
					}
				}
			}
		}

		unsigned char BPLBits =	OutBuffer[0]>>31;
		BPLBits |= (OutBuffer[1]>>30) & 2;
		BPLBits |= (OutBuffer[2]>>29) & 4;
		BPLBits |= (OutBuffer[3]>>28) & 8;
		BPLBits |= (OutBuffer[4]>>27) & 16;
		BPLBits |= (OutBuffer[5]>>26) & 32;
		BPLBits |= (OutBuffer[6]>>25) & 64;
		BPLBits |= (OutBuffer[7]>>24) & 128;
		BPLBits &= Streaming;

		OutBuffer[0] <<= 1;
		OutBuffer[1] <<= 1;
		OutBuffer[2] <<= 1;
		OutBuffer[3] <<= 1;
		OutBuffer[4] <<= 1;
		OutBuffer[5] <<= 1;
		OutBuffer[6] <<= 1;
		OutBuffer[7] <<= 1;

		if((SubCycle & 1) == 0)
		{
			BPlaneControl & ReadBPL = BPlane[SubCycle>>1];
			if(ReadBPL.Reading)
			{
				MMU.ReadWord(ReadBPL.InBuffer, ReadBPL.CurAdd.l++);
				if(ReadBPL.CurAdd.l == 0x80000)
					ReadBPL.CurAdd.l = 0x40000;
			}
		}

		if(CurPack <= 26)
		{
			if(CurPack > 0)
			{
				unsigned char FinalClutIdx;
				if(Control.flags.OverlayMode == 0)
					FinalClutIdx = ((BPLBits>>4) == 0) ? BPLBits & 0xf : (BPLBits>>4) + 16;
				else
					FinalClutIdx = ((BPLBits>>5) == 0) ? BPLBits & 0x1f : ((BPLBits>>5)<<1) + 16 + Control.flags.OverlayBPL4Fill;

				((unsigned int *)OutputSurface)[(V<<9) + H] = Clut[FinalClutIdx].CachedValue;
				
				Control.flags.HBL = 0;

				if(H == INT_H && V == INT_V)
					CPU.Interrupt(5);// // trig GFXPos

				H++;
			}
		}
		else if(H != 0)
		{
			CPU.Interrupt(6);// trig HBlank
			Control.flags.HBL = 1;

			H = 0;
			V++;

			for(auto & BPL : BPlane)
			{
				if(BPL.Enabled)
				{
					BPL.VActive = false;
					if(V >= BPL.VStart && V <= BPL.VEnd)
					{
						if(V > BPL.VStart)
						{
							BPL.CurAdd.l += BPL.Stride;
							BPL.CurAdd.l = 0x40000 + (BPL.CurAdd.l & 0x3FFFF);
						}

						if(V < BPL.VEnd)
							BPL.VActive = true;
					}
				}
			}
		}

		CycleCount += 1;
		if(CycleCount >= 268288/2)
			Tick = &CorticoChip::Tick_PostFrame;
	}

	void Tick_PostFrame()
	{		
		if(CycleCount == 268800/2)
		{
			Control.flags.VBL = 1;
			H = V = CycleCount = 0;
			CPU.Interrupt(7);// // trig VBlank

			for(auto & BPL : BPlane)
			{
				if(BPL.Enabled)
					BPL.CurAdd.l = BPL.BaseAdd.l;

				BPL.VActive = false;
			}

			Tick = &CorticoChip::Tick_PreFrame;
		}

		CycleCount += 1;
	}


	void SetControlRegister(uWORD uw)
	{
		Control.w = uw;
		for(int i = 0; i < CorticoBPlaneNr; i++)
		{
			BPlane[i].Enabled = Control.flags.BPLEnable & (1<<i);
			BPlane[i].IsBKG = (i < 4 || (i < 5 && Control.flags.OverlayMode != 0));
		}
	}

	void HardReset()
	{
		H = 0;
		V = 0;
		INT_H = -1;
		INT_V = -1;
		Control.w = 0;
		CycleCount = 0;

		for(auto & C : Clut)
			C = 0;

		for(auto & BPL : BPlane)
		{
			BPL.BaseAdd.l = 0;
			BPL.CurAdd.l = 0;
			BPL.Stride = 0;
			BPL.Shift = 0;
			BPL.VStart = 0;
			BPL.VEnd = 0;
			BPL.HStart = 0;
			BPL.HEnd = 0;
			BPL.VActive = 0;
		}

		SetOutputSurface(0);
		Tick = &CorticoChip::Tick_PreFrame;
	}

	void SetOutputSurface(unsigned char * _OutputSurface)
	{
		OutputSurface = _OutputSurface;
	}
};

