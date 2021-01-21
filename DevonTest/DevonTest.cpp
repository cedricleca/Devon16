// DevonTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "devon.h"
#include "Cortico.h"
#include "devonMMU.h"

using namespace Devon;

int main(int argc, char * argv[])
{
	unsigned char * ROM = NULL;
	long ROMSize = 0;
	if (argc > 1)
	{
		FILE * f;
		fopen_s(&f, argv[1], "rb");
		if(f)
		{
			fseek(f, 0, SEEK_END);
			ROMSize = ftell(f);
			rewind(f);

			ROM = new unsigned char[ROMSize];
			fread_s(ROM, ROMSize, 1, ROMSize, f);
			fclose(f);
		}
	}

	DevonMMU MMU(1024, 1024);
	MMU.SetROM((uWORD*)ROM, ROMSize);
	Devon::CPU CPU = Devon::CPU(MMU);

	CPU.Tick();

	if(ROM != NULL)
		delete ROM;

	return 0;
}

