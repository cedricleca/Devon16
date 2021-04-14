#pragma once

#include "devon.h"

using namespace Devon;

class TimerChip
{
	friend class DevonMMU;

	union TimerRegister
	{
		uLONG l = 0;
		struct { uWORD lsw; uWORD msw; } w;
	};

	struct TimerUnit
	{
		TimerRegister Base;
		TimerRegister Value;
	};

	Devon::CPU & CPU;
	TimerUnit TimerA;
	TimerUnit TimerB;

public:
	union 
	{
		uWORD ControlRegister;
		struct { uWORD RUN_A:1, LOOP_A:1, RUN_B:1, LOOP_B:1;};
	};

	TimerChip(Devon::CPU & InCPU) : CPU(InCPU)
	{
		Reset();
	}

	void Tick()
	{
		if(RUN_A)
		{
			if(TimerA.Value.l == 0)
			{
				CPU.Interrupt(4);
				if(LOOP_A)
					TimerA.Value.l = TimerA.Base.l;
				else
					RUN_A = 0;
			}
			else
			{
				TimerA.Value.l--;
			}
		}

		if(RUN_B)
		{
			if(TimerB.Value.l == 0)
			{
				CPU.Interrupt(3);
				if(LOOP_B)
					TimerB.Value.l = TimerB.Base.l;
				else
					RUN_B = 0;
			}
			else
			{
				TimerB.Value.l--;
			}
		}
	}

	void Reset()
	{
		ControlRegister = 0;
		TimerA.Base.l = 0; 
		TimerA.Value.l = 0;
		TimerB.Base.l = 0; 
		TimerB.Value.l = 0;
	}
};