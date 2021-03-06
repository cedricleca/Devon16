#include "devon.h"

using namespace Devon;

/*			Group0,
			ADD = Group0, MUL, SUB, DIV, MOD, CMP,
			MOV, MOVI, XOR, OR, AND, MOVB,
			SOP (SSAVE, SLOAD, SSWAP)
			FOP, FTOI, ITOF,
			JMP, JSR,
			SHIFT, SHIFTI, BOP, BOPI,

			Group1,
			SWP = Group1, EXT, TRAP, NOT, NEG,
			INTMASK, VBASE,
		
			Group2,
			HALT = Group2, RESET, RTS, RTE, NOP, CHKX, CHKV
*/

CPU::CPU(BaseMMU & InMMU) : MMU(InMMU)
{
	HardReset();
}

void CPU::HardReset()
{
	VectorTableBase = 0;

	Reset();
}

void CPU::Reset()
{
	// reset stuff
	IntMask = 0;
	SR.SR = 0;
	bHalt = false;
	PendingInterrupt = EVector::NoVector;
	StackingStatus = NoStacking;
	ExecCycleCount = -1; // execution over
	CurException = EVector::NoVector;
	Tick = &Devon::CPU::Tick_GetInst0;

	// reset registers
	for(int i = 0; i < NbRegs; i++)
	{
		R[i].u = 0;
		S[i].u = 0;
	}

	for(auto Entry : ICache_Add)
		Entry = -1;
	for(auto Entry : ICache_Inst0)
		Entry = -1;

	R[PC].u = 0;
	R[Dummy].u = 0;

	// load stack pointer
	MMU.ReadWord(R[SP].uw.msw, VectorTableBase + (ResetStack<<1), true);
	MMU.ReadWord(R[SP].uw.lsw, VectorTableBase + (ResetStack<<1)+1, true);

	// load program counter
	MMU.ReadWord(R[PC].uw.msw, VectorTableBase + (ResetPC<<1), true);
	MMU.ReadWord(R[PC].uw.lsw, VectorTableBase + (ResetPC<<1)+1, true);
}

void CPU::Halt(bool NewHalt)
{
	bHalt = NewHalt;
	bTrace = false;
}

void CPU::Trace()
{
	bTrace = true;
}

void CPU::Tick_Exception_ComputeVectorAdd()
{
	CurExceptionVectorAddress = VectorTableBase + (CurException<<1);
	Tick = &Devon::CPU::Tick_Exception_GetVector0;
}

void CPU::Tick_Exception_GetVector0()
{
	if(MReadWord(R[Dummy].uw.msw, CurExceptionVectorAddress))
		Tick = &Devon::CPU::Tick_Exception_GetVector1;
}

void CPU::Tick_Exception_GetVector1()
{
	if(MReadWord(R[Dummy].uw.lsw, CurExceptionVectorAddress+1))
		Tick = &Devon::CPU::Tick_Exception_StackRet0;
}

void CPU::Tick_Exception_StackRet0()
{
	if(MWriteWord(R[PC].u>>16, R[SP].u))
		Tick = &Devon::CPU::Tick_Exception_StackRet1;
}

void CPU::Tick_Exception_StackRet1()
{
	if(MWriteWord(R[PC].u, R[SP].u+1))
	{
		R[SP].u += 2;
		Tick = &Devon::CPU::Tick_Exception_StackSR;
	}
}

void CPU::Tick_Exception_StackSR()
{
	if(MWriteWord(SR.SR, R[SP].u))
	{
		R[SP].u += 1;
		Tick = &Devon::CPU::Tick_Exception_Jmp;
	}
}

void CPU::Tick_Exception_Jmp()
{
	R[PC].u = R[Dummy].u;
	Tick = &Devon::CPU::Tick_GetInst0;
	CurException = EVector::NoVector; // reset exception pipeline
}

void CPU::Tick_GetInst0()
{
	if(bHalt)
	{
		if(!bTrace)
			return;

		bTrace = false;
	}

	if(PendingInterrupt != NoVector)
	{
		Exception(PendingInterrupt);
		if(PendingInterrupt < EVector::Trap0 && PendingInterrupt >= EVector::ExternalInterrupt0)
			SR.Flags.IntLvl = 7 - (PendingInterrupt - EVector::ExternalInterrupt0);
		PendingInterrupt = NoVector;
		Tick_Exception_ComputeVectorAdd();
		return;
	}

	if(FetchInstruction())
	{
		if(FetchedInstruction.AdMode == Imm20 || FetchedInstruction.AdMode == XEA20W || FetchedInstruction.AdMode ==  XEA20L)
			Tick = &Devon::CPU::Tick_GetInst1;
		else
			Tick = &Devon::CPU::Tick_Decode;
	}
}

void CPU::Tick_GetInst1()
{
	if(FetchInstructionExtension())
		Tick = &Devon::CPU::Tick_Decode;
}

void CPU::Tick_PreDecrGet()
{
	R[ExecInstruction.Op].u--;
	Tick = &Devon::CPU::Tick_GetOp0;
}

void CPU::Tick_GetOp0()
{
	CurOPAddress = (ExecInstruction.AdMode == XEA20W || ExecInstruction.AdMode ==  XEA20L) ? ExecInstruction.Op : R[ExecInstruction.Op].u;
	if(ExecInstruction.Opcode == MOVB)
		CurOPAddress >>= 1;
	CurOPAddress &= 0xFFFFF;

	if(!ExecInstruction.LongOP)
	{
		if(MReadWord(R[Dummy].uw.lsw, CurOPAddress))
		{
			R[Dummy].uw.msw = 0;
			Tick = &Devon::CPU::Tick_Exec;
			if(ExecInstruction.AdMode == XRegInc)
				R[ExecInstruction.Op].u++;
		}
	}
	else if(MReadWord(R[Dummy].uw.msw, CurOPAddress))
	{
		Tick = &Devon::CPU::Tick_GetOp1;
	}
}

void CPU::Tick_GetOp1()
{
	if(MReadWord(R[Dummy].uw.lsw, CurOPAddress+1))
	{
		Tick = &Devon::CPU::Tick_Exec;
		if(ExecInstruction.AdMode == XRegInc)
			R[ExecInstruction.Op].u += 2;
	}
}

void CPU::Tick_Decode()
{
	ExecInstruction = FetchedInstruction;
	bInstructionPreFetched = false;
	bInstructionExtensionPreFetched = false;

	switch(ExecInstruction.Opcode)
	{
	case SWP:		ExecCycleCount = 3;		break;
	case EXT:		ExecCycleCount = 2;		break;
	case MOV:
	case MOVI:		ExecCycleCount = 0;		break;
	case SOP:
		if(ExecInstruction.Helper.Type1.M == SSWAP)
		{
			ExecCycleCount = 8;
			for(int j = 0; j < 8; j++)
			{
				if((ExecInstruction.Helper.Type1.RS & (1<<j)) != 0)
					ExecCycleCount += 2;
			}
		}
		break;
	case MUL:
		{
			int SrcBits = 0;
			int DstBits = 0;
			for(int i = 0; i < 16; i++)
			{
				SrcBits += ((R[ExecInstruction.SrcRegister].s & (1<<i)) != 0) ? 1 : 0;
				DstBits += ((R[ExecInstruction.DstRegister].s & (1<<i)) != 0) ? 1 : 0;
			}

			ExecCycleCount = 16 + ((SrcBits > DstBits ? DstBits : SrcBits) << 1);
		}
		break;
	case DIV:
		{
			int num_bits = 32;
			unsigned int remainder = 0;
			int dividend = R[ExecInstruction.DstRegister].u;
			while (remainder < R[ExecInstruction.SrcRegister].u) 
			{
				const int bit = (dividend & 0x80000000) >> 31;
				remainder = (remainder << 1) | bit;
				dividend = dividend << 1;
				num_bits--;
			}
			ExecCycleCount = 8 + (num_bits<<3);
		}
		break;
	default:
		ExecCycleCount = 1;
	}

	if(ExecInstruction.Dir == ToRegister)
	{
		ExecInstruction.DstRegister = ExecInstruction.Register;
		ExecInstruction.SrcRegister = (ExecInstruction.AdMode == Reg) ? ExecInstruction.Op : Dummy;
	}
	else
	{
		ExecInstruction.DstRegister = (ExecInstruction.AdMode == Reg) ? ExecInstruction.Op : Dummy;
		ExecInstruction.SrcRegister = ExecInstruction.Register;
	}

	Tick = &Devon::CPU::Tick_Exec;
	switch(ExecInstruction.AdMode)
	{
	case Imm4:
		R[Dummy].u = ExecInstruction.Op;
		if((ExecInstruction.Op & (1<<3)) != 0)
			R[Dummy].u |= 0xFFFFFFF0;
		break;
	case Imm8:
		R[Dummy].u = ExecInstruction.Op;
		if((ExecInstruction.Op & (1<<7)) != 0)
			R[Dummy].u |= 0xFFFFFF00;
		break;
	case Imm20:
		R[Dummy].u = ExecInstruction.Op;
		if((ExecInstruction.Op & (1<<19)) != 0)
			R[Dummy].u |= 0xFFF00000;
		break;
	case Reg:
	case EAdModeMax:
		break;
	default:
		if(FetchedInstruction.Opcode == MOV && FetchedInstruction.Dir == EDirection::ToAdMode)
			break;
			
		if(ExecInstruction.AdMode == XRegDec && ExecInstruction.Dir == ToRegister)
			Tick = &Devon::CPU::Tick_PreDecrGet;
		else
			Tick = &Devon::CPU::Tick_GetOp0;

	}

	bMemWriteOperand = !(ExecInstruction.Dir == EDirection::ToRegister
						|| ExecInstruction.AdMode == Reg
						|| ExecInstruction.AdMode == Imm20 // single op instructions
						|| ExecInstruction.AdMode == Imm4
						|| ExecInstruction.AdMode == Imm8
						|| ExecInstruction.AdMode == EAdModeMax
						|| ExecInstruction.Opcode == JMP
						|| ExecInstruction.Opcode == JSR
						|| ExecInstruction.Opcode == CMP
						);

	if(bMemWriteOperand)
	{
		CurOPAddress = (ExecInstruction.AdMode == XEA20W || ExecInstruction.AdMode ==  XEA20L) ? ExecInstruction.Op : R[ExecInstruction.Op].u;
		if(ExecInstruction.Opcode == MOVB)
			CurOPAddress >>= 1;
		CurOPAddress &= 0xFFFFF;
	}

	InstSize = (ExecInstruction.AdMode == Imm20 || ExecInstruction.AdMode == XEA20W || ExecInstruction.AdMode == XEA20L) ? 2 : 1;
}

void CPU::Tick_Exec()
{
	if(!bInstructionPreFetched)
		bInstructionPreFetched = FetchInstruction(InstSize);
	else if(FetchedInstruction.LongInst && !bInstructionExtensionPreFetched) 
		bInstructionExtensionPreFetched = FetchInstructionExtension(InstSize);

	if(ExecCycleCount > 0)
	{
		ExecCycleCount--;
		return;
	}
		
	// perform inst
	switch(ExecInstruction.Opcode)
	{
	case ADD:
		{
			bool TestC = R[ExecInstruction.DstRegister].s * R[ExecInstruction.SrcRegister].s >= 0;
			R[ExecInstruction.DstRegister].s += R[ExecInstruction.SrcRegister].s;
			TestReg(ExecInstruction.DstRegister);
			if(TestC)
				SR.Flags.V = (R[ExecInstruction.DstRegister].s * R[ExecInstruction.SrcRegister].s < 0) ? 1 : 0;
			else
				SR.Flags.V = 0;
		}

		break;
	case MUL:	
		R[ExecInstruction.DstRegister].s *= R[ExecInstruction.SrcRegister].s;
		TestReg(ExecInstruction.DstRegister);
		break;
	case SUB:
		{
			bool TestC = R[ExecInstruction.DstRegister].s * R[ExecInstruction.SrcRegister].s < 0;
			R[ExecInstruction.DstRegister].s -= R[ExecInstruction.SrcRegister].s;
			TestReg(ExecInstruction.DstRegister);
			if(TestC)
				SR.Flags.V = (R[ExecInstruction.DstRegister].s * R[ExecInstruction.SrcRegister].s > 0) ? 1 : 0;
			else
				SR.Flags.V = 0;
		}
		break;
	case DIV:	
		if(R[ExecInstruction.SrcRegister].s == 0)
		{
			Exception(EVector::DivideByZero);
			return;
		}
		R[ExecInstruction.DstRegister].s /= R[ExecInstruction.SrcRegister].s;
		TestReg(ExecInstruction.DstRegister);
		break;
	case MOD:
		if(R[ExecInstruction.SrcRegister].s == 0)
		{
			Exception(EVector::DivideByZero);
			return;
		}
		R[ExecInstruction.DstRegister].s %= R[ExecInstruction.SrcRegister].s;
		TestReg(ExecInstruction.DstRegister);
		break;
	case CMP:
		R[Dummy].s = R[ExecInstruction.DstRegister].s - R[ExecInstruction.SrcRegister].s;
		TestReg(Dummy);
		break;
	case MOV:	
		R[ExecInstruction.DstRegister].u = R[ExecInstruction.SrcRegister].u;
		if(ExecInstruction.AdMode == XRegInc && ExecInstruction.Dir == ToAdMode)
			R[ExecInstruction.Op].u += ExecInstruction.LongOP ? 2 : 1;
		break;
	case MOVI:
		R[ExecInstruction.DstRegister].u = R[ExecInstruction.SrcRegister].u;
		break;
	case XOR:	
		R[ExecInstruction.DstRegister].u ^= R[ExecInstruction.SrcRegister].u;
		if(ExecInstruction.LongOP)
			TestReg(ExecInstruction.DstRegister);
		break;
	case OR:	
		R[ExecInstruction.DstRegister].u |= R[ExecInstruction.SrcRegister].u;
		if(ExecInstruction.LongOP)
			TestReg(ExecInstruction.DstRegister);
		break;
	case AND:	
		R[ExecInstruction.DstRegister].u &= R[ExecInstruction.SrcRegister].u;
		if(ExecInstruction.LongOP)
			TestReg(ExecInstruction.DstRegister);
		break;
	case MOVB:
		if(ExecInstruction.Dir == ToRegister)
		{
			if((R[ExecInstruction.Op].u & 1) == (ExecInstruction.AdMode == XRegInc ? 0 : 1))
				R[ExecInstruction.DstRegister].uw.lsw = R[ExecInstruction.SrcRegister].u & 0xFF;
			else
				R[ExecInstruction.DstRegister].uw.lsw = (R[ExecInstruction.SrcRegister].u>>8) & 0xFF;
		}
		else
		{
			if((R[ExecInstruction.Op].u & 1) == (ExecInstruction.AdMode == XRegInc ? 0 : 1))
			{
				R[ExecInstruction.DstRegister].uw.lsw &= 0xFF00;
				R[ExecInstruction.DstRegister].uw.lsw |= R[ExecInstruction.SrcRegister].uw.lsw;
			}
			else
			{
				R[ExecInstruction.DstRegister].uw.lsw &= 0x00FF;
				R[ExecInstruction.DstRegister].uw.lsw |= R[ExecInstruction.SrcRegister].uw.lsw<<8;
			}
		}
		break;
	case SOP:
		for(int j = 0; j < 8; j++)
		{
			if((ExecInstruction.Helper.Type1.RS & (1<<j)) != 0)
			{
				switch(ExecInstruction.Helper.Type1.M)
				{
				case CPU::ESOPMode::SSAVE:	S[j].u = R[j].u;	break;
				case CPU::ESOPMode::SLOAD:	R[j].u = S[j].u;	break;
				case CPU::ESOPMode::SSWAP:
					const DataRegister Temp = S[j];
					S[j] = R[j];
					R[j] = Temp;
					break;
				}
			}
		}
		break;
	case JMP:
		if( (ExecInstruction.Helper.Type4.CND == 0)
			|| ((ExecInstruction.Helper.Type4.CND & ECndFlag::Z) != 0 && SR.Flags.Z)
			|| ((ExecInstruction.Helper.Type4.CND & ECndFlag::N) != 0 && SR.Flags.N)
			|| ((ExecInstruction.Helper.Type4.CND & ECndFlag::P) != 0 && !SR.Flags.Z && !SR.Flags.N)
			)
		{
			if(ExecInstruction.Helper.Type4.M != 0)
				R[PC].u += R[ExecInstruction.DstRegister].s;
			else
				R[PC].u = R[ExecInstruction.DstRegister].u & 0xFFFFF;

			R[PC].u -= InstSize; // To compensate because : Jump Occured 
			bInstructionPreFetched = false;
			bInstructionExtensionPreFetched = false;
		}
		break;
	case JSR:	
		if( (ExecInstruction.Helper.Type4.CND == 0)
			|| ((ExecInstruction.Helper.Type4.CND & ECndFlag::Z) != 0 && SR.Flags.Z)
			|| ((ExecInstruction.Helper.Type4.CND & ECndFlag::N) != 0 && SR.Flags.N)
			|| ((ExecInstruction.Helper.Type4.CND & ECndFlag::P) != 0 && !SR.Flags.Z && !SR.Flags.N)
			)
		{
			switch(StackingStatus)
			{
			case NoStacking:
				StackingStatus = StackingHi;
			case StackingHi:
				if(MWriteWord((((R[PC].u + InstSize)>>16) & 0xFFFF), R[SP].u))
				{
					R[SP].u++;
					StackingStatus = StackingLo;
				}
				return;

			case StackingLo:
				if(MWriteWord((R[PC].u + InstSize & 0xFFFF), R[SP].u))
				{
					R[SP].u++;
					StackingStatus = StackingOK;
				}
				return;
			}

			StackingStatus = NoStacking;

			if(ExecInstruction.Helper.Type4.M != 0)
				R[PC].u += R[ExecInstruction.DstRegister].s;
			else
				R[PC].u = R[ExecInstruction.DstRegister].u & 0xFFFFF;

			R[PC].u -= InstSize; // To compensate because : Jump Occured 
			bInstructionPreFetched = false;
			bInstructionExtensionPreFetched = false;
		}
		break;
	case SHIFT:
	case SHIFTI:
		{
			const int Shift = ExecInstruction.Opcode == SHIFT ? R[ExecInstruction.SrcRegister].u & 0x1F : ExecInstruction.Helper.Type6.OP;
			switch(ExecInstruction.Helper.Type5.M)
			{
			case CPU::EShiftMode::LSL:
				SR.Flags.X = (R[ExecInstruction.DstRegister].s < 0);
				R[ExecInstruction.DstRegister].u <<= Shift;
				break;
			case CPU::EShiftMode::ASR:
				SR.Flags.X = (R[ExecInstruction.DstRegister].u & 1) != 0;
				R[ExecInstruction.DstRegister].s >>= Shift;
				break;
			case CPU::EShiftMode::LSR:
				SR.Flags.X = (R[ExecInstruction.DstRegister].u & 1) != 0;
				R[ExecInstruction.DstRegister].u >>= Shift;
				break;
			case CPU::EShiftMode::ROL:
				R[ExecInstruction.DstRegister].u = (R[ExecInstruction.DstRegister].u << Shift) | (R[ExecInstruction.DstRegister].u >> (31-Shift));
				break;
			case CPU::EShiftMode::ROR:
				R[ExecInstruction.DstRegister].u = (R[ExecInstruction.DstRegister].u >> Shift) | (R[ExecInstruction.DstRegister].u << (31-Shift));
				break;
			case CPU::EShiftMode::ROXL:
				for(int i = 0; i < Shift; i++)
				{
					const bool tmp = (R[ExecInstruction.DstRegister].s < 0);
					R[ExecInstruction.DstRegister].u = (R[ExecInstruction.DstRegister].u << 1) | SR.Flags.X;
					SR.Flags.X = tmp;
				}
				break;
			case CPU::EShiftMode::ROXR:
				for(int i = 0; i < Shift; i++)
				{
					const bool tmp = (R[ExecInstruction.DstRegister].s < 0);
					R[ExecInstruction.DstRegister].u = (R[ExecInstruction.DstRegister].u >> 1) | (SR.Flags.X<<31);
					SR.Flags.X = tmp;
				}
				break;
			}
			TestReg(ExecInstruction.DstRegister);
		}
		break;
	case BOP:
	case BOPI:
		{
			const int Bit = ExecInstruction.Opcode == BOP ? R[ExecInstruction.SrcRegister].u & 0x1F : ExecInstruction.Helper.Type6.OP;
			switch(ExecInstruction.Helper.Type5.M)
			{
			case CPU::EBOPMode::BCLR:	
				R[ExecInstruction.DstRegister].u &= ~(1<<Bit);	
				TestReg(ExecInstruction.DstRegister);
				break;
			case CPU::EBOPMode::BSET:	
				R[ExecInstruction.DstRegister].u |= (1<<Bit);	
				TestReg(ExecInstruction.DstRegister);
				break;
			case CPU::EBOPMode::BTST:
				SR.Flags.Z = ((R[ExecInstruction.DstRegister].u & (1<<Bit)) == 0);
				SR.Flags.N = 0;
				break;
			case CPU::EBOPMode::BCPY:
				if(SR.Flags.Z)
					R[ExecInstruction.DstRegister].u |= (1<<Bit);
				else
					R[ExecInstruction.DstRegister].u &= ~(1<<Bit);
				TestReg(ExecInstruction.DstRegister);
				break;
			case CPU::EBOPMode::BNOT:
				if((R[ExecInstruction.DstRegister].u & (1<<Bit)) == 0)
					R[ExecInstruction.DstRegister].u |= (1<<Bit);
				else
					R[ExecInstruction.DstRegister].u &= ~(1<<Bit);
				TestReg(ExecInstruction.DstRegister);
				break;
			}
		}
		break;
	case SWP:
		if(ExecInstruction.Helper.Type7.LS)
			R[ExecInstruction.Helper.Type7.REG].u = ((R[ExecInstruction.Helper.Type7.REG].u>>16) & 0xFFFF) | R[ExecInstruction.Helper.Type7.REG].u<<16;
		if(ExecInstruction.Helper.Type7.WLS)
			R[ExecInstruction.Helper.Type7.REG].u = (R[ExecInstruction.Helper.Type7.REG].u & 0xFFFF0000) | ((R[ExecInstruction.Helper.Type7.REG].u>>8) & 0x00FF) | ((R[ExecInstruction.Helper.Type7.REG].u<<8) & 0xFF00);
		if(ExecInstruction.Helper.Type7.WHS)
			R[ExecInstruction.Helper.Type7.REG].u = (R[ExecInstruction.Helper.Type7.REG].u & 0x0000FFFF) | ((R[ExecInstruction.Helper.Type7.REG].u>>8) & 0x00FF0000) | ((R[ExecInstruction.Helper.Type7.REG].u<<8) & 0xFF000000);
		TestReg(ExecInstruction.Helper.Type7.REG);
		break;
	case EXT:
		switch(ExecInstruction.Helper.Type11.M)
		{
		case CPU::EEXTMode::EXT4:	
			if((R[ExecInstruction.Register].u & (1<<3)) != 0)
				R[ExecInstruction.Register].u |= 0xFFFFFFF0;
			else
				R[ExecInstruction.Register].u &= 0x0000000F;
			break;
		case CPU::EEXTMode::EXT8:
			if((R[ExecInstruction.Register].u & (1<<7)) != 0)
				R[ExecInstruction.Register].u |= 0xFFFFFF00;
			else
				R[ExecInstruction.Register].u &= 0x000000FF;
			break;
		case CPU::EEXTMode::EXT16:
			if((R[ExecInstruction.Register].u & (1<<15)) != 0)
				R[ExecInstruction.Register].u |= 0xFFFF0000;
			else
				R[ExecInstruction.Register].u &= 0x0000FFFF;
			break;
		case CPU::EEXTMode::EXT20:
			if((R[ExecInstruction.Register].u & (1<<19)) != 0)
				R[ExecInstruction.Register].u |= 0xFFF00000;
			else
				R[ExecInstruction.DstRegister].u &= 0x000FFFFF;
			break;
		}
		TestReg(ExecInstruction.Register);
		break;
	case TRAP:
		PendingInterrupt = (EVector)(R[ExecInstruction.SrcRegister].u + EVector::Trap0);
		bInstructionPreFetched = false;
		bInstructionExtensionPreFetched = false;
		break;
	case NOT:	R[ExecInstruction.SrcRegister].u = ~R[ExecInstruction.SrcRegister].u;		
		TestReg(ExecInstruction.DstRegister);
		break;					
	case NEG:	R[ExecInstruction.SrcRegister].s = -R[ExecInstruction.SrcRegister].s;	
		TestReg(ExecInstruction.SrcRegister);	
		break;
	case INTMASK:
		IntMask = ExecInstruction.Helper.Type9.OP;
		break;
	case VBASE:	VectorTableBase = R[ExecInstruction.DstRegister].u & 0xFFFFF;				
		break;
	case HALT:	Halt(true);																	
		break;
	case RESET:	Reset();																	
		break;
	case RTE:	
		switch(StackingStatus)
		{
		case NoStacking:
			StackingStatus = UnstackingSR;
			R[SP].u--;
			return;

		case UnstackingSR:
			if(MReadWord(SR.SR, R[SP].u))
				StackingStatus = NoStacking;
			break;
		}
	case RTS:
		switch(StackingStatus)
		{
		case NoStacking:
			R[SP].u--;
			StackingStatus = StackingLo;
			return;

		case StackingLo:
			if(MReadWord(R[PC].uw.lsw, R[SP].u))
			{
				R[SP].u--;
				StackingStatus = StackingHi;
			}
			return;

		case StackingHi:
			if(MReadWord(R[PC].uw.msw, R[SP].u))
				StackingStatus = StackingOK;
			return;

		case StackingOK:
			StackingStatus = NoStacking;
			R[PC].u -= InstSize; // To compensate because : Jump Occured
			bInstructionPreFetched = false;
			bInstructionExtensionPreFetched = false;
			break;

		default:
			return;
		}
		break;
	case NOP:
		break;
	case CHKX:
		SR.Flags.Z = SR.Flags.X;
		break;
	case CHKV:
		SR.Flags.Z = SR.Flags.V;
		break;
	default:
		Exception(EVector::BadInstruction);
		return;
	}

	if(bMemWriteOperand)
	{
		if(ExecInstruction.AdMode == XRegDec)
			Tick = &Devon::CPU::Tick_PreDecrPut;
		else
			Tick = &Devon::CPU::Tick_PutRes0;
	}
	else
	{
		R[PC].u += InstSize;

		if(!bInstructionPreFetched || bHalt)
		{
			Tick = &Devon::CPU::Tick_GetInst0;
			bInstructionPreFetched = false;
			bInstructionExtensionPreFetched = false;
		}
		else if(bInstructionExtensionPreFetched)
			Tick = &Devon::CPU::Tick_Decode;		
		else if(FetchedInstruction.LongInst)
			Tick = &Devon::CPU::Tick_GetInst1;
		else
			Tick = &Devon::CPU::Tick_Decode;
	}
}

void CPU::Tick_PreDecrPut()
{
	R[ExecInstruction.Op].u--;
	Tick = &Devon::CPU::Tick_PutRes0;
}

void CPU::Tick_PutRes0()
{
	if(!ExecInstruction.LongOP)
	{
		if(MWriteWord(R[Dummy].uw.lsw, CurOPAddress))
		{
			R[PC].u += InstSize;
			Tick = &Devon::CPU::Tick_GetInst0;
			bMemWriteOperand = false;
		}
	}
	else if(MWriteWord(R[Dummy].uw.msw, CurOPAddress))
	{
		Tick = &Devon::CPU::Tick_PutRes1;
	}
}

void CPU::Tick_PutRes1()
{
	if(MWriteWord(R[Dummy].uw.lsw, CurOPAddress+1))
	{
		R[PC].u += InstSize;
		Tick = &Devon::CPU::Tick_GetInst0;
		bMemWriteOperand = false;
	}
}

bool CPU::FetchInstructionExtension(const uLONG Offset /*= 0*/)
{
	uWORD OpW2;
	const uLONG Add = R[PC].u + Offset + 1;
	if(ICache_Add[0] == Add)		
		OpW2 = ICache_Inst0[0];
	else if(ICache_Add[1] == Add)		
		OpW2 = ICache_Inst0[1];
	else if(ICache_Add[2] == Add)		
		OpW2 = ICache_Inst0[2];
	else if(ICache_Add[3] == Add)		
		OpW2 = ICache_Inst0[3];
	else if(ICache_Add[4] == Add)		
		OpW2 = ICache_Inst0[4];
	else if(ICache_Add[5] == Add)		
		OpW2 = ICache_Inst0[5];
	else if(ICache_Add[6] == Add)		
		OpW2 = ICache_Inst0[6];
	else if(ICache_Add[7] == Add)		
		OpW2 = ICache_Inst0[7];
	else if(!MReadWord(OpW2, Add))		
		return false; // read fail/wait
	else 
	{
		CacheIdx = ++CacheIdx & 7;
		ICache_Add[CacheIdx] = Add;
		ICache_Inst0[CacheIdx] = OpW2;
	}

	FetchedInstruction.Op = (FetchedInstruction.Op<<16) | OpW2;
	return true;
}

bool CPU::FetchInstruction(const uLONG Offset /*= 0*/)
{
	const uLONG Add = R[PC].u + Offset;
	if(ICache_Add[0] == Add)	
		FetchedInstruction.Helper.Instruction = ICache_Inst0[0];
	else if(ICache_Add[1] == Add)	
		FetchedInstruction.Helper.Instruction = ICache_Inst0[1];
	else if(ICache_Add[2] == Add)	
		FetchedInstruction.Helper.Instruction = ICache_Inst0[2];
	else if(ICache_Add[3] == Add)	
		FetchedInstruction.Helper.Instruction = ICache_Inst0[3];
	else if(ICache_Add[4] == Add)	
		FetchedInstruction.Helper.Instruction = ICache_Inst0[4];
	else if(ICache_Add[5] == Add)	
		FetchedInstruction.Helper.Instruction = ICache_Inst0[5];
	else if(ICache_Add[6] == Add)	
		FetchedInstruction.Helper.Instruction = ICache_Inst0[6];
	else if(ICache_Add[7] == Add)	
		FetchedInstruction.Helper.Instruction = ICache_Inst0[7];
	else if(bMemWriteOperand )
		return false;
	else if(!MReadWord(FetchedInstruction.Helper.Instruction, Add))
		return false;
	else
	{
		CacheIdx = ++CacheIdx & 7;
		ICache_Add[CacheIdx] = Add;
		ICache_Inst0[CacheIdx] = FetchedInstruction.Helper.Instruction;
	}

	if(FetchedInstruction.Helper.Opcode.f0 < 31)
		FetchedInstruction.Opcode = EOpcode(FetchedInstruction.Helper.Opcode.f0);
	else if(FetchedInstruction.Helper.Opcode.f1 < 7)
		FetchedInstruction.Opcode = EOpcode(FetchedInstruction.Helper.Opcode.f1 + Group1);
	else
		FetchedInstruction.Opcode = EOpcode(FetchedInstruction.Helper.Opcode.f2 + Group2);

	switch(FetchedInstruction.Opcode)
	{
	case ADD: 
	case MUL: 
	case SUB: 
	case DIV: 
	case MOD: 
	case CMP: 
	case MOV: 
	case XOR: 
	case OR: 
	case AND: 
	case MOVB: 
		FetchedInstruction.AdMode = FetchedInstruction.Helper.Type0.AM;
		FetchedInstruction.Op = FetchedInstruction.Helper.Type0.OP;
		FetchedInstruction.Dir = FetchedInstruction.Helper.Type0.DIR;
		FetchedInstruction.Register = FetchedInstruction.Helper.Type0.REG;
		break;
	case MOVI:
		FetchedInstruction.AdMode = Devon::CPU::Imm8;
		FetchedInstruction.Op = FetchedInstruction.Helper.Type12.OP;
		FetchedInstruction.Dir = ToRegister;
		FetchedInstruction.Register = FetchedInstruction.Helper.Type12.REG;		
		break;
	case JMP: 
	case JSR: 
		FetchedInstruction.AdMode = FetchedInstruction.Helper.Type4.AM;
		FetchedInstruction.Op = FetchedInstruction.Helper.Type4.OP;
		FetchedInstruction.Dir = ToAdMode;
		break;
	case SHIFT: 
	case BOP: 
		FetchedInstruction.AdMode = FetchedInstruction.Helper.Type5.AM;		
		FetchedInstruction.Op = FetchedInstruction.Helper.Type5.REGB<<1;
		FetchedInstruction.Dir = ToRegister;
		FetchedInstruction.Register = FetchedInstruction.Helper.Type5.REGA;
		break;
	case SHIFTI:
	case BOPI: 
		FetchedInstruction.AdMode = Reg;		
		FetchedInstruction.Dir = ToRegister;
		FetchedInstruction.Register = FetchedInstruction.Helper.Type6.REGA;
		break;
	case VBASE: 
	case TRAP: 
	case NOT: 
	case NEG: 
		FetchedInstruction.AdMode = FetchedInstruction.Helper.Type8.AM;		
		FetchedInstruction.Op = FetchedInstruction.Helper.Type8.OP;
		break;
	case EXT: 
		FetchedInstruction.AdMode = Reg;		
		FetchedInstruction.Register = FetchedInstruction.Helper.Type11.REG;
		break;
	default:
		FetchedInstruction.AdMode = EAdModeMax;
	}

	switch(FetchedInstruction.AdMode)
	{
	case Reg:
	case XReg:
	case XRegInc:
	case XRegDec:
		FetchedInstruction.LongOP = FetchedInstruction.Op & 1;
		FetchedInstruction.Op >>= 1;
		FetchedInstruction.LongInst = false;
		break;
	case XEA20L:
		FetchedInstruction.LongOP = true;
		FetchedInstruction.LongInst = true;
		break;
	case XEA20W:
		FetchedInstruction.LongOP = false;
		FetchedInstruction.LongInst = true;
		break;
	case Imm20:
		FetchedInstruction.LongInst = true;
		break;
	default:
		FetchedInstruction.LongInst = false;
	}

	return true;
}

void CPU::TestReg(const int RegIndex)
{
	SR.Flags.N = (R[RegIndex].s < 0);
	SR.Flags.Z = (R[RegIndex].s == 0);
}

void CPU::Interrupt(const int InterruptionLvl)
{
	if(bHalt)
		return;

	if(InterruptionLvl > SR.Flags.IntLvl)
	{
		const EVector VectorRequest = (EVector)(ExternalInterrupt0 + InterruptionLvl);
		if(PendingInterrupt == EVector::NoVector || VectorRequest < PendingInterrupt)
		{
			if(((1<<InterruptionLvl) & IntMask) != 0)
				PendingInterrupt = VectorRequest;
		}
	}
}

void CPU::Exception(const EVector ExceptionVector)
{
	CurException = ExceptionVector;
	Tick = &Devon::CPU::Tick_Exception_ComputeVectorAdd;
	ExecCycleCount = -1;
	bInstructionPreFetched = false;
	bInstructionExtensionPreFetched = false;
}

bool CPU::MReadWord(uWORD & Word, const uLONG Address)
{
	switch(MMU.ReadWord(Word, Address))
	{
	case OK:	return true;
	case WAIT:	return false;
	default:	Exception(EVector::BusError);
	}

	return false;
}

bool CPU::MWriteWord(const uWORD Word, const uLONG Address)
{
	switch(MMU.WriteWord(Word, Address))
	{
	case OK:	return true;
	case WAIT:	return false;
	default:	Exception(EVector::BusError);
	}

	return false;
}
