#pragma once

#include "devon.h"
#include "Cortico.h"
#include "Timers.h"
#include "MTUChip.h"
#include "JKevChip.h"
#include "KeyB.h"
#include <array>
#include <vector>
#include <memory>

//using namespace Devon;

class DevonMMU
{
	std::array<uWORD, 128*1024> RAMBuf;
	std::array<uWORD, 128*1024> GFXRAMBuf;
	std::vector<uWORD> ROMBuf;
	std::vector<uWORD> CARTBuf;

	void SetROMBuf(const std::vector<char> & InBuf, std::vector<uWORD> & Dest)
	{
		Dest.resize(InBuf.size() / sizeof(uWORD));
		memcpy_s(Dest.data(), InBuf.size(), InBuf.data(), InBuf.size());
	}

public:
	uWORD CycleCount = 0;
	uWORD GFXRAMAccessOccured:1, RAMAccessOccured:1, ROMAccessOccured:1;

	CorticoChip * Cortico = nullptr;
	TimerChip * Timers = nullptr;
	MTUChip * MTUs = nullptr;
	JKevChip * JKev = nullptr;
	KeyBChip * KeyB = nullptr;

	virtual void HardReset()
	{
		GFXRAMBuf.fill(0);
		RAMBuf.fill(0);
	}	

	void SetROM(const std::vector<char> & ROM)			{ SetROMBuf(ROM, ROMBuf); }
	void PlugCartrige(const std::vector<char> & CART)	{ SetROMBuf(CART, CARTBuf); }
	void UnplugCartridge() { CARTBuf.resize(0); }

	uWORD GFXReadWord(uLONG & Address) // use from cortico only
	{
		uWORD ret = GFXRAMBuf[(Address - 0x40000) & 0x1ffff];
		GFXRAMAccessOccured = true;
		if(++Address == 0x60000)
			Address = 0x40000;

		return ret;
	}

	enum MemAccessFail { AcceptFail, NoFail };
	template<MemAccessFail Fail = AcceptFail>
	EMemAck ReadWord(uWORD & Word, const uLONG Address)
	{
		const uLONG Page = Address>>16;
		switch(Page)
		{
		case 0x0: // ROM
			if constexpr (Fail == AcceptFail)
			{
				if(ROMAccessOccured)
					return WAIT;
				if((CycleCount & 3) != 0)
					return WAIT;
				ROMAccessOccured = true;
			}			

			{
				const uLONG Offset = Address;
				if(Offset >= ROMBuf.size() || ROMBuf.empty())
					return ERR;

				Word = ROMBuf[Offset];
			}
			break;

		case 0x1: // coprocessors control registers
			if constexpr (Fail == AcceptFail)
			{
				if((CycleCount & 1) != 0)
					return WAIT;
			}

			switch((Address>>12) & 0xF)
			{
			case 0x0:
				// MMU
				switch(Address & 0xFFF)
				{
				case 0x0:	Word = 0;							break;
				case 0x1:	Word = (uWORD)(ROMBuf.size()>>16);	break;
				case 0x2:	Word = (uWORD)ROMBuf.size();		break;
				case 0x3:	Word = (uWORD)(CARTBuf.size()>>16);	break;
				case 0x4:	Word = (uWORD)CARTBuf.size();		break;
				default:
					return ERR;
				}
				break;
			case 0x1:
				// JKev
				switch(Address & 0x3F)
				{
				case 0x00:	Word =	JKev->Channel[0].Oscillator[0].OscStep.w.msw;		break;
				case 0x01:	Word =	JKev->Channel[0].Oscillator[0].OscStep.w.lsw;		break;
				case 0x02:	Word =	JKev->Channel[0].Oscillator[0].WaveAmplitude.uw;	break;
				case 0x03:	Word =	JKev->Channel[0].Oscillator[1].OscStep.w.msw;		break;
				case 0x04:	Word =	JKev->Channel[0].Oscillator[1].OscStep.w.lsw;		break;
				case 0x05:	Word =	JKev->Channel[0].Oscillator[1].WaveAmplitude.uw;	break;
				case 0x06:	Word =	JKev->Channel[0].PreModOffset;						break;
				case 0x07:	Word =	JKev->Channel[0].Out;								break;
				case 0x08:	Word =	JKev->Channel[0].Filter.uw;							break;
				case 0x09:	Word =	JKev->Channel[1].Oscillator[0].OscStep.w.msw;		break;
				case 0x0A:	Word =	JKev->Channel[1].Oscillator[0].OscStep.w.lsw;		break;
				case 0x0B:	Word =	JKev->Channel[1].Oscillator[0].WaveAmplitude.uw;	break;
				case 0x0C:	Word =	JKev->Channel[1].Oscillator[1].OscStep.w.msw;		break;
				case 0x0D:	Word =	JKev->Channel[1].Oscillator[1].OscStep.w.lsw;		break;
				case 0x0E:	Word =	JKev->Channel[1].Oscillator[1].WaveAmplitude.uw;	break;
				case 0x0F:	Word =	JKev->Channel[1].PreModOffset;						break;
				case 0x10:	Word =	JKev->Channel[1].Out;								break;
				case 0x11:	Word =	JKev->Channel[1].Filter.uw;							break;
				case 0x12:	Word =	JKev->Channel[2].Oscillator[0].OscStep.w.msw;		break;
				case 0x13:	Word =	JKev->Channel[2].Oscillator[0].OscStep.w.lsw;		break;
				case 0x14:	Word =	JKev->Channel[2].Oscillator[0].WaveAmplitude.uw;	break;
				case 0x15:	Word =	JKev->Channel[2].Oscillator[1].OscStep.w.msw;		break;
				case 0x16:	Word =	JKev->Channel[2].Oscillator[1].OscStep.w.lsw;		break;
				case 0x17:	Word =	JKev->Channel[2].Oscillator[1].WaveAmplitude.uw;	break;
				case 0x18:	Word =	JKev->Channel[2].PreModOffset;						break;
				case 0x19:	Word =	JKev->Channel[2].Out;								break;
				case 0x1A:	Word =	JKev->Channel[2].Filter.uw;							break;
				case 0x1B:	Word =	JKev->Channel[3].Oscillator[0].OscStep.w.msw;		break;
				case 0x1C:	Word =	JKev->Channel[3].Oscillator[0].OscStep.w.lsw;		break;
				case 0x1D:	Word =	JKev->Channel[3].Oscillator[0].WaveAmplitude.uw;	break;
				case 0x1E:	Word =	JKev->Channel[3].Oscillator[1].OscStep.w.msw;		break;
				case 0x1F:	Word =	JKev->Channel[3].Oscillator[1].OscStep.w.lsw;		break;
				case 0x20:	Word =	JKev->Channel[3].Oscillator[1].WaveAmplitude.uw;	break;
				case 0x21:	Word =	JKev->Channel[3].PreModOffset;						break;
				case 0x22:	Word =	JKev->Channel[3].Out;								break;
				case 0x23:	Word =	JKev->Channel[3].Filter.uw;							break;
				default:
					return ERR;
				}
				break;
			case 0x2:
				// Cortico
				if constexpr (Fail == AcceptFail)
				{
					if(GFXRAMAccessOccured)
						return WAIT;
				}

				{
					uWORD SubAdd = Address & 0xFFF;
					switch(SubAdd)
					{
					case 0x00:	Word = Cortico->H;							break;
					case 0x01:	Word = Cortico->V;							break;
					case 0x02:	Word = Cortico->INT_H;						break;
					case 0x03:	Word = Cortico->INT_V;						break;
					case 0x04:	Word = Cortico->Control.w;					break;
					case 0x05:	Word = Cortico->BaseAdd[0].w.msw;			break;
					case 0x06:	Word = Cortico->BaseAdd[0].w.lsw;			break;
					case 0x07:	Word = Cortico->BPlane[0].CurAdd.w.msw;		break;
					case 0x08:	Word = Cortico->BPlane[0].CurAdd.w.lsw;		break;
					case 0x09:	Word = Cortico->Shift(0);					break;
					case 0x0A:	Word = Cortico->BPlane[0].Stride;			break;
					case 0x0B:	Word = Cortico->BPlane[0].VStart;			break;
					case 0x0C:	Word = Cortico->BPlane[0].VEnd;				break;
					case 0x0D:	Word = Cortico->mHStart.m256i_i32[0];		break;
					case 0x0E:	Word = Cortico->mHEnd.m256i_i32[0];			break;
					case 0x0F:	Word = Cortico->BaseAdd[1].w.msw;			break;
					case 0x10:	Word = Cortico->BaseAdd[1].w.lsw;			break;
					case 0x11:	Word = Cortico->BPlane[1].CurAdd.w.msw;		break;
					case 0x12:	Word = Cortico->BPlane[1].CurAdd.w.lsw;		break;
					case 0x13:	Word = Cortico->Shift(1);					break;
					case 0x14:	Word = Cortico->BPlane[1].Stride;			break;
					case 0x15:	Word = Cortico->BPlane[1].VStart;			break;
					case 0x16:	Word = Cortico->BPlane[1].VEnd;				break;
					case 0x17:	Word = Cortico->mHStart.m256i_i32[1];		break;
					case 0x18:	Word = Cortico->mHEnd.m256i_i32[1];			break;
					case 0x19:	Word = Cortico->BaseAdd[2].w.msw;			break;
					case 0x1A:	Word = Cortico->BaseAdd[2].w.lsw;			break;
					case 0x1B:	Word = Cortico->BPlane[2].CurAdd.w.msw;		break;
					case 0x1C:	Word = Cortico->BPlane[2].CurAdd.w.lsw;		break;
					case 0x1D:	Word = Cortico->Shift(2);					break;
					case 0x1E:	Word = Cortico->BPlane[2].Stride;			break;
					case 0x1F:	Word = Cortico->BPlane[2].VStart;			break;
					case 0x20:	Word = Cortico->BPlane[2].VEnd;				break;
					case 0x21:	Word = Cortico->mHStart.m256i_i32[2];		break;
					case 0x22:	Word = Cortico->mHEnd.m256i_i32[2];			break;
					case 0x23:	Word = Cortico->BaseAdd[3].w.msw;			break;
					case 0x24:	Word = Cortico->BaseAdd[3].w.lsw;			break;
					case 0x25:	Word = Cortico->BPlane[3].CurAdd.w.msw;		break;
					case 0x26:	Word = Cortico->BPlane[3].CurAdd.w.lsw;		break;
					case 0x27:	Word = Cortico->Shift(3);					break;
					case 0x28:	Word = Cortico->BPlane[3].Stride;			break;
					case 0x29:	Word = Cortico->BPlane[3].VStart;			break;
					case 0x2A:	Word = Cortico->BPlane[3].VEnd;				break;
					case 0x2B:	Word = Cortico->mHStart.m256i_i32[3];		break;
					case 0x2C:	Word = Cortico->mHEnd.m256i_i32[3];			break;
					case 0x2D:	Word = Cortico->BaseAdd[4].w.msw;			break;
					case 0x2E:	Word = Cortico->BaseAdd[4].w.lsw;			break;
					case 0x2F:	Word = Cortico->BPlane[4].CurAdd.w.msw;		break;
					case 0x30:	Word = Cortico->BPlane[4].CurAdd.w.lsw;		break;
					case 0x31:	Word = Cortico->Shift(4);					break;
					case 0x32:	Word = Cortico->BPlane[4].Stride;			break;
					case 0x33:	Word = Cortico->BPlane[4].VStart;			break;
					case 0x34:	Word = Cortico->BPlane[4].VEnd;				break;
					case 0x35:	Word = Cortico->mHStart.m256i_i32[4];		break;
					case 0x36:	Word = Cortico->mHEnd.m256i_i32[4];			break;
					case 0x37:	Word = Cortico->BaseAdd[5].w.msw;			break;
					case 0x38:	Word = Cortico->BaseAdd[5].w.lsw;			break;
					case 0x39:	Word = Cortico->BPlane[5].CurAdd.w.msw;		break;
					case 0x3A:	Word = Cortico->BPlane[5].CurAdd.w.lsw;		break;
					case 0x3B:	Word = Cortico->Shift(5);					break;
					case 0x3C:	Word = Cortico->BPlane[5].Stride;			break;
					case 0x3D:	Word = Cortico->BPlane[5].VStart;			break;
					case 0x3E:	Word = Cortico->BPlane[5].VEnd;				break;
					case 0x3F:	Word = Cortico->mHStart.m256i_i32[5];		break;
					case 0x40:	Word = Cortico->mHEnd.m256i_i32[5];			break;
					case 0x41:	Word = Cortico->BaseAdd[6].w.msw;			break;
					case 0x42:	Word = Cortico->BaseAdd[6].w.lsw;			break;
					case 0x43:	Word = Cortico->BPlane[6].CurAdd.w.msw;		break;
					case 0x44:	Word = Cortico->BPlane[6].CurAdd.w.lsw;		break;
					case 0x45:	Word = Cortico->Shift(6);					break;
					case 0x46:	Word = Cortico->BPlane[6].Stride;			break;
					case 0x47:	Word = Cortico->BPlane[6].VStart;			break;
					case 0x48:	Word = Cortico->BPlane[6].VEnd;				break;
					case 0x49:	Word = Cortico->mHStart.m256i_i32[6];		break;
					case 0x4A:	Word = Cortico->mHEnd.m256i_i32[6];			break;
					case 0x4B:	Word = Cortico->BaseAdd[7].w.msw;			break;
					case 0x4C:	Word = Cortico->BaseAdd[7].w.lsw;			break;
					case 0x4D:	Word = Cortico->BPlane[7].CurAdd.w.msw;		break;
					case 0x4E:	Word = Cortico->BPlane[7].CurAdd.w.lsw;		break;
					case 0x4F:	Word = Cortico->Shift(7);					break;
					case 0x50:	Word = Cortico->BPlane[7].Stride;			break;
					case 0x51:	Word = Cortico->BPlane[7].VStart;			break;
					case 0x52:	Word = Cortico->BPlane[7].VEnd;				break;
					case 0x53:	Word = Cortico->mHStart.m256i_i32[7];		break;
					case 0x54:	Word = Cortico->mHEnd.m256i_i32[7];			break;

					default:
						if(SubAdd >= 0x100 && SubAdd < 0x120)
							Word = Cortico->Clut[SubAdd-0x100].uw;
						else
							return ERR;
					}
				}
				break;
			case 0x3:
				// Keyb & GamePad
				switch(Address & 0xFFF)
				{
					case 0x0:	Word = KeyB->PopKeyEvent();			break;
					case 0x1:	Word = KeyB->KeybType;				break;
					default:
						return ERR;
				}
				break;
			case 0x4:
				// Timers
				switch(Address & 0xF)
				{
				case 0x0:	Word = Timers->ControlRegister;			break;
				case 0x1:	Word = Timers->TimerA.Base.w.msw;		break;
				case 0x2:	Word = Timers->TimerA.Base.w.lsw;		break;
				case 0x3:	Word = Timers->TimerA.Value.w.msw;		break;
				case 0x4:	Word = Timers->TimerA.Value.w.lsw;		break;
				case 0x5:	Word = Timers->TimerB.Base.w.msw;		break;
				case 0x6:	Word = Timers->TimerB.Base.w.lsw;		break;
				case 0x7:	Word = Timers->TimerB.Value.w.msw;		break;
				case 0x8:	Word = Timers->TimerB.Value.w.lsw;		break;
				default:
					return ERR;
				}
				break;
			case 0x5:
				// MTUs
				switch(Address & 0x1F)
				{
				case 0x00:	Word = MTUs->Control.w;								break;
				case 0x01:	Word = MTUs->MTUA.SrcPointer.WAddr.Page;			break;
				case 0x02:	Word = MTUs->MTUA.SrcPointer.WAddr.SubPageAddr;		break;
				case 0x03:	Word = MTUs->MTUA.DstPointer.WAddr.Page;			break;
				case 0x04:	Word = MTUs->MTUA.DstPointer.WAddr.SubPageAddr;		break;
				case 0x05:	Word = MTUs->MTUA.SrcStride;						break;
				case 0x06:	Word = MTUs->MTUA.DstStride;						break;
				case 0x07:	Word = MTUs->MTUA.Width;							break;
				case 0x08:	Word = MTUs->MTUA.Size;								break;
				case 0x09:	Word = MTUs->MTUB.SrcPointer.WAddr.Page;			break;
				case 0x0A:	Word = MTUs->MTUB.SrcPointer.WAddr.SubPageAddr;		break;
				case 0x0B:	Word = MTUs->MTUB.DstPointer.WAddr.Page;			break;
				case 0x0C:	Word = MTUs->MTUB.DstPointer.WAddr.SubPageAddr;		break;
				case 0x0D:	Word = MTUs->MTUB.SrcStride;						break;
				case 0x0E:	Word = MTUs->MTUB.DstStride;						break;
				case 0x0F:	Word = MTUs->MTUB.Width;							break;
				case 0x10:	Word = MTUs->MTUB.Size;								break;
				default:
					return ERR;
				}
				break;
			case 0x6:
				// Disk
				break;
			}
			break;

		case 0x2:
		case 0x3: // Cartridge
			if constexpr (Fail == AcceptFail)
			{
				if(ROMAccessOccured)
					return WAIT;
				if((CycleCount & 3) != 0)
					return WAIT;
				ROMAccessOccured = true;
			}			

			{
				const uLONG Offset = Address - 0x20000;
				if(Offset >= CARTBuf.size())
					return ERR;
				Word = CARTBuf[Offset];
			}
			break;

		case 0x4:
		case 0x5: // GFX RAM
			if constexpr (Fail == AcceptFail)
			{
				if(GFXRAMAccessOccured)
					return WAIT;
				if((CycleCount & 1) != 0)
					return WAIT;
				GFXRAMAccessOccured = true;
			}

			{
				const uLONG Offset = Address - 0x40000;
				if(Offset >= GFXRAMBuf.size())
					return ERR;
				Word = GFXRAMBuf[Offset];
			}
			break;


		case 0x8:
		case 0x9: // RAM
			if constexpr (Fail == AcceptFail)
			{
				if(RAMAccessOccured)
					return WAIT;
				if((CycleCount & 1) != 0)
					return WAIT;
				RAMAccessOccured = true;
			}			

			{
				const uLONG Offset = Address - 0x80000;
				if(Offset >= RAMBuf.size())
					return ERR;
				Word = RAMBuf[Offset];
			}
			break;

		//case 0x60000: // Extra GFX RAM
		//case 0x70000: // Extra GFX RAM
		//case 0xA0000: // Extra RAM
		//case 0xB0000: // Extra RAM
		default:
			return ERR;
		}

		return OK;
	}

	template<MemAccessFail Fail = AcceptFail>
	EMemAck WriteWord(const uWORD Word, const uLONG Address)
	{
		const uLONG Page = Address>>16;
		switch(Page)
		{
		case 0x1: // coprocessors control registers
			
			if constexpr (Fail == AcceptFail)
			if((CycleCount & 1) != 0)
				return WAIT;

			switch((Address>>12) & 0xF)
			{
			case 0x0:
				// MMU
				return ERR;
			case 0x1:
				// JKev
				switch(Address & 0x3F)
				{
				case 0x00:	JKev->Channel[0].Oscillator[0].OscStep.w.msw		= Word;		break;
				case 0x01:	JKev->Channel[0].Oscillator[0].OscStep.w.lsw		= Word;		break;
				case 0x02:	JKev->Channel[0].Oscillator[0].WaveAmplitude.uw		= Word;		break;
				case 0x03:	JKev->Channel[0].Oscillator[1].OscStep.w.msw		= Word;		break;
				case 0x04:	JKev->Channel[0].Oscillator[1].OscStep.w.lsw		= Word;		break;
				case 0x05:	JKev->Channel[0].Oscillator[1].WaveAmplitude.uw		= Word;		break;
				case 0x06:	JKev->Channel[0].PreModOffset						= Word;		break;
				case 0x08:	JKev->SetFilterReg(0, Word);									break;
				case 0x09:	JKev->Channel[1].Oscillator[0].OscStep.w.msw		= Word;		break;
				case 0x0A:	JKev->Channel[1].Oscillator[0].OscStep.w.lsw		= Word;		break;
				case 0x0B:	JKev->Channel[1].Oscillator[0].WaveAmplitude.uw		= Word;		break;
				case 0x0C:	JKev->Channel[1].Oscillator[1].OscStep.w.msw		= Word;		break;
				case 0x0D:	JKev->Channel[1].Oscillator[1].OscStep.w.lsw		= Word;		break;
				case 0x0E:	JKev->Channel[1].Oscillator[1].WaveAmplitude.uw		= Word;		break;
				case 0x0F:	JKev->Channel[1].PreModOffset						= Word;		break;
				case 0x11:	JKev->SetFilterReg(1, Word);									break;
				case 0x12:	JKev->Channel[2].Oscillator[0].OscStep.w.msw		= Word;		break;
				case 0x13:	JKev->Channel[2].Oscillator[0].OscStep.w.lsw		= Word;		break;
				case 0x14:	JKev->Channel[2].Oscillator[0].WaveAmplitude.uw		= Word;		break;
				case 0x15:	JKev->Channel[2].Oscillator[1].OscStep.w.msw		= Word;		break;
				case 0x16:	JKev->Channel[2].Oscillator[1].OscStep.w.lsw		= Word;		break;
				case 0x17:	JKev->Channel[2].Oscillator[1].WaveAmplitude.uw		= Word;		break;
				case 0x18:	JKev->Channel[2].PreModOffset						= Word;		break;
				case 0x1A:	JKev->SetFilterReg(2, Word);									break;
				case 0x1B:	JKev->Channel[3].Oscillator[0].OscStep.w.msw		= Word;		break;
				case 0x1C:	JKev->Channel[3].Oscillator[0].OscStep.w.lsw		= Word;		break;
				case 0x1D:	JKev->Channel[3].Oscillator[0].WaveAmplitude.uw		= Word;		break;
				case 0x1E:	JKev->Channel[3].Oscillator[1].OscStep.w.msw		= Word;		break;
				case 0x1F:	JKev->Channel[3].Oscillator[1].OscStep.w.lsw		= Word;		break;
				case 0x20:	JKev->Channel[3].Oscillator[1].WaveAmplitude.uw		= Word;		break;
				case 0x21:	JKev->Channel[3].PreModOffset						= Word;		break;
				case 0x23:	JKev->SetFilterReg(3, Word);									break;
				default:
					return ERR;
				}
				break;
			case 0x2:
				// Cortico
				if constexpr (Fail == AcceptFail)
				{
					if(GFXRAMAccessOccured)
						return WAIT;
				}

				{
					uWORD SubAdd = Address & 0xFFF;
					switch(SubAdd)
					{
					case 0x00:		return ERR;
					case 0x01:		return ERR;
					case 0x02:		Cortico->INT_H						= Word;		break;
					case 0x03:		Cortico->INT_V						= Word;		break;
					case 0x04:		Cortico->SetControlRegister(Word);				break;
					case 0x05:		Cortico->BaseAdd[0].w.msw			= Word;		break;
					case 0x06:		Cortico->BaseAdd[0].w.lsw			= Word;		break;
					case 0x07:		Cortico->BPlane[0].CurAdd.w.msw		= Word;		break;
					case 0x08:		Cortico->BPlane[0].CurAdd.w.lsw		= Word;		break;
					case 0x09:		Cortico->Shift(0, Word);						break;
					case 0x0A:		Cortico->BPlane[0].Stride			= Word;		break;
					case 0x0B:		Cortico->BPlane[0].VStart			= Word;		break;
					case 0x0C:		Cortico->BPlane[0].VEnd				= Word;		break;
					case 0x0D:		Cortico->mHStart.m256i_i32[0]		= Word;		break;
					case 0x0E:		Cortico->mHEnd.m256i_i32[0]			= Word;		break;
					case 0x0F:		Cortico->BaseAdd[1].w.msw			= Word;		break;
					case 0x10:		Cortico->BaseAdd[1].w.lsw			= Word;		break;
					case 0x11:		Cortico->BPlane[1].CurAdd.w.msw		= Word;		break;
					case 0x12:		Cortico->BPlane[1].CurAdd.w.lsw		= Word;		break;
					case 0x13:		Cortico->Shift(1, Word);						break;
					case 0x14:		Cortico->BPlane[1].Stride			= Word;		break;
					case 0x15:		Cortico->BPlane[1].VStart			= Word;		break;
					case 0x16:		Cortico->BPlane[1].VEnd				= Word;		break;
					case 0x17:		Cortico->mHStart.m256i_i32[1]		= Word;		break;
					case 0x18:		Cortico->mHEnd.m256i_i32[1]			= Word;		break;
					case 0x19:		Cortico->BaseAdd[2].w.msw			= Word;		break;
					case 0x1A:		Cortico->BaseAdd[2].w.lsw			= Word;		break;
					case 0x1B:		Cortico->BPlane[2].CurAdd.w.msw		= Word;		break;
					case 0x1C:		Cortico->BPlane[2].CurAdd.w.lsw		= Word;		break;
					case 0x1D:		Cortico->Shift(2, Word);						break;
					case 0x1E:		Cortico->BPlane[2].Stride			= Word;		break;
					case 0x1F:		Cortico->BPlane[2].VStart			= Word;		break;
					case 0x20:		Cortico->BPlane[2].VEnd				= Word;		break;
					case 0x21:		Cortico->mHStart.m256i_i32[2]		= Word;		break;
					case 0x22:		Cortico->mHEnd.m256i_i32[2]			= Word;		break;
					case 0x23:		Cortico->BaseAdd[3].w.msw			= Word;		break;
					case 0x24:		Cortico->BaseAdd[3].w.lsw			= Word;		break;
					case 0x25:		Cortico->BPlane[3].CurAdd.w.msw		= Word;		break;
					case 0x26:		Cortico->BPlane[3].CurAdd.w.lsw		= Word;		break;
					case 0x27:		Cortico->Shift(3, Word);						break;
					case 0x28:		Cortico->BPlane[3].Stride			= Word;		break;
					case 0x29:		Cortico->BPlane[3].VStart			= Word;		break;
					case 0x2A:		Cortico->BPlane[3].VEnd				= Word;		break;
					case 0x2B:		Cortico->mHStart.m256i_i32[3]		= Word;		break;
					case 0x2C:		Cortico->mHEnd.m256i_i32[3]			= Word;		break;
					case 0x2D:		Cortico->BaseAdd[4].w.msw			= Word;		break;
					case 0x2E:		Cortico->BaseAdd[4].w.lsw			= Word;		break;
					case 0x2F:		Cortico->BPlane[4].CurAdd.w.msw		= Word;		break;
					case 0x30:		Cortico->BPlane[4].CurAdd.w.lsw		= Word;		break;
					case 0x31:		Cortico->Shift(4, Word);						break;
					case 0x32:		Cortico->BPlane[4].Stride			= Word;		break;
					case 0x33:		Cortico->BPlane[4].VStart			= Word;		break;
					case 0x34:		Cortico->BPlane[4].VEnd				= Word;		break;
					case 0x35:		Cortico->mHStart.m256i_i32[4]		= Word;		break;
					case 0x36:		Cortico->mHEnd.m256i_i32[4]			= Word;		break;
					case 0x37:		Cortico->BaseAdd[5].w.msw			= Word;		break;
					case 0x38:		Cortico->BaseAdd[5].w.lsw			= Word;		break;
					case 0x39:		Cortico->BPlane[5].CurAdd.w.msw		= Word;		break;
					case 0x3A:		Cortico->BPlane[5].CurAdd.w.lsw		= Word;		break;
					case 0x3B:		Cortico->Shift(5, Word);						break;
					case 0x3C:		Cortico->BPlane[5].Stride			= Word;		break;
					case 0x3D:		Cortico->BPlane[5].VStart			= Word;		break;
					case 0x3E:		Cortico->BPlane[5].VEnd				= Word;		break;
					case 0x3F:		Cortico->mHStart.m256i_i32[5]		= Word;		break;
					case 0x40:		Cortico->mHEnd.m256i_i32[5]			= Word;		break;
					case 0x41:		Cortico->BaseAdd[6].w.msw			= Word;		break;
					case 0x42:		Cortico->BaseAdd[6].w.lsw			= Word;		break;
					case 0x43:		Cortico->BPlane[6].CurAdd.w.msw		= Word;		break;
					case 0x44:		Cortico->BPlane[6].CurAdd.w.lsw		= Word;		break;
					case 0x45:		Cortico->Shift(6, Word);						break;
					case 0x46:		Cortico->BPlane[6].Stride			= Word;		break;
					case 0x47:		Cortico->BPlane[6].VStart			= Word;		break;
					case 0x48:		Cortico->BPlane[6].VEnd				= Word;		break;
					case 0x49:		Cortico->mHStart.m256i_i32[6]		= Word;		break;
					case 0x4A:		Cortico->mHEnd.m256i_i32[6]			= Word;		break;
					case 0x4B:		Cortico->BaseAdd[7].w.msw			= Word;		break;
					case 0x4C:		Cortico->BaseAdd[7].w.lsw			= Word;		break;
					case 0x4D:		Cortico->BPlane[7].CurAdd.w.msw		= Word;		break;
					case 0x4E:		Cortico->BPlane[7].CurAdd.w.lsw		= Word;		break;
					case 0x4F:		Cortico->Shift(7, Word);						break;
					case 0x50:		Cortico->BPlane[7].Stride			= Word;		break;
					case 0x51:		Cortico->BPlane[7].VStart			= Word;		break;
					case 0x52:		Cortico->BPlane[7].VEnd				= Word;		break;
					case 0x53:		Cortico->mHStart.m256i_i32[7]		= Word;		break;
					case 0x54:		Cortico->mHEnd.m256i_i32[7]			= Word;		break;

					default:
						if(SubAdd >= 0x100 && SubAdd < 0x120)
							Cortico->SetClutEntry(SubAdd-0x100, Word);
						else
							return ERR;
					}
				}
				break;
			case 0x3:
				// Keyb & GamePad
				return ERR;
			case 0x4:
				// Timers
				switch(Address & 0xF)
				{
				case 0x0:	Timers->ControlRegister			= Word; 	break;
				case 0x1:	Timers->TimerA.Base.w.msw		= Word; 	break;
				case 0x2:	Timers->TimerA.Base.w.lsw		= Word; 	break;
				case 0x3:	Timers->TimerA.Value.w.msw		= Word;		break;
				case 0x4:	Timers->TimerA.Value.w.lsw		= Word;		break;
				case 0x5:	Timers->TimerB.Base.w.msw		= Word; 	break;
				case 0x6:	Timers->TimerB.Base.w.lsw		= Word; 	break;
				case 0x7:	Timers->TimerB.Value.w.msw		= Word;		break;
				case 0x8:	Timers->TimerB.Value.w.lsw		= Word;		break;
				default:
					return ERR;
				}
				break;
			case 0x5:
				// MTUs
				switch(Address & 0x1F)
				{
				case 0x00:	MTUs->Control.w							= Word;								break;
				case 0x01:	MTUs->MTUA.SetSrcPage(Word);												break;
				case 0x02:	MTUs->MTUA.SrcPointer.WAddr.SubPageAddr	= Word;								break;
				case 0x03:	MTUs->MTUA.SetDstPage(Word);												break;
				case 0x04:	MTUs->MTUA.DstPointer.WAddr.SubPageAddr	= Word;								break;
				case 0x05:	MTUs->MTUA.SrcStride					= Word;								break;
				case 0x06:	MTUs->MTUA.DstStride					= Word;								break;
				case 0x07:	MTUs->MTUA.Width						= Word;	MTUs->MTUA.CurrentX = 0;	break;
				case 0x08:	MTUs->MTUA.Size							= Word;	MTUs->MTUA.Counter = 0;		break;
				case 0x09:	MTUs->MTUB.SetSrcPage(Word);												break;
				case 0x0A:	MTUs->MTUB.SrcPointer.WAddr.SubPageAddr	= Word;								break;
				case 0x0B:	MTUs->MTUB.SetDstPage(Word);												break;
				case 0x0C:	MTUs->MTUB.DstPointer.WAddr.SubPageAddr	= Word;								break;
				case 0x0D:	MTUs->MTUB.SrcStride					= Word;								break;
				case 0x0E:	MTUs->MTUB.DstStride					= Word;								break;
				case 0x0F:	MTUs->MTUB.Width						= Word;	MTUs->MTUB.CurrentX = 0;	break;
				case 0x10:	MTUs->MTUB.Size							= Word;	MTUs->MTUB.Counter = 0;		break;
				default:
					return ERR;
				}
				break;
			case 0x6:
				// Disk
				break;
			}
			break;

		case 0x4:
		case 0x5: // GFX RAM
			if constexpr (Fail == AcceptFail)
			{
				if(GFXRAMAccessOccured || (CycleCount & 1) != 0)
					return WAIT;
				GFXRAMAccessOccured = true;
			}

			{
				const uLONG Offset = Address - 0x40000;
				if(Offset >= GFXRAMBuf.size())
					return ERR;
				GFXRAMBuf[Offset] = Word;
			}
			break;
		
		case 0x8:
		case 0x9: // RAM
			if constexpr (Fail == AcceptFail)
			{
				if(RAMAccessOccured || (CycleCount & 1) != 0)
					return WAIT;
				RAMAccessOccured = true;
			}			

			{
				const uLONG Offset = Address - 0x80000;
				if(Offset >= RAMBuf.size())
					return ERR;
				RAMBuf[Offset] = Word;
			}
			break;

		//case 0x00000: // ROM
		//case 0x20000: // Cartridge
		//case 0x30000: // Cartridge
		//case 0x60000: // Extra GFX RAM
		//case 0x70000: // Extra GFX RAM
		//case 0xA0000: // Extra RAM
		//case 0xB0000: // Extra RAM
		default:
			return ERR;
		}

		return OK;
	}
};
