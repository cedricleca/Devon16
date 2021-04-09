#pragma once

#include <vector>
#include "devon.h"
#include "windows.h"
#include "imgui.h"

using namespace Devon;

class KeyBChip
{
	friend class DevonMMU;

	static const int EventBufSize = 16;

	enum KeybType
	{
		QWERTY,
		AZERTY,
		QWERTZ,
		DVORAK
	};

	sWORD KeyEvents[EventBufSize];
	uWORD PushEventIndex;
	uWORD PopEventIndex;
	KeybType KeybType = KeybType::QWERTY;
	BYTE KeyState[2][256];
	int CurKeyStateBuf = 0;

public:
	KeyBChip()
	{
		Reset();

		union
		{
		  HKL hkl;
		  unsigned short T[4];
		} Keyb;

		Keyb.hkl = GetKeyboardLayout(0);
		switch(Keyb.T[1])
		{
			case 0x040c:
			case 0x080c:
			case 0x180C:
			case 0x0488:
			KeybType = KeybType::AZERTY;	break;

			case 0x140C:
			case 0x100C:
			case 0x0405:
			case 0x040E:
			case 0x0415:
			case 0x041B:
			case 0x0C07:
			case 0x0407:
			case 0x1407:
			case 0x1007:
			case 0x0807:
			KeybType = KeybType::QWERTZ;	break;

			case 0xf002:
			KeybType = KeybType::DVORAK;	break;

			default:
			KeybType = KeybType::QWERTY;	break;
		}
	}

	void PushKeyEvents()
	{
		auto PushEvent = [this](int key, uWORD Toggles)
		{
			auto TranslateKey = [](int key) -> int
			{
				switch(key)
				{
				case VK_OEM_1     :		return ';';
				case VK_OEM_PLUS  :		return '=';
				case VK_OEM_COMMA :		return ',';
				case VK_OEM_MINUS :		return '-';
				case VK_OEM_PERIOD:		return '.';
				case VK_OEM_2     :		return '/';
				case VK_OEM_3     :		return '`';
				case VK_OEM_4     :		return '[';
				case VK_OEM_5     :		return '\\';
				case VK_OEM_6     :		return ']';
		//		case VK_OEM_7     :		return '\'';
				default:				return key;
				}
			};

			if((KeyState[CurKeyStateBuf][key] & 0x80) && !(KeyState[1 - CurKeyStateBuf][key] & 0x80))
			{
				KeyEvents[PushEventIndex] = Toggles | (TranslateKey(key) & 0xff);
				PushEventIndex = (PushEventIndex + 1) % EventBufSize;
			}
			else if(!(KeyState[CurKeyStateBuf][key] & 0x80) && (KeyState[1 - CurKeyStateBuf][key] & 0x80))
			{
				KeyEvents[PushEventIndex] = 0x8000 | Toggles | (TranslateKey(key) & 0xff);
				PushEventIndex = (PushEventIndex + 1) % EventBufSize;
			}
		};

		CurKeyStateBuf = 1 - CurKeyStateBuf;
		if(GetKeyboardState(KeyState[CurKeyStateBuf]))
		{
			uWORD Toggles =	  ((KeyState[CurKeyStateBuf][VK_CAPITAL]>>7)<<8)
							| ((KeyState[CurKeyStateBuf][VK_NUMLOCK]>>7)<<9)
							| ((KeyState[CurKeyStateBuf][VK_SCROLL]>>7)<<10)
							| ((KeyState[CurKeyStateBuf][VK_INSERT]>>7)<<11)
							| ((KeyState[CurKeyStateBuf][VK_SHIFT]>>7)<<12)
							| ((KeyState[CurKeyStateBuf][VK_CONTROL]>>7)<<13)
							| ((KeyState[CurKeyStateBuf][VK_MENU]>>7)<<14);

			for(int key = VK_BACK; key <= VK_HELP; key++)
				PushEvent(key, Toggles);

			for(int key = '0'; key <= '9'; key++)
				PushEvent(key, Toggles);

			for(int key = 'A'; key <= 'Z'; key++)
				PushEvent(key, Toggles);

			for(int key = VK_NUMPAD0; key <= VK_F6; key++)
				PushEvent(key, Toggles);

			for(int key = VK_OEM_1; key <= VK_OEM_3; key++)
				PushEvent(key, Toggles);

			for(int key = VK_OEM_4; key <= VK_OEM_7; key++)
				PushEvent(key, Toggles);
		}
	}

	uWORD PopKeyEvent()
	{
		if(PopEventIndex == PushEventIndex)
			return 0;

		uWORD Ret = KeyEvents[PopEventIndex];
		PopEventIndex = (PopEventIndex + 1) % EventBufSize;
		return Ret;
	}

	void Reset()
	{
		PushEventIndex = PopEventIndex = 0;
		for(int i = 0; i < EventBufSize; i++)
			KeyEvents[i] = 0;
	}
};