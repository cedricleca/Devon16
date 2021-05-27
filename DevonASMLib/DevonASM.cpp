#pragma once

#include "DevonASM.h"
#include <fstream>
#include <string>

void DevonASM::Assembler::Reset()
{
	Symbols.clear();
	CodeChunks.clear();
	CodeChunks.push_back(CodeChunk()); // add first code chunk 
	IncludePathStack.clear();
	IntsToAdd.clear();

	LastInt = -1;
	CurAddress = 0;
	CurPass = 0;
	LastOPLong = false;
	LastCharAvailable = false;
	CurChunk = 0;
	NbErrors = 0;
	ROMExport = false;
}

void DevonASM::Assembler::Pass1()
{
	LastInt = -1;
	CurAddress = 0;
	Entry = 0;
	CurPass = 1;
	LastOPLong = false;
	LastCharAvailable = false;
	CurChunk = 0;
	IntsToAdd.clear();

	// reserve chunks data
	for(auto & chunck : CodeChunks)
	{
		chunck.Data.resize(chunck.WordSize);
		chunck.Cursor = 0;
	}
}

void DevonASM::Assembler::NewInstruction()
{
	LastAdmode0 = CPU::EAdMode::EAdModeMax;
	LastAdmode1 = CPU::EAdMode::EAdModeMax;
	LastOPLong = true;
	ExplicitShortOP = false;
	LastCharAvailable = false;
	CndP = false;
	CndN = false;
	CndZ = false;
	CndW = false;
	CndH = false;
	CndL = false;
}

int DevonASM::Assembler::GetInstructionSize()
{
	if (LastAdmode0 == CPU::EAdMode::Imm20
		|| LastAdmode0 == CPU::EAdMode::XEA20L
		|| LastAdmode0 == CPU::EAdMode::XEA20W
		)
		return 2;

	if (LastAdmode1 == CPU::EAdMode::XEA20L
		|| LastAdmode1 == CPU::EAdMode::XEA20W
		)
		return 2;

	return 1;
}

void DevonASM::Assembler::CheckAdModeOPRanges(uWORD & OP0, uWORD & OP1, bool NoImm)
{
	switch (LastAdmode)
	{
	case CPU::EAdMode::Imm4:
		if(NoImm)
		{
			ErrorMessage(ErrorForbiddenImmAdMode);// error
		}
		else
		{
			if (LastAdmodeOP < -8 || LastAdmodeOP > 7)
			{
				ErrorMessage(ErrorRange4);// error
			}
			OP0 = LastAdmodeOP & 0xF;
		}
		break;
	case CPU::EAdMode::Imm20:
	case CPU::EAdMode::XEA20W:
	case CPU::EAdMode::XEA20L:
		if (LastAdmode == CPU::EAdMode::Imm20 && NoImm)
		{
			ErrorMessage(ErrorForbiddenImmAdMode);// error
		}
		else
		{
			if (LastAdmodeOP < -(2 << 19) || LastAdmodeOP >(2 << 19) - 1)
			{
				ErrorMessage(ErrorRange20);// error
			}
			OP0 = LastAdmodeOP >> 16;
			OP1 = LastAdmodeOP & 0xFFFF;
		}

		if(!LastOPLong && LastAdmode == CPU::EAdMode::XEA20L)
			LastAdmode = CPU::EAdMode::XEA20W;
		break;
	case CPU::EAdMode::Reg:
	case CPU::EAdMode::XReg:
	case CPU::EAdMode::XRegDec:
	case CPU::EAdMode::XRegInc:
		if (LastAdmodeOP > 7)
		{
			ErrorMessage(ErrorRegIndex);
		}
		OP0 = LastAdmodeOP & 0x7;
		break;
	}
}

void DevonASM::Assembler::AssembleInstruction(int iM)
{
	DevonASM::EMnemonic M = (DevonASM::EMnemonic)iM;

	if(M == DevonASM::EMnemonic::SSAVE 
		|| M == DevonASM::EMnemonic::SLOAD
		|| M == DevonASM::EMnemonic::SSWAP
		|| M == DevonASM::EMnemonic::MOVI
		|| M == DevonASM::EMnemonic::ASLI
		|| M == DevonASM::EMnemonic::ASRI
		|| M == DevonASM::EMnemonic::LSLI
		|| M == DevonASM::EMnemonic::LSRI
		|| M == DevonASM::EMnemonic::ROLI
		|| M == DevonASM::EMnemonic::RORI
		|| M == DevonASM::EMnemonic::BCLRI
		|| M == DevonASM::EMnemonic::BSETI
		|| M == DevonASM::EMnemonic::BNOTI
		|| M == DevonASM::EMnemonic::BTSTI
		|| M == DevonASM::EMnemonic::BCPYI
		|| M == DevonASM::EMnemonic::INTMASK
		)
	{
		if (LastAdmode == CPU::EAdMode::Imm4 || LastAdmode == CPU::EAdMode::Imm20)
		{
			LastAdmode = CPU::EAdMode::Imm8;
			LastAdmode0 = CPU::EAdMode::Imm8;
		}
	}

	if (CurPass == 0)
	{
		if(M == DevonASM::EMnemonic::FADD
			|| M == DevonASM::EMnemonic::FCMP
			|| M == DevonASM::EMnemonic::FMOD
			|| M == DevonASM::EMnemonic::FMOV
			|| M == DevonASM::EMnemonic::FMUL
			|| M == DevonASM::EMnemonic::FPOW
			|| M == DevonASM::EMnemonic::FTOI
			|| M == DevonASM::EMnemonic::ITOF
			)
		{
			ErrorMessage(ErrorUnsupportedFloatOp);
		}
	}
	else if (CurPass == 1)
	{
		if(ExplicitShortOP) // .w asked
		{
			switch(M)
			{
				case DevonASM::EMnemonic::MOV:
				case DevonASM::EMnemonic::XOR:
				case DevonASM::EMnemonic::OR:
				case DevonASM::EMnemonic::AND:
				case DevonASM::EMnemonic::NOT:
				break;

				default:
				ErrorMessage(ErrorUnsupportedShortModifier);
				break;
			}
		}

		if(LastAdmode == CPU::EAdMode::XEA20L && !LastOPLong)
			LastAdmode = CPU::EAdMode::XEA20W;

		uWORD OP0 = 0;
		uWORD OP1 = 0;

		CPU::InstDecode Inst;
		Inst.Helper.Instruction = 0;
		switch(M)
		{
		case DevonASM::EMnemonic::MOVB:
			if(LastAdmode != CPU::EAdMode::XReg 
				&& LastAdmode != CPU::EAdMode::XRegInc
				&& LastAdmode != CPU::EAdMode::XRegDec
				)
			{
				ErrorMessage(ErrorForbiddenAdMode);// bad admode
			}
			LastOPLong = false;
			goto AssembleType0;
		case DevonASM::EMnemonic::ADD:
		case DevonASM::EMnemonic::MUL:
		case DevonASM::EMnemonic::SUB:
		case DevonASM::EMnemonic::CMP:
		case DevonASM::EMnemonic::DIV:
		case DevonASM::EMnemonic::MOD:
			LastOPLong = true;
		case DevonASM::EMnemonic::MOV:
		case DevonASM::EMnemonic::XOR:
		case DevonASM::EMnemonic::OR:
		case DevonASM::EMnemonic::AND:
AssembleType0:
			CheckAdModeOPRanges(OP0, OP1);
			Inst.Helper.Type0.OPC = Mnemonic2Opcode[M];
			Inst.Helper.Type0.DIR = LastDirection;
			Inst.Helper.Type0.REG = LastRegisterOp;
			Inst.Helper.Type0.AM = (LastDirection == CPU::EDirection::ToAdMode ? LastAdmode1 : LastAdmode0);
			if(Inst.Helper.Type0.AM == CPU::EAdMode::XEA20L && !LastOPLong)
				Inst.Helper.Type0.AM = CPU::EAdMode::XEA20W;
			if(Inst.Helper.Type0.AM == CPU::EAdMode::Reg
				|| Inst.Helper.Type0.AM == CPU::EAdMode::XReg
				|| Inst.Helper.Type0.AM == CPU::EAdMode::XRegInc
				|| Inst.Helper.Type0.AM == CPU::EAdMode::XRegDec
				)
			{
				Inst.Helper.Type0.OP = (LastOPLong ? 1 : 0);
				Inst.Helper.Type0.OP |= (OP0<<1);
			}
			else
			{
				Inst.Helper.Type0.OP = OP0;
			}
			break;
		
		case DevonASM::EMnemonic::SSAVE:
		case DevonASM::EMnemonic::SLOAD:
		case DevonASM::EMnemonic::SSWAP:
			if (LastAdmodeOP < 0 || LastAdmodeOP > 255)
				ErrorMessage(ErrorRange8);// error

			if(LastAdmode != CPU::EAdMode::Imm8)
				ErrorMessage(ErrorForbiddenAdMode);// bad admode

			Inst.Helper.Type1.OPC = Mnemonic2Opcode[M];
			Inst.Helper.Type1.RS = LastAdmodeOP;
			switch (M)
			{
			case DevonASM::EMnemonic::SSAVE:	Inst.Helper.Type1.M = CPU::ESOPMode::SSAVE;		break;
			case DevonASM::EMnemonic::SLOAD:	Inst.Helper.Type1.M = CPU::ESOPMode::SLOAD;		break;
			case DevonASM::EMnemonic::SSWAP:	Inst.Helper.Type1.M = CPU::ESOPMode::SSWAP;		break;
			}
			break;

		case DevonASM::EMnemonic::BSR:
		case DevonASM::EMnemonic::BRA:
			if(LastAdmode == CPU::EAdMode::Imm20 || LastAdmode == CPU::EAdMode::Imm4)
				LastAdmodeOP -= CurAddress;
		case DevonASM::EMnemonic::JSR:
		case DevonASM::EMnemonic::JMP:
			CheckAdModeOPRanges(OP0, OP1);
			Inst.Helper.Type4.OPC = Mnemonic2Opcode[M];
			Inst.Helper.Type4.CND = (CndP ? CPU::ECndFlag::P : 0) | (CndN ? CPU::ECndFlag::N : 0) | (CndZ ? CPU::ECndFlag::Z : 0);
			Inst.Helper.Type4.M = (M == DevonASM::EMnemonic::BSR || M == DevonASM::EMnemonic::BRA) ? 1 : 0;
			Inst.Helper.Type4.OP = OP0;
			Inst.Helper.Type4.AM = LastAdmode0;
			if(Inst.Helper.Type0.AM == CPU::EAdMode::Reg
				|| Inst.Helper.Type0.AM == CPU::EAdMode::XReg
				|| Inst.Helper.Type0.AM == CPU::EAdMode::XRegInc
				|| Inst.Helper.Type0.AM == CPU::EAdMode::XRegDec
				)
			{
				Inst.Helper.Type4.OP <<= 1;
				Inst.Helper.Type4.OP |= 1;
			}
			break;

		case DevonASM::EMnemonic::ASL:
		case DevonASM::EMnemonic::ASR:
		case DevonASM::EMnemonic::LSL:
		case DevonASM::EMnemonic::LSR:
		case DevonASM::EMnemonic::ROL:
		case DevonASM::EMnemonic::ROR:
		case DevonASM::EMnemonic::ROXL:
		case DevonASM::EMnemonic::ROXR:
			if (LastAdmode != CPU::EAdMode::Reg
				&& LastAdmode != CPU::EAdMode::XReg
				&& LastAdmode != CPU::EAdMode::XRegDec
				&& LastAdmode != CPU::EAdMode::XRegInc
				)
			{
				ErrorMessage(ErrorForbiddenAdMode);// bad admode
			}

			if (LastAdmodeOP < 0 || LastAdmodeOP > 7)
				ErrorMessage(ErrorRegIndex);

			Inst.Helper.Type5.OPC = Mnemonic2Opcode[M];
			Inst.Helper.Type5.AM = LastAdmode;
			Inst.Helper.Type5.REGA = LastAdmodeOP;
			Inst.Helper.Type5.REGB = LastRegisterOp;
			switch (M)
			{
			case DevonASM::EMnemonic::ASL:	Inst.Helper.Type5.M = CPU::EShiftMode::LSL;		break;
			case DevonASM::EMnemonic::ASR:	Inst.Helper.Type5.M = CPU::EShiftMode::ASR;		break;
			case DevonASM::EMnemonic::LSL:	Inst.Helper.Type5.M = CPU::EShiftMode::LSL;		break;
			case DevonASM::EMnemonic::LSR:	Inst.Helper.Type5.M = CPU::EShiftMode::LSR;		break;
			case DevonASM::EMnemonic::ROL:	Inst.Helper.Type5.M = CPU::EShiftMode::ROL;		break;
			case DevonASM::EMnemonic::ROR:	Inst.Helper.Type5.M = CPU::EShiftMode::ROR;		break;
			case DevonASM::EMnemonic::ROXL:	Inst.Helper.Type5.M = CPU::EShiftMode::ROXL;	break;
			case DevonASM::EMnemonic::ROXR:	Inst.Helper.Type5.M = CPU::EShiftMode::ROXR;	break;
			}
			break;

		case DevonASM::EMnemonic::ASLI:
		case DevonASM::EMnemonic::ASRI:
		case DevonASM::EMnemonic::LSLI:
		case DevonASM::EMnemonic::LSRI:
		case DevonASM::EMnemonic::ROLI:
		case DevonASM::EMnemonic::RORI:
		case DevonASM::EMnemonic::ROXLI:
		case DevonASM::EMnemonic::ROXRI:
			if (LastAdmode != CPU::EAdMode::Imm8)
				ErrorMessage(ErrorForbiddenAdMode);// bad admode

			if (LastAdmodeOP < 0 || LastAdmodeOP > 31)
				ErrorMessage(ErrorRange8);

			Inst.Helper.Type6.OPC = Mnemonic2Opcode[M];
			Inst.Helper.Type6.OP = LastAdmodeOP;
			Inst.Helper.Type6.REGA = LastRegisterOp;
			switch (M)
			{
			case DevonASM::EMnemonic::ASLI:		Inst.Helper.Type6.M = CPU::EShiftMode::LSL;		break;
			case DevonASM::EMnemonic::ASRI:		Inst.Helper.Type6.M = CPU::EShiftMode::ASR;		break;
			case DevonASM::EMnemonic::LSLI:		Inst.Helper.Type6.M = CPU::EShiftMode::LSL;		break;
			case DevonASM::EMnemonic::LSRI:		Inst.Helper.Type6.M = CPU::EShiftMode::LSR;		break;
			case DevonASM::EMnemonic::ROLI:		Inst.Helper.Type6.M = CPU::EShiftMode::ROL;		break;
			case DevonASM::EMnemonic::RORI:		Inst.Helper.Type6.M = CPU::EShiftMode::ROR;		break;
			case DevonASM::EMnemonic::ROXLI:	Inst.Helper.Type6.M = CPU::EShiftMode::ROXL;	break;
			case DevonASM::EMnemonic::ROXRI:	Inst.Helper.Type6.M = CPU::EShiftMode::ROXR;	break;
			}
			break;

		case DevonASM::EMnemonic::BCLR:
		case DevonASM::EMnemonic::BSET:
		case DevonASM::EMnemonic::BNOT:
		case DevonASM::EMnemonic::BTST:
		case DevonASM::EMnemonic::BCPY:
			if (LastAdmode != CPU::EAdMode::Reg
				&& LastAdmode != CPU::EAdMode::XReg
				&& LastAdmode != CPU::EAdMode::XRegDec
				&& LastAdmode != CPU::EAdMode::XRegInc
				)
			{
				ErrorMessage(ErrorForbiddenAdMode);// bad admode
			}

			if (LastAdmodeOP < 0 || LastAdmodeOP > 7)
				ErrorMessage(ErrorRegIndex);

			Inst.Helper.Type5.OPC = Mnemonic2Opcode[M];
			Inst.Helper.Type5.AM = LastAdmode;
			Inst.Helper.Type5.REGA = LastAdmodeOP;
			Inst.Helper.Type5.REGB = LastRegisterOp;
			switch (M)
			{
			case DevonASM::EMnemonic::BCLR:	Inst.Helper.Type5.M = CPU::EBOPMode::BCLR;		break;
			case DevonASM::EMnemonic::BSET:	Inst.Helper.Type5.M = CPU::EBOPMode::BSET;		break;
			case DevonASM::EMnemonic::BNOT:	Inst.Helper.Type5.M = CPU::EBOPMode::BNOT;		break;
			case DevonASM::EMnemonic::BTST:	Inst.Helper.Type5.M = CPU::EBOPMode::BTST;		break;
			case DevonASM::EMnemonic::BCPY:	Inst.Helper.Type5.M = CPU::EBOPMode::BCPY;		break;
			}
			break;

		case DevonASM::EMnemonic::BCLRI:
		case DevonASM::EMnemonic::BSETI:
		case DevonASM::EMnemonic::BNOTI:
		case DevonASM::EMnemonic::BTSTI:
		case DevonASM::EMnemonic::BCPYI:
			if (LastAdmode != CPU::EAdMode::Imm8)
				ErrorMessage(ErrorForbiddenAdMode);// bad admode

			if (LastAdmodeOP < 0 || LastAdmodeOP > 31)
				ErrorMessage(ErrorRange8);

			Inst.Helper.Type6.OPC = Mnemonic2Opcode[M];
			Inst.Helper.Type6.OP = LastAdmodeOP;
			Inst.Helper.Type6.REGA = LastRegisterOp;
			switch (M)
			{
			case DevonASM::EMnemonic::BCLRI:	Inst.Helper.Type6.M = CPU::EBOPMode::BCLR;		break;
			case DevonASM::EMnemonic::BSETI:	Inst.Helper.Type6.M = CPU::EBOPMode::BSET;		break;
			case DevonASM::EMnemonic::BNOTI:	Inst.Helper.Type6.M = CPU::EBOPMode::BNOT;		break;
			case DevonASM::EMnemonic::BTSTI:	Inst.Helper.Type6.M = CPU::EBOPMode::BTST;		break;
			case DevonASM::EMnemonic::BCPYI:	Inst.Helper.Type6.M = CPU::EBOPMode::BCPY;		break;
			}
			break;

		case DevonASM::EMnemonic::EXT4:
		case DevonASM::EMnemonic::EXT8:
		case DevonASM::EMnemonic::EXT16:
		case DevonASM::EMnemonic::EXT20:
			if (LastAdmode != CPU::EAdMode::Reg)
				ErrorMessage(ErrorForbiddenAdMode);// bad admode

			if (LastAdmodeOP < 0 || LastAdmodeOP > 7)
				ErrorMessage(ErrorRegIndex);

			Inst.Helper.Type11.POPC = -1;
			Inst.Helper.Type11.OPC = Mnemonic2Opcode[M] - CPU::EOpcode::Group1;
			Inst.Helper.Type11.REG = LastAdmodeOP;
			switch (M)
			{
			case DevonASM::EMnemonic::EXT4:		Inst.Helper.Type11.M = CPU::EEXTMode::EXT4;		break;
			case DevonASM::EMnemonic::EXT8:		Inst.Helper.Type11.M = CPU::EEXTMode::EXT8;		break;
			case DevonASM::EMnemonic::EXT16:	Inst.Helper.Type11.M = CPU::EEXTMode::EXT16;	break;
			case DevonASM::EMnemonic::EXT20:	Inst.Helper.Type11.M = CPU::EEXTMode::EXT20;	break;
			}
			break;

		case DevonASM::EMnemonic::VBASE:
		case DevonASM::EMnemonic::TRAP:
		case DevonASM::EMnemonic::NOT:
		case DevonASM::EMnemonic::NEG:
			if(M == DevonASM::EMnemonic::VBASE || M == DevonASM::EMnemonic::TRAP)
				CheckAdModeOPRanges(OP0, OP1);
			else
				CheckAdModeOPRanges(OP0, OP1, true);
			Inst.Helper.Type8.POPC = -1;
			Inst.Helper.Type8.OPC = Mnemonic2Opcode[M] - CPU::EOpcode::Group1;
			Inst.Helper.Type8.AM = LastAdmode;
			Inst.Helper.Type8.OP = OP0;

			if(M == DevonASM::EMnemonic::NOT || M == DevonASM::EMnemonic::NEG)
			{
				if (Inst.Helper.Type8.AM == CPU::EAdMode::Reg
					|| Inst.Helper.Type8.AM == CPU::EAdMode::XReg
					|| Inst.Helper.Type8.AM == CPU::EAdMode::XRegInc
					|| Inst.Helper.Type8.AM == CPU::EAdMode::XRegDec
					)
				{
					Inst.Helper.Type8.OP = (LastOPLong ? 1 : 0);
					Inst.Helper.Type8.OP |= (OP0 << 1);
				}
				else
				{
					Inst.Helper.Type8.OP = OP0;
				}
			}
			break;

		case DevonASM::EMnemonic::SWP:
			if (LastAdmode != CPU::EAdMode::Reg)
			{
				ErrorMessage(ErrorForbiddenAdMode);// bad admode
			}
			Inst.Helper.Type7.POPC = -1;
			Inst.Helper.Type7.OPC = Mnemonic2Opcode[M] - CPU::EOpcode::Group1;
			Inst.Helper.Type7.REG = LastAdmodeOP;
			Inst.Helper.Type7.LS = CndW;
			Inst.Helper.Type7.WHS = CndH;
			Inst.Helper.Type7.WLS = CndL;
			break;

		case DevonASM::EMnemonic::INTMASK:
			if (LastAdmode != CPU::EAdMode::Imm8)
				ErrorMessage(ErrorForbiddenAdMode);// bad admode

			if (LastAdmodeOP < 0 || LastAdmodeOP > 255)
				ErrorMessage(ErrorRange8);

			Inst.Helper.Type9.POPC = -1;
			Inst.Helper.Type9.OPC = Mnemonic2Opcode[M] - CPU::EOpcode::Group1;
			Inst.Helper.Type9.OP = LastAdmodeOP;
			break;

		case DevonASM::EMnemonic::MOVI:
			if (LastAdmode != CPU::EAdMode::Imm8)
				ErrorMessage(ErrorForbiddenAdMode);// bad admode

			if (LastAdmodeOP < -128 || LastAdmodeOP > 127)
				ErrorMessage(ErrorRange8);

			Inst.Helper.Type12.REG = LastRegisterOp;
			Inst.Helper.Type12.OPC = Mnemonic2Opcode[M];
			Inst.Helper.Type12.OP = LastAdmodeOP;
			break;

		case DevonASM::EMnemonic::HALT:
		case DevonASM::EMnemonic::RESET:
		case DevonASM::EMnemonic::RTS:
		case DevonASM::EMnemonic::RTE:
		case DevonASM::EMnemonic::NOP:
		case DevonASM::EMnemonic::CHKX:
		case DevonASM::EMnemonic::CHKV:
			Inst.Helper.Type10.POPC = -1;
			Inst.Helper.Type10.OPC = Mnemonic2Opcode[M] - CPU::EOpcode::Group2;
			break;
		}

		AddWord(Inst.Helper.Instruction);
		if (GetInstructionSize() == 2)
			AddWord(OP1);
	}
}

void DevonASM::Assembler::NextCodeChunk()
{
	CurAddress = LastInt;
	if (CurPass == 0)
	{
		size_t sz = CodeChunks.size();
		if (sz == 0 || CodeChunks[sz - 1].WordSize > 0)
		{
			CodeChunks.push_back(CodeChunk());
			sz++;
		}

		CodeChunks[sz - 1].BaseAddress = CurAddress;
	}
	else if (CurPass == 1)
	{
		if(CurChunk > 0 || CodeChunks[0].Cursor == CodeChunks[0].WordSize)
			CurChunk++;
	}
}

void DevonASM::Assembler::AdvanceAddress()
{
	if (CurPass == 0)
	{
		CurAddress += GetInstructionSize();
		CodeChunks.back().WordSize += GetInstructionSize();
	}
}

void DevonASM::Assembler::AddWord(uWORD w)
{
	CurAddress++;
	if (CurPass == 0)
		CodeChunks.back().WordSize++;
	else if (CurPass == 1)
		CodeChunks[CurChunk].Data[CodeChunks[CurChunk].Cursor++] = w;//((w>>8) & 0xFF) | (w<<8);   // big endian swap
}

void DevonASM::Assembler::AddByte(char b)
{
	if(!LastCharAvailable)
	{
		LastChar = b;
		LastCharAvailable = true;
	}
	else
	{
		AddWord((LastChar<<8) | (b & 0xFF));
		LastCharAvailable = false;
	}
}

void DevonASM::Assembler::ErrorMessage(EErrorCode ErrorCode)
{
	ErrorsThisLine.push_back(ErrorCode);
}

bool DevonASM::Assembler::DumpErrorsThisLine(std::string source, size_t line)
{
	for(const auto & err : ErrorsThisLine)
	{
		std::cout << source << "(" << line << ") ";

		switch(err)
		{
		case ErrorForbiddenAdMode:			std::cout << "Forbidden addressing mode.";	break;
		case ErrorForbiddenImmAdMode:		std::cout << "Immediate addressing mode not permitted.";	break;
		case ErrorRange4:					std::cout << "Short Immediate addressing mode range error.";	break;
		case ErrorRange8:					std::cout << "Immediate addressing mode range error.";	break;
		case ErrorRange20:					std::cout << "Long Immediate addressing mode range error.";	break;
		case ErrorRegIndex:					std::cout << "Bad register index.";	break;
		case ErrorUnknownSymbol:			std::cout << "Unknown symbol reference.";	break;
		case ErrorBadSWPFlag:				std::cout << "Invalid SWP flag.";	break;			
		case ErrorBadJmpCnd:				std::cout << "Invalid JMP/BRA condition.";	break;						
		case ErrorUnsupportedFloatOp:		std::cout << "Floating point operations are not supported on the current version of the chip.";	break;
		case ErrorBadAdMode:				std::cout << "Invalid addressing mode.";	break;
		case ErrorMalformedInstruction:		std::cout << "Malformed Line.";	break;
		case ErrorIncludeFileFailed:		std::cout << "Include file failed.";	break;
		case ErrorIncbinFileFailed:			std::cout << "Incbin file failed.";	break;
		case ErrorUnsupportedShortModifier:	std::cout << "Short modifier '.w' not supported on this instruction.";	break;
		}

		std::cout << '\n';
	}

	if(ErrorsThisLine.size() > 0)
	{
		NbErrors += (int)ErrorsThisLine.size();
		ErrorsThisLine.clear();
		return true;
	}

	return false;
}

void DevonASM::Assembler::AssemblyCompleteMessage()
{
	if(NbErrors < 2)
		std::cout << "Assembly complete. " << NbErrors << " error." << '\n';
	else
		std::cout << "Assembly complete. " << NbErrors << " errors." << '\n';
}

void DevonASM::Assembler::Incbin(const char * FileName)
{
	std::ifstream input( FileName, std::ios::binary );
	if(input.is_open())
	{
		std::vector<unsigned char> Buf = { std::istreambuf_iterator<char>(input), {} };
		for(const auto & byte : Buf)
			AddByte(byte);

		if(LastCharAvailable)
			AddByte(-1);
	}
	else
	{
		throw std::runtime_error("Can't open file " + std::string(FileName));
	}
}

std::string DevonASM::Assembler::FilePath(const std::string & FileName)
{
	size_t cur = FileName.size() - 1;
	for(; cur > 0 && FileName[cur] != '//' && FileName[cur] != '\\'; --cur);

	if(cur > 0)
		return FileName.substr(0, cur).append("//");
	else
		return "";
}

bool DevonASM::Assembler::AssembleFile(const char * FileName)
{
	Reset();

	std::cout << "Assembling "<<FileName<<"\n";
	std::cout << "Pass 0\n";

	IncludePathStack.push_back(FileName);

	try
	{
		pegtl::file_input<> in( FileName );

		// pass 0
		try
		{
			pegtl::parse< DevonASM::grammar, DevonASM::action >(in, *this);
		}
		catch (pegtl::parse_error & err)
		{
			std::cout << err.what();
			AssemblyCompleteMessage();
			return false;
		}
	}
	catch(std::exception & err)
	{
		std::cout << err.what() << '\n';
		return false;
	}

	if(NbErrors > 0)
	{
		AssemblyCompleteMessage();
		return false;
	}

	std::cout << "Pass 1\n";
	Pass1();

	pegtl::file_input<> in2( FileName );

	// pass 1
	try
	{
		pegtl::parse< DevonASM::grammar, DevonASM::action >(in2, *this);
	}
	catch (pegtl::parse_error & err)
	{
		std::cout << err.what();
		AssemblyCompleteMessage();
		return false;
	}

	AssemblyCompleteMessage();
	return 	NbErrors == 0;
}

bool DevonASM::Assembler::ExportROMFile(const char * FileName, int ROMBaseAddress, int MaxSize, int MinSize)
{
	// check chunk overlaps
	for (auto it = CodeChunks.begin(); it != CodeChunks.end(); ++it)
	{
		for (auto it2 = CodeChunks.begin(); it2 != CodeChunks.end(); ++it2)
		{
			if(it != it2)
			{
				if (it->BaseAddress >= it2->BaseAddress && it->BaseAddress < it2->BaseAddress + it2->WordSize)
				{
					// overlap error
					std::cout << "ExportROMFile : check chunk overlaps : overlap error\n";
					return false;
				}

				if (it->BaseAddress + it->WordSize > it2->BaseAddress && it->BaseAddress + it->WordSize < it2->BaseAddress + it2->WordSize)
				{
					// overlap error
					std::cout << "ExportROMFile : check chunk overlaps : overlap error\n";
					return false;
				}
			}
		}
	}

	// check size
	int highest = -1;
	int lowest = -1;
	for(const auto & chunk : CodeChunks)
	{
		if(lowest < 0 || chunk.BaseAddress < lowest)
			lowest = chunk.BaseAddress;

		if (highest < 0 || chunk.BaseAddress + chunk.WordSize > highest)
			highest = chunk.BaseAddress + chunk.WordSize;
	}

	if(lowest != ROMBaseAddress)
	{
		// bad base address
		std::cout << "ExportROMFile : bad base address\n";
		return false;
	}

	const int size = highest - lowest;
	if(size > MaxSize)
	{
		// too large for cartridge
		std::cout << "ExportROMFile : too large for cartridge\n";
		return false;
	}

	// save file
	try
	{
		std::ofstream out(FileName, std::ios_base::out | std::ios_base::binary);

		// write chunks
		int outsize = 0;
		for(size_t i = 0; i < CodeChunks.size(); i++)
		{
			const auto & chunk = CodeChunks[i];
			out.write((const char *)chunk.Data.data(), chunk.WordSize * sizeof(uWORD));
			outsize += chunk.WordSize;

			if(i < CodeChunks.size() - 1)
			{
				for(; outsize < CodeChunks[i+1].BaseAddress; outsize++)
					out.put('?').put('.');
			}
		}

		// write padding to power of 2 size
		int csize = MinSize;
		while(size > csize)
			csize *= 2;

		for(; outsize < csize; outsize++)
			out.put('|').put('_');

		out.close();
	}
	catch(std::exception & err)
	{
		std::cout << "ExportROMFile failed :\n";
		std::cout << err.what() << '\n';
		return false;
	}

	std::cout << "ExportROMFile : passed\n";
	return true;
}

void DevonASM::Assembler::ExportDEXFile(const char * FileName)
{
}
