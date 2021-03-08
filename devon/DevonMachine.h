#pragma once

#include "devon.h"
#include "devonMMU.h"
#include "Cortico.h"
#include "Timers.h"
#include "MTUChip.h"
#include "JKevChip.h"
#include "KeyB.h"

using namespace Devon;

class DevonMachine
{
public:
	DevonMMU	MMU;
	Devon::CPU	CPU;
	CorticoChip	Cortico;
	TimerChip	Timers;
	MTUChip		MTUs;
	JKevChip	JKev;
	KeyBChip	KeyB;

	DevonMachine() :
		CPU(MMU), 
		Cortico(MMU, CPU), 
		Timers(CPU), 
		MTUs(MMU, CPU),
		JKev(),
		KeyB()
	{
		MMU.Cortico = &Cortico;
		MMU.Timers = &Timers;
		MMU.MTUs = &MTUs;
		MMU.JKev = &JKev;
		MMU.KeyB = &KeyB;
	}

	void TickFrame()
	{
		for(int i = (268800/4)-1; i >= 0; --i)
		{
			// Even Cycle
			MMU.GFXRAMAccessOccured = false;
			MMU.RAMAccessOccured = false;
			MMU.ROMAccessOccured = false;

			(Cortico.*Cortico.Tick)();
			Timers.Tick();
			MTUs.Tick();
			JKev.Tick();
			(CPU.*CPU.Tick)();
			MMU.PostTick();

			// Odd Cycle
			Timers.Tick();
			(CPU.*CPU.Tick)();
			MMU.PostTick();

			// Even Cycle
			MMU.GFXRAMAccessOccured = false;
			MMU.RAMAccessOccured = false;

			(Cortico.*Cortico.Tick)();
			Timers.Tick();
			MTUs.Tick();
			JKev.Tick();
			(CPU.*CPU.Tick)();
			MMU.PostTick();

			// Odd Cycle
			Timers.Tick();
			(CPU.*CPU.Tick)();
			MMU.PostTick();
		}
	}

	void HardReset()
	{
		MMU.HardReset();
		CPU.HardReset();
		Cortico.HardReset();
		Timers.Reset();
		MTUs.HardReset();
		JKev.HardReset();
	}
};