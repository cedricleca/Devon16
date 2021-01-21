// DevonASM.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <string>
#include <iostream>
#include <stdio.h>

#include "DASM.h"

int main(int argc, char * argv[])
{
	if (argc > 1)
	{
		DevonASM::Assembler ASM = DevonASM::Assembler();
		ASM.AssembleFile(argv[1]);

		if (argc > 2 && 0 == strncmp(argv[2], "-dca:", 5))
		{
			ASM.ExportROMFile(argv[2] + 5, 0x20000, 0x20000, 0x200);
		}

		if (argc > 2 && 0 == strncmp(argv[2], "-dro:", 5))
		{
			ASM.ExportROMFile(argv[2] + 5, 0x0, 0x10000);
		}

		std::cin.get();
	}
}