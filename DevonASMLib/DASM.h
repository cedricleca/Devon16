#pragma once

#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <stdio.h>
#include "../devon/devon.h"

using namespace Devon;

namespace DevonASM
{
	enum EErrorCode
	{
		ErrorForbiddenAdMode,
		ErrorForbiddenImmAdMode,
		ErrorRange4,
		ErrorRange8,
		ErrorRange20,
		ErrorRegIndex,
		ErrorUnknownSymbol,
		ErrorBadSWPFlag,
		ErrorBadJmpCnd,
		ErrorBadAdMode,
		ErrorUnsupportedFloatOp,
		ErrorMalformedInstruction,
		ErrorIncludeFileFailed,
		ErrorIncbinFileFailed,
		ErrorUnsupportedShortModifier,
	};

	struct CodeChunk
	{
		int BaseAddress = 0;
		int WordSize = 0;
		std::vector<unsigned short> Data;
		int Cursor = 0;
	};

	class Assembler
	{
	public:
		int LastInt = -1;
		char LastChar = false;
		bool LastCharAvailable = false;
		bool LastOPLong = false;
		bool ExplicitShortOP;
		std::string LastSymbol;
		CPU::EAdMode LastAdmode;
		CPU::EAdMode LastAdmode0;
		CPU::EAdMode LastAdmode1;
		int LastAdmodeOP;
		CPU::EDirection LastDirection;
		int LastRegisterOp;
		bool CndP;
		bool CndN;
		bool CndZ;
		bool CndW;
		bool CndH;
		bool CndL;

		bool ROMExport;
		int NbErrors = 0;
		int Entry = 0;
		int CurAddress = 0;
		int CurPass = 0;
		int CurChunk;
		std::map<std::string, int> Symbols;
		std::map<std::string, int>::iterator SymbolsIt;
		std::string LastFileName;
		std::vector<std::string> IncludePathStack;

		std::vector<CodeChunk> CodeChunks;
		std::vector<EErrorCode> ErrorsThisLine;

		void Reset();
		bool AssembleFile(const char * FileName);
		void ExportDEXFile(const char * FileName);
		bool ExportROMFile(const char * FileName, int ROMBaseAddress, int MaxSize=0x20000, int MinSize=0x400);
		void Pass1();
		void NextCodeChunk();
		void NewInstruction();
		void AddWord(uWORD w);
		void AddByte(char b);
		void Incbin(const char * FileName);
		void AdvanceAddress();
		int GetInstructionSize();
		void CheckAdModeOPRanges(uWORD & OP0, uWORD & OP1, bool NoImm=false);
		void AssembleInstruction(int M);
		void ErrorMessage(EErrorCode ErrorCode);
		void AssemblyCompleteMessage();
		bool DumpErrorsThisLine(std::string source, size_t line);
		static std::string FilePath(const std::string & FileName);
	};
};