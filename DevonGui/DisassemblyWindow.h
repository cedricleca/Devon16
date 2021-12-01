#pragma once

#include "..\devon\devonMMU.h"
#include <string>

struct DisassemblyWindow
{
    bool            Open=true;                                   // = true   // set to false when DrawWindow() was closed. ignore if not using DrawWindow
	int				NextJump;
	unsigned int	OldPC;

    DisassemblyWindow();
	void DrawWindow(const char* title, DevonMMU & MMU, Devon::CPU & CPU, uLONG base_display_addr = 0x0000);
	int DisassembleInstruction(std::string & out, uLONG addr, DevonMMU & MMU);
};
