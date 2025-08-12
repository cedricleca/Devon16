#pragma once

#include "devon.h"
#include <algorithm>
#include <array>
#include <cmath>

using namespace Devon;

static const float JKevRenderPeriod = (16111616.0f / 44100.0f);
static const int JKevChannelNr = 4;

class JKevChip
{
	friend class DevonMMU;

	char * OutputBuffer = nullptr;
	int OutputWriteIndex = 0;
	int OutputReadIndex = 0;
	int OutputBufferSize = 0;

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

	union FilterReg
	{
		uWORD uw;
		struct {uWORD Drive:4, Damp:4, InvM:8;} flags;
	};

	struct ChannelControl
	{
		sWORD Out;
		FilterReg Filter;
		sWORD PreModOffset;
		OscillatorControl Oscillator[2];

		float v = 0.f;
		float InvM;
		float Fric;
		float T;
	};

	ChannelControl Channel[JKevChannelNr];

public:
	float RenderTimer = 0.f;

	struct 
	{
		std::array<float, 100> Data;
		int OscilloWriteCursor = 0;
		void Push(float In) { Data[OscilloWriteCursor] = In;  OscilloWriteCursor = (OscilloWriteCursor+1) % Data.size(); }
	} OscilloTab[4];

	void Tick()
	{
		RenderTimer += JKevRenderPeriod;

		for(auto & Chan : Channel)
		{
			auto WaveForm = [](const OscillatorControl & OscControl) -> float
			{
				float Amp = float(OscControl.WaveAmplitude.flags.Amplitude) / 256.f;

				switch(OscControl.WaveAmplitude.flags.Waveform)
				{
				case 1:		return Amp * (float(OscControl.CurOffset>>24) - 128.0f);
				case 2:
					if(OscControl.CurOffset > (0xFFFFffff>>1))
						return Amp * (float(OscControl.CurOffset>>23) - 384.0f);
					else
						return Amp * (-float(OscControl.CurOffset>>23) + 127.0f);
				case 3:		return Amp * (float(std::rand() & 0xff) - 128.0f);
				default:	return Amp * ((OscControl.CurOffset > (0xFFFFffff>>1)) ? 127.0f : -128.0f);	
				}
			};

			Chan.Oscillator[0].CurOffset += Chan.Oscillator[0].OscStep.l * uLONG(JKevRenderPeriod); // losing sme precision on the period here, for optim sake
			Chan.Oscillator[1].CurOffset += Chan.Oscillator[1].OscStep.l * uLONG(JKevRenderPeriod);
			const float W0 = WaveForm(Chan.Oscillator[0]);
			const float W1 = WaveForm(Chan.Oscillator[1]);
			const float out = std::clamp(Chan.PreModOffset + W1, -128.f, 127.f) * W0 / 256.f; // 8b range

			Chan.v += (Chan.T * (out - float(Chan.Out)) - Chan.v * Chan.Fric) * Chan.InvM;
			Chan.Out = sWORD(std::clamp(float(Chan.Out) + Chan.v, -128.f, 127.f));
		}

		OscilloTab[0].Push(Channel[0].Out);
		OscilloTab[1].Push(Channel[1].Out);
		OscilloTab[2].Push(Channel[2].Out);
		OscilloTab[3].Push(Channel[3].Out);

		if(OutputBuffer)
		{
			int mixR = std::clamp<int>(Channel[0].Out + Channel[1].Out, -128, 127);
			int mixL = std::clamp<int>(Channel[2].Out + Channel[3].Out, -128, 127);
			Push(static_cast<char>(mixR)); // R
			Push(static_cast<char>(mixL)); // L
		}
	}

	void SetFilterReg(int ChanIdx, uWORD val)
	{
		Channel[ChanIdx].Filter.uw = val;
		const float F = float(Channel[ChanIdx].Filter.flags.InvM) / 255.f;
		const float C = float(Channel[ChanIdx].Filter.flags.Damp) / 15.f;
		const float T = float(Channel[ChanIdx].Filter.flags.Drive) / 15.f;
		Channel[ChanIdx].InvM = 1.f / (10000.f * F*F*F + 8.f); // F : 0
		Channel[ChanIdx].Fric = 1.f / (.15f + 8.f * (1.f-C) * (1.f-C)); // C : 0
		Channel[ChanIdx].T = 12.f * T*T; // D : 0.5
	}

	void SetOutputSurface(char * _OutputBuffer, int Size)
	{
		OutputBuffer = _OutputBuffer;
		OutputBufferSize = Size;
		OutputWriteIndex = 0;
		OutputReadIndex  = 0;
	}

	inline void Push(char Value)
	{
		if(!OutputBuffer || OutputBufferSize <= 1)
			return;

		const int next = (OutputWriteIndex + 1) % OutputBufferSize;        // proper wrap
		if(next == OutputReadIndex)
			return;                          // buffer full -> drop

		OutputBuffer[OutputWriteIndex] = Value;
		OutputWriteIndex = next;
	}

	inline bool Pop(char & Out)
	{
		if(!OutputBuffer)
			return false;

		if(OutputReadIndex == OutputWriteIndex)
			return false;        // empty

		Out = OutputBuffer[OutputReadIndex];
		OutputReadIndex = (OutputReadIndex + 1) % OutputBufferSize;   // wrap
		return true;
	}

	void HardReset()
	{
		RenderTimer = JKevRenderPeriod;

		for(auto & chan : Channel)
		{
			chan.Out = 0;
			chan.PreModOffset = 0;
			chan.Filter.flags.InvM = 0;
			chan.Filter.flags.Damp = 15;
			chan.Filter.flags.Drive = 8;
			chan.v = 0.f;

			for(auto & osc : chan.Oscillator)
			{
				osc.WaveAmplitude.uw = 0;
				osc.OscStep.l = 0;
				osc.CurOffset = 0;
			}
		}

		SetFilterReg(0, Channel[0].Filter.uw);
		SetFilterReg(1, Channel[1].Filter.uw);
		SetFilterReg(2, Channel[2].Filter.uw);
		SetFilterReg(3, Channel[3].Filter.uw);
	}
};