// DevonASM.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include <iostream>
#include <stdio.h>

#include "DASM.h"

int main(int argc, char * argv[])
{
	try
	{
		if(argc > 1)
		{
			DevonASM::Assembler ASM = DevonASM::Assembler();
			ASM.AssembleFile(argv[1]);

			if(argc > 2)
			{
				if(0 == strncmp(argv[2], "-dca:", 5))
				{
					ASM.ExportROMFile(argv[2] + 5, 0x20000, 0x20000, 0x200);
					std::cout << "Done exporting cartridge file.\n";
				}
				else if(0 == strncmp(argv[2], "-dro:", 5))
				{
					ASM.ExportROMFile(argv[2] + 5, 0x0, 0x10000);
					std::cout << "Done exporting rom file.\n";
				}
				else
				{
					std::cout << "-dca:filename.das to export cartridge file\n";
					std::cout << "-dro:filename.dro to export rom file\n";
				}
			}
		}
	}
	catch(std::exception & err)
	{
		std::cout << err.what() << '\n';
		return false;
	}
}