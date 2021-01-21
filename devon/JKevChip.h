#pragma once

#include "devon.h"
#include <algorithm>

using namespace Devon;

static const float JKevRenderPeriod = (16111616.0f / 44100.0f);
static const int JKevChannelNr = 4;

class JKevChip
{
	friend class DevonMMU;

	char * OutputBuffer;
	int OutputWriteIndex;
	int OutputReadIndex;
	int OutputBufferSize;

	union LongReg
	{
		uLONG l;

		struct
		{
			uWORD lsw;
			uWORD msw;
		} w;
	};

	union WaveAmplitudeReg
	{
		uWORD uw;
		struct {uWORD Amplitude:8, Waveform:2;} flags;
	};

	struct OscillatorControl
	{
		WaveAmplitudeReg WaveAmplitude;
		LongReg OscStep;
		uLONG CurOffset;
	};

	struct ChannelControl
	{
		sLONG Hold;
		sWORD Out;
		sWORD Smooth;
		sWORD PreModOffset;
		OscillatorControl Oscillator[2];
	};

	ChannelControl Channel[JKevChannelNr];

	float RenderTimer;
	uLONG UnrenderedCount;

public:
	JKevChip() : OutputBuffer(nullptr),
		OutputWriteIndex(0),
		OutputReadIndex(0),
		OutputBufferSize(0),
		RenderTimer(0.0f),
		UnrenderedCount(0)
	{
	}

	void Tick()
	{
		UnrenderedCount += 2;

		RenderTimer	-= 2.0f;
		if(RenderTimer >= 0.0f)
			return;

		for(int i = 0; i < JKevChannelNr; i++)
		{
			for(int j = 0; j < 2; j++)
				Channel[i].Oscillator[j].CurOffset += Channel[i].Oscillator[j].OscStep.l * UnrenderedCount;
		}

		UnrenderedCount = 0;
		RenderTimer += JKevRenderPeriod;

		for(int i = 0; i < JKevChannelNr; i++)
		{
			float OscOut[2];
			for(int j = 0; j < 2; j++)
			{
				switch(Channel[i].Oscillator[j].WaveAmplitude.flags.Waveform)
				{
				case 0:	
					OscOut[j] = (Channel[i].Oscillator[j].CurOffset > (0xFFFFffff>>1)) ? 127.0f : -128.0f;	
					break;
				case 1:	
					OscOut[j] = float(Channel[i].Oscillator[j].CurOffset>>24) - 128.0f;					
					break;
				case 2:	
					if(Channel[i].Oscillator[j].CurOffset > (0xFFFFffff>>1))
						OscOut[j] = float(Channel[i].Oscillator[j].CurOffset>>23) - 384.0f;
					else
						OscOut[j] = -float(Channel[i].Oscillator[j].CurOffset>>23) + 127.0f;
					break;
				case 3:	
					OscOut[j] = float(std::rand() & 0xff) - 128.0f;					
					break;
				}

				unsigned char Amplitude = Channel[i].Oscillator[j].WaveAmplitude.flags.Amplitude;
				OscOut[j] *= float(*((char*)&Amplitude)) / 127.0f;
			}

			float out = Channel[i].PreModOffset + OscOut[1];
			if(out > 127.0f)		out = 127.0f;
			else if(out < -128.0f)	out = -128.0f;
			out *= OscOut[0] * 512.0f;

			float Smooth = 2.0f * float(Channel[i].Smooth);
			float Hold = float(Channel[i].Hold);
			Hold = (out + Hold * Smooth) / (Smooth + 1.0f);
			Channel[i].Hold = int(Hold);
			Channel[i].Out = Channel[i].Hold>>16;

			/*
			Channel[i].Hold = (Channel[i].Hold << Channel[i].Smooth) - Channel[i].Hold;
			Channel[i].Hold += int(out * 65536.0f);
			Channel[i].Hold >>= Channel[i].Smooth;
			Channel[i].Out = Channel[i].Hold>>16;
			*/
		}

		if(OutputBuffer)
		{
			int outR = Channel[0].Out + Channel[1].Out;
			if(outR > 127)				Push(127);
			else if(outR < -128)		Push(-128);
			else						Push(outR);

			int outL = Channel[2].Out + Channel[3].Out;
			if(outL > 127)				Push(127);
			else if(outL < -128)		Push(-128);
			else						Push(outL);
		}
	}

	void SetOutputSurface(char * _OutputBuffer, int Size)
	{
		OutputBuffer = _OutputBuffer;
		OutputBufferSize = Size;
	}

	inline void Push(char Value)
	{
		if(OutputWriteIndex+1 == OutputReadIndex)
			return;

		OutputBuffer[OutputWriteIndex++] = Value;
		if(OutputWriteIndex >= OutputBufferSize)
			OutputWriteIndex = 0;
	}

	inline bool Pop(char & Out)
	{
		if(OutputReadIndex+1 == OutputWriteIndex)
			return false;

		Out = OutputBuffer[OutputReadIndex++];
		if(OutputReadIndex >= OutputBufferSize)
			OutputReadIndex = 0;

		return true;
	}

	void HardReset()
	{
		RenderTimer = JKevRenderPeriod;
		UnrenderedCount = 0;

		for(int i = 0; i < JKevChannelNr; i++)
		{
			Channel[i].Out = 0;
			Channel[i].Hold = 0;
			Channel[i].PreModOffset = 0;
			Channel[i].Smooth = 0;

			for(int j = 0; j < 2; j++)
			{
				Channel[i].Oscillator[j].WaveAmplitude.uw = 0;
				Channel[i].Oscillator[j].OscStep.l = 0;
				Channel[i].Oscillator[j].CurOffset = 0;
			}
		}
	}
};