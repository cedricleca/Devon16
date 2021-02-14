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

	union FilterReg
	{
		uWORD uw;
		struct {uWORD Freq:8, Reso:8;} flags;
	};

	struct ChannelControl
	{
		sWORD Out;
		FilterReg Filter;
		sWORD PreModOffset;
		OscillatorControl Oscillator[2];

		// filter stuff
		float az1 = 0.f;
		float az2 = 0.f;
		float az3 = 0.f;
		float az4 = 0.f;
		float az5 = 0.f;
		float ay1 = 0.f;
		float ay2 = 0.f;
		float ay3 = 0.f;
		float ay4 = 0.f;
		float amf = 0.f;
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

		for(auto & Chan : Channel)
			for(auto & Osc : Chan.Oscillator)
				Osc.CurOffset += Osc.OscStep.l * UnrenderedCount;

		UnrenderedCount = 0;
		RenderTimer += JKevRenderPeriod;

		for(auto & Chan : Channel)
		{
			auto WaveForm = [](const OscillatorControl & OscControl) -> float
			{
				float Amp = (float(OscControl.WaveAmplitude.flags.Amplitude) - 128.f) / 128.f;

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

			const float W0 = WaveForm(Chan.Oscillator[0]);
			const float W1 = WaveForm(Chan.Oscillator[1]);
			const float out = std::clamp(Chan.PreModOffset + W1, -128.f, 127.f) * W0 / 256.f; // 8b range
			Chan.Out = sWORD(128.f * ResoFilter(Chan, out / 128.f, float(Chan.Filter.flags.Freq) / 256.f, float(Chan.Filter.flags.Reso) / 255.f));
		}

		if(OutputBuffer)
		{
			Push(std::clamp(Channel[0].Out + Channel[1].Out, -128, 127)); // R
			Push(std::clamp(Channel[2].Out + Channel[3].Out, -128, 127)); // L
		}
	}

	float ResoFilter(ChannelControl & Chan, float Input, float Cutoff, float Resonance) 
	{
		// filter based on the text "Non linear digital implementation of the moog ladder filter" by Antti Houvilainen
		// adopted from Csound code at http://www.kunstmusik.com/udo/cache/moogladder.udo

		// resonance [0..1]
		// cutoff from 0 (0Hz) to 1 (nyquist)

		const float v2 = 40000.f;   // twice the 'thermal voltage of a transistor'
		static const float sr = 22050.f;
		const float cutoff_hz = Cutoff * sr;
		const float kfc = cutoff_hz / sr; // sr is half the actual filter sampling rate
		const float kf = .5f * kfc;
		
		// frequency & amplitude correction
		const float kfcr = 1.8730f*kfc*kfc*kfc + 0.4955f*kfc*kfc - 0.6490f*kfc + 0.9988f;
		const float kacr = -3.9364f*kfc*kfc    + 1.8409f*kfc       + 0.9968f;
		const float k2vg = v2*(1.f-expf(-2.0f * 3.1415926535f * kfcr * kf)); // filter tuning

		// cascade of 4 1st order sections
		auto F = [k2vg, v2](float & ay, float & az, float t)
		{
			ay  = az + k2vg * (tanhf(t/v2) - tanhf(az/v2));
			az  = ay;
		};

		auto Pass = [&]()
		{
			F(Chan.ay1, Chan.az1, Input - 4.f*Resonance*Chan.amf*kacr);
			F(Chan.ay2, Chan.az2, Chan.ay1);
			F(Chan.ay3, Chan.az3, Chan.ay2);
			F(Chan.ay4, Chan.az4, Chan.ay3);
			Chan.amf  = (Chan.ay4 + Chan.az5)*0.5f; // 1/2-sample delay for phase compensation
			Chan.az5  = Chan.ay4;
		};

		Pass();
		Pass();

		return Chan.amf;
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
			Channel[i].PreModOffset = 0;
			Channel[i].Filter.flags.Freq = 255;
			Channel[i].Filter.flags.Reso = 0;

			for(int j = 0; j < 2; j++)
			{
				Channel[i].Oscillator[j].WaveAmplitude.uw = 0;
				Channel[i].Oscillator[j].OscStep.l = 0;
				Channel[i].Oscillator[j].CurOffset = 0;
			}
		}
	}
};