#pragma once

#include <vector>
#include "devon.h"

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
	unsigned char KeyState[2][256];
	int CurKeyStateBuf = 0;
	int Toggles = 0;

public:
	KeyBChip();
	void PushKeyEvents(bool IncludeKeyDowns);
	uWORD PopKeyEvent();
	void Reset();
};