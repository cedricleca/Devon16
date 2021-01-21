#pragma once

#include <dsound.h>
#include "..\devon\DevonMachine.h"

namespace DSoundTools
{
	void Init(HWND  hWnd, DevonMachine & Machine);
	void Render(DevonMachine & Machine, float Volume);
	void Release();
};
