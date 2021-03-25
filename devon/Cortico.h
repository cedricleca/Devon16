#pragma once

#include <cassert>
#include "devon.h"
#include <immintrin.h>

static const int CorticoBPlaneNr = 8;

class DevonMMU;

using namespace Devon;

class CorticoChip
{
	friend DevonMMU;

	DevonMMU * MMU;
	Devon::CPU * CPU;

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
		MemPointer CurAdd;
		uWORD Stride;
		uWORD VStart;
		uWORD VEnd;
		uWORD HStart;
		uWORD HEnd;
		uWORD DumPad;
	};

	__m256i mInBuffer;
	__m256i mOutBuffer;
	__m256i mVEnable;
	__m256i mInBufShift;

	unsigned short RVEnable;
	int CurPack;

	uWORD H;
	uWORD V;
	uWORD INT_H;
	uWORD INT_V;
	ControlRegiser Control;
	ClutEntry Clut[32];
	unsigned int ClutCache[32];
	BPlaneControl BPlane[CorticoBPlaneNr];
	MemPointer BaseAdd[CorticoBPlaneNr];

	uLONG CycleCount;
	unsigned char * OutputSurface = nullptr;
	unsigned int * OutCursor = nullptr;

public:
	CorticoChip(DevonMMU * InMMU, Devon::CPU * InCPU) : MMU(InMMU), CPU(InCPU) 
	{
		assert(MMU && CPU);
	}

	void (CorticoChip::*Tick)() = nullptr;

	void Tick_PreFrame();
	void Tick_Frame_P0();
	void Tick_Frame_RasterC0();
	void Tick_Frame_Raster();	
	void Tick_Frame_HBL();
	void Tick_PostFrame();
	void SetControlRegister(uWORD uw);
	void SetClutEntry(int Index, uWORD val);
	void Shift(int BplIdx, int Val);
	uWORD Shift(int BplIdx) const;
	void HardReset();
	void SetOutputSurface(unsigned char * _OutputSurface);
};

