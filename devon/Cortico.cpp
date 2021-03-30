#pragma once

#include "Cortico.h"
#include "devon.h"
#include "devonMMU.h"

void CorticoChip::Tick_PreFrame()
{
	CycleCount++;
	if(CycleCount >= 19424/2)
	{
		Control.flags.VBL = 0;
		OutCursor = (unsigned int *)OutputSurface;
		Tick = &CorticoChip::Tick_Frame_P0;
		CurPack = 0;
	}
}

void CorticoChip::Tick_Frame_P0()
{
	const int SubCycle = CycleCount++ & 0xf;
	const int bpl = SubCycle>>1;
	if(RVEnable & (1<<SubCycle))
	{
		BPlaneControl & ReadBPL = BPlane[bpl];
		if(mHStart.m256i_i32[bpl] == 0)
		{
			mInBuffer.m256i_i32[bpl] = MMU->GFXReadWord(ReadBPL.CurAdd.l);
				
			if(mHEnd.m256i_i32[bpl] == 1)
				RVEnable &= ~(1<<SubCycle);
		}
	}

	if(SubCycle == 15)
		Tick = Tick_Frame_RasterC0_OvM;
}

void CorticoChip::Tick_Frame_RasterC0_OvM0()
{
	CurPack++;
	Control.flags.HBL = 0;
	mOutBuffer = _mm256_or_si256(_mm256_slli_epi32(mOutBuffer, 16), _mm256_sllv_epi32(mInBuffer, mInBufShift));

	__m256i mCurPack = _mm256_set1_epi32(CurPack);
	__m256i mStreamMask = mVEnable;
	mStreamMask = _mm256_and_si256(_mm256_cmpgt_epi32(mCurPack, mHStart), mStreamMask);
	mStreamMask = _mm256_andnot_si256(_mm256_cmpgt_epi32(mCurPack, mHEnd), mStreamMask);
	__m256i mStartPlusOne = _mm256_add_epi32(mHStart, _mm256_set1_epi32(1));
	__m256i mShiftCond = _mm256_andnot_si256(_mm256_cmpgt_epi32(mInBufShift, _mm256_set1_epi32(15)), _mm256_cmpeq_epi32(mCurPack, mStartPlusOne));
	mStreamMask = _mm256_andnot_si256(mShiftCond, mStreamMask);

	for(int i = 0; i < 16; i++) // output 16 pix at once
	{
		__m256i Shift = _mm256_sub_epi32(_mm256_setr_epi32(31, 30, 29, 28, 31, 30, 29, 28), _mm256_set1_epi32(i));
		__m256i PreOr = _mm256_and_si256(mStreamMask, _mm256_srlv_epi32(mOutBuffer, Shift));
		PreOr = _mm256_hadd_epi32(PreOr, PreOr);
		PreOr = _mm256_hadd_epi32(PreOr, PreOr);
		__m256i EquZero = _mm256_cmpeq_epi32(_mm256_setzero_si256(), PreOr);
		__m256i Composited = _mm256_or_si256(_mm256_and_si256(EquZero, _mm256_permute2x128_si256(PreOr, PreOr, 0x1)), _mm256_andnot_si256(EquZero, _mm256_add_epi32(_mm256_set1_epi32(16), PreOr)));
		
		*OutCursor++ = ClutCache[Composited.m256i_i32[4]];
	}

	if(H++ == INT_H && V == INT_V)
		CPU->Interrupt(5); // trig GFXPos

	if(RVEnable & 1)
	{
		if(CurPack >= mHStart.m256i_i32[0])
		{
			mInBuffer.m256i_i32[0] = MMU->GFXReadWord(BPlane[0].CurAdd.l);
				
			if(CurPack == mHEnd.m256i_i32[0]-1)
				RVEnable &= ~1;
		}
	}

	CycleCount++;
	Tick = &CorticoChip::Tick_Frame_Raster;
}

void CorticoChip::Tick_Frame_RasterC0_OvM1()
{
	CurPack++;
	Control.flags.HBL = 0;
	mOutBuffer = _mm256_or_si256(_mm256_slli_epi32(mOutBuffer, 16), _mm256_sllv_epi32(mInBuffer, mInBufShift));

	__m256i mCurPack = _mm256_set1_epi32(CurPack);
	__m256i mStreamMask = mVEnable;
	mStreamMask = _mm256_and_si256(_mm256_cmpgt_epi32(mCurPack, mHStart), mStreamMask);
	mStreamMask = _mm256_andnot_si256(_mm256_cmpgt_epi32(mCurPack, mHEnd), mStreamMask);
	__m256i mStartPlusOne = _mm256_add_epi32(mHStart, _mm256_set1_epi32(1));
	__m256i mShiftCond = _mm256_andnot_si256(_mm256_cmpgt_epi32(mInBufShift, _mm256_set1_epi32(15)), _mm256_cmpeq_epi32(mCurPack, mStartPlusOne));
	mStreamMask = _mm256_andnot_si256(mShiftCond, mStreamMask);

	for(int i = 0; i < 16; i++) // output 16 pix at once
	{
		__m256i Shift = _mm256_sub_epi32(_mm256_setr_epi32(31, 30, 29, 28, 27, 26, 25, 24), _mm256_set1_epi32(i));
		__m256i PreOr = _mm256_and_si256(mStreamMask, _mm256_srlv_epi32(mOutBuffer, Shift));
		PreOr = _mm256_hadd_epi32(PreOr, PreOr);
		PreOr = _mm256_hadd_epi32(PreOr, PreOr);
		
		unsigned char BPLBits = PreOr.m256i_i32[0] | PreOr.m256i_i32[4];
		unsigned char FinalClutIdx = ((BPLBits>>5) == 0) ? BPLBits : ((PreOr.m256i_i32[4]>>4) & 0xfe) + 16 + Control.flags.OverlayBPL4Fill;
		*OutCursor++ = ClutCache[FinalClutIdx];
	}

	if(H++ == INT_H && V == INT_V)
		CPU->Interrupt(5); // trig GFXPos

	if(RVEnable & 1)
	{
		if(CurPack >= mHStart.m256i_i32[0])
		{
			mInBuffer.m256i_i32[0] = MMU->GFXReadWord(BPlane[0].CurAdd.l);
				
			if(CurPack == mHEnd.m256i_i32[0]-1)
				RVEnable &= ~1;
		}
	}

	CycleCount++;
	Tick = &CorticoChip::Tick_Frame_Raster;
}

void CorticoChip::Tick_Frame_Raster()
{
	if(H++ == INT_H && V == INT_V)
		CPU->Interrupt(5); // trig GFXPos

	const int SubCycle = CycleCount & 0xf;
	if(RVEnable & (1<<SubCycle))
	{
		const int bpl = SubCycle>>1;
		BPlaneControl & ReadBPL = BPlane[bpl];
		if(CurPack >= mHStart.m256i_i32[bpl])
		{
			mInBuffer.m256i_i32[SubCycle>>1] = MMU->GFXReadWord(ReadBPL.CurAdd.l);
				
			if(CurPack == mHEnd.m256i_i32[bpl]-1)
				RVEnable &= ~(1<<SubCycle);
		}
	}

	if(SubCycle == 15)
	{
		if(CurPack == 26)
		{
			CPU->Interrupt(6);// trig HBlank
			Control.flags.HBL = 1;

			OutCursor += 512-416;
			H = 0;
			V++;

			if(Control.flags.OverlayMode == 0)
			{
				mVEnable = _mm256_setr_epi32(1, 2, 4, 8, 1, 2, 4, 8);
				Tick_Frame_RasterC0_OvM = &CorticoChip::Tick_Frame_RasterC0_OvM0;
			}
			else
			{
				mVEnable = _mm256_setr_epi32(1, 2, 4, 8, 16, 32, 64, 128);
				Tick_Frame_RasterC0_OvM = &CorticoChip::Tick_Frame_RasterC0_OvM1;
			}

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

			Tick = &CorticoChip::Tick_Frame_HBL;
		}
		else
		{
			Tick = Tick_Frame_RasterC0_OvM;
		}
	}

	CycleCount++;
}
	
void CorticoChip::Tick_Frame_HBL()
{
	CycleCount++;
	if(V < 243 && ((CycleCount>>4) & 31) == 31)
	{
		Tick = &CorticoChip::Tick_Frame_P0;
		CurPack = 0;
	}
	else if(CycleCount == 268288/2)
		Tick = &CorticoChip::Tick_PostFrame;
}

void CorticoChip::Tick_PostFrame()
{		
	if(CycleCount == 268800/2)
	{
		Control.flags.VBL = 1;
		H = V = CycleCount = 0;
		CPU->Interrupt(7); // trig VBlank
		RVEnable = 0;

		for(int i = 0; i < CorticoBPlaneNr; i++)
			BPlane[i].CurAdd.l = BaseAdd[i].l;

		Tick = &CorticoChip::Tick_PreFrame;
	}

	CycleCount += 1;
}


void CorticoChip::SetControlRegister(uWORD uw)
{
	Control.w = uw;
}

void CorticoChip::SetClutEntry(int Index, uWORD val)
{
	Clut[Index].uw = val;
	ClutCache[Index] = (Clut[Index].rgb.r<<4) | (Clut[Index].rgb.g<<12) | (Clut[Index].rgb.b<<20);
}

void CorticoChip::Shift(int BplIdx, int Val)	{ mInBufShift.m256i_i32[BplIdx] = Val; }
uWORD CorticoChip::Shift(int BplIdx) const { return mInBufShift.m256i_i32[BplIdx]; }

void CorticoChip::HardReset()
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

	for(auto & B : BaseAdd)		
		B.l = 0;
		
	for(auto & BPL : BPlane)
	{
		BPL.CurAdd.l = 0;
		BPL.Stride = 0;
		BPL.VStart = 0;
		BPL.VEnd = 0;
	}

	mHStart = _mm256_setzero_si256();
	mHEnd = _mm256_setzero_si256();
	mInBufShift = _mm256_setzero_si256();

	SetOutputSurface(0);
	Tick = &CorticoChip::Tick_PreFrame;
}

void CorticoChip::SetOutputSurface(unsigned char * _OutputSurface)
{
	OutputSurface = _OutputSurface;
	OutCursor = 0;
}
