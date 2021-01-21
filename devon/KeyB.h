#pragma once

#include "devon.h"
#include <imgui.h>

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

	int TranslateKey(int key)
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
	}

	void PushEvent(int key, uWORD Toggles)
	{
		if(ImGui::IsKeyPressed(key, false))
		{
			KeyEvents[PushEventIndex] = Toggles | (TranslateKey(key) & 0xff);
			PushEventIndex = (PushEventIndex + 1) % EventBufSize;
		}
		else if(ImGui::IsKeyReleased(key))
		{
			KeyEvents[PushEventIndex] = 0x8000 | Toggles | (TranslateKey(key) & 0xff);
			PushEventIndex = (PushEventIndex + 1) % EventBufSize;
		}
	}

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
		uWORD Toggles = ((GetKeyState(VK_CAPITAL) & 1)<<8)
				| ((GetKeyState(VK_NUMLOCK) & 1)<<9)
				| ((GetKeyState(VK_SCROLL) & 1)<<10)
				| ((GetKeyState(VK_INSERT) & 1)<<11)
				| ((GetKeyState(VK_SHIFT) < 0)<<12)
				| ((GetKeyState(VK_CONTROL) < 0)<<13)
				| ((GetKeyState(VK_MENU) < 0)<<14);

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