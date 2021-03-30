#pragma once

#include "devon.h"

using namespace Devon;

class TimerChip
{
	friend class DevonMMU;

	union TimerRegister
	{
		uLONG l;

		struct
		{
			uWORD lsw;
			uWORD msw;
		} w;
	};

	struct TimerUnit
	{
		TimerRegister Base;
		TimerRegister Value;

		TimerUnit()
		{
			Base.l = 0; 
			Value.l = 0;
		}
	};

	Devon::CPU & CPU;
	TimerUnit Timer[2];

public:
	union 
	{
		uWORD ControlRegister;
		struct{ uWORD RUN_A:1, LOOP_A:1, RUN_B:1, LOOP_B:1;};
	};

	TimerChip(Devon::CPU & InCPU) : CPU(InCPU)
	{
		Reset();
	}

	void Tick()
	{
		if(RUN_A)
		{
			if(Timer[0].Value.l == 0)
			{
				CPU.Interrupt(4);
				if(LOOP_A)
					Timer[0].Value.l = Timer[0].Base.l;
				else
					RUN_A = 0;
			}
			else
			{
				Timer[0].Value.l--;
			}
		}

		if(RUN_B)
		{
			if(Timer[1].Value.l == 0)
			{
				CPU.Interrupt(3);
				if(LOOP_B)
					Timer[1].Value.l = Timer[1].Base.l;
				else
					RUN_B = 0;
			}
			else
			{
				Timer[1].Value.l--;
			}
		}
	}

	void Reset()
	{
		ControlRegister = 0;
		for(int i = 0; i < 2; i++)
		{
			Timer[i].Base.l = 0; 
			Timer[i].Value.l = 0;
		}
	}
};