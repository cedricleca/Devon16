#pragma once

#include "devon.h"
#include <string>
#include <immintrin.h>

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

	union ClutEntry
	{
		uWORD uw = 0;
		struct {uWORD b:4, g:4, r:4;} rgb;
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
		uWORD Stride;
		uWORD Shift;
		uWORD VStart;
		uWORD VEnd;
		uWORD HStart;
		uWORD HEnd;
	};

	__m256i mInBuffer;
	__m256i mOutBuffer;
	__m256i mStreamMask;
	__m256i mVEnable;

	unsigned char RVEnable;

	uWORD H;
	uWORD V;
	uWORD INT_H;
	uWORD INT_V;
	ControlRegiser Control;
	ClutEntry Clut[32];
	unsigned int ClutCache[32];
	BPlaneControl BPlane[CorticoBPlaneNr];

	uLONG CycleCount;
	unsigned char * OutputSurface = nullptr;
	unsigned int * OutCursor = nullptr;

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
			OutCursor = (unsigned int *)OutputSurface;
			Tick = &CorticoChip::Tick_Frame;
		}
	}

	void Tick_Frame()
	{
		const int CurPack = ((CycleCount>>4) & 31) + 1; // starts 1 pack before the new raster
		if(CurPack <= 26)
		{
			const int SubCycle = CycleCount & 0xf;
			if(CurPack > 0)
			{
				if(SubCycle == 0)
				{
					mStreamMask = mVEnable;
					mOutBuffer = _mm256_or_si256(_mm256_slli_epi32(mOutBuffer, 16), mInBuffer);

					for(int i = 0; i < CorticoBPlaneNr; i++)
					{
						BPlaneControl & BPL = BPlane[i];
						if(CurPack <= BPL.HStart
							|| CurPack > BPL.HEnd
							|| (CurPack == BPL.HStart+1 && BPL.Shift < 16)
							)
							mStreamMask.m256i_i32[i] = 0;
					}
				}
			
				__m256i Shift = _mm256_sub_epi32(_mm256_setr_epi32(31, 30, 29, 28, 27, 26, 25, 24), _mm256_set1_epi32(SubCycle));
				__m256i PreOr = _mm256_and_si256(mStreamMask, _mm256_srlv_epi32(mOutBuffer, Shift));
				PreOr = _mm256_hadd_epi32(PreOr, PreOr);
				PreOr = _mm256_hadd_epi32(PreOr, PreOr);
	
				unsigned char FinalClutIdx;
				if(PreOr.m256i_i32[4] == 0)
					FinalClutIdx = PreOr.m256i_i32[0];
				else if(Control.flags.OverlayMode == 0)
					FinalClutIdx = PreOr.m256i_i32[4] + 16;
				else
				{
					unsigned char BPLBits = PreOr.m256i_i32[0] | PreOr.m256i_i32[4];
					FinalClutIdx = ((BPLBits>>5) == 0) ? BPLBits : (PreOr.m256i_i32[4] & 0xfe) + 16 + Control.flags.OverlayBPL4Fill;
				}

				*OutCursor++ = ClutCache[FinalClutIdx];
				
				Control.flags.HBL = 0;

				if(H == INT_H && V == INT_V)
					CPU.Interrupt(5); // trig GFXPos

				H++;
			}

			if(RVEnable & (1<<SubCycle))
			{
				BPlaneControl & ReadBPL = BPlane[SubCycle>>1];
				if(CurPack >= ReadBPL.HStart)
				{
					mInBuffer.m256i_i32[SubCycle>>1] = MMU.GFXReadWord(ReadBPL.CurAdd.l) << ReadBPL.Shift;
				
					if(CurPack == ReadBPL.HEnd-1)
						RVEnable &= ~(1<<SubCycle);
				}
			}
		}
		else if(H != 0)
		{
			CPU.Interrupt(6);// trig HBlank
			Control.flags.HBL = 1;

			OutCursor += 512-416;
			H = 0;
			V++;

			mVEnable = _mm256_setr_epi32(1, 2, 4, 8, 16, 32, 64, 128);
			RVEnable = 0;
			for(int i = 0; i < CorticoBPlaneNr; i++)
			{
				BPlaneControl & BPL = BPlane[i];
				if((Control.flags.BPLEnable & (1<<i)) && V >= BPL.VStart && V < BPL.VEnd)
					RVEnable |= 1<<(i*2);
				else
					mVEnable.m256i_i32[i] = 0;

				if(V > BPL.VStart && V <= BPL.VEnd)
				{
					BPL.CurAdd.l += BPL.Stride;
					BPL.CurAdd.l = 0x40000 + (BPL.CurAdd.l & 0x1FFFF);
				}
			}
		}

		CycleCount++;
		if(CycleCount == 268288/2)
			Tick = &CorticoChip::Tick_PostFrame;
	}

	void Tick_PostFrame()
	{		
		if(CycleCount == 268800/2)
		{
			Control.flags.VBL = 1;
			H = V = CycleCount = 0;
			CPU.Interrupt(7); // trig VBlank
			RVEnable = 0;

			for(auto & BPL : BPlane)
				BPL.CurAdd.l = BPL.BaseAdd.l;

			Tick = &CorticoChip::Tick_PreFrame;
		}

		CycleCount += 1;
	}


	void SetControlRegister(uWORD uw)
	{
		Control.w = uw;
	}

	void SetClutEntry(int Index, uWORD val)
	{
		Clut[Index].uw = val;
		ClutCache[Index] = (Clut[Index].rgb.r<<4) | (Clut[Index].rgb.g<<12) | (Clut[Index].rgb.b<<20);
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
			C.uw = 0;

		for(auto & C : ClutCache)
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
		}

		SetOutputSurface(0);
		Tick = &CorticoChip::Tick_PreFrame;
	}

	void SetOutputSurface(unsigned char * _OutputSurface)
	{
		OutputSurface = _OutputSurface;
		OutCursor = 0;
	}
};

