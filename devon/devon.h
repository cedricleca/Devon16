#pragma once

#include <immintrin.h>

class DevonMMU;

namespace Devon
{
	typedef unsigned short uWORD;
	typedef signed short sWORD;
	typedef unsigned int uLONG;
	typedef signed int sLONG;

	enum EMemAck
	{
		OK,
		WAIT,
		ERR,
	};

	class CPU
	{
	public:
		const static int NbRegs = 8;
		const static int SP = 7;
		const static int Dummy = 8;
		const static int NbICacheEntries = 8;

		union DataRegister
		{
			uLONG u;
			sLONG s;
			struct { uWORD lsw; uWORD msw; } uw;
			struct { sWORD lsw; sWORD msw; } sw;
		};

		enum EVector
		{
			NoVector = 0,
			ResetPC = 0,
			ResetStack,
			BusError,
			BadInstruction,
			DivideByZero,

			ExternalInterrupt0 = 0x10,
			ExternalInterrupt1, ExternalInterrupt2,	ExternalInterrupt3,	ExternalInterrupt4,
			ExternalInterrupt5,	ExternalInterrupt6,	ExternalInterrupt7,

			Trap0 = 0x20,
			Trap1, Trap2, Trap3, Trap4, Trap5, Trap6, Trap7, Trap8,	Trap9, TrapA, TrapB,
			TrapC, TrapD, TrapE, TrapF,
		};

		enum EStackingStatus
		{
			NoStacking, StackingLo, StackingHi, StackingOK, UnstackingSR,
		};

		enum EAdMode
		{
			Reg, XReg, XRegInc, XRegDec,
			Imm4, Imm20, XEA20W, XEA20L,
			Imm8,
			EAdModeMax
		};

		enum ECndFlag
		{
			P=(1<<0),
			N=(1<<1),
			Z=(1<<2),
		};

		enum EShiftMode
		{
			ASR, LSL, LSR, ROL, ROR, ROXL, ROXR,
		};

		enum EBOPMode
		{
			BCLR, BSET, BNOT, BTST, BCPY, 
		};

		enum ESOPMode
		{
			SSAVE, SLOAD, SSWAP, 
		};

		enum EEXTMode
		{
			EXT4, EXT8, EXT16, EXT20,
		};

		enum EOpcode : unsigned char
		{
			Group0,
			ADD = Group0, MUL, SUB, DIV, MOD, CMP,
			MOV, MOVI, XOR, OR, AND, MOVB,
			SOP,
			FOP, FTOI, ITOF,
			JMP, JSR,
			SHIFT, SHIFTI, BOP, BOPI,

			Group1,
			SWP = Group1, EXT, TRAP, NOT, NEG,
			INTMASK, VBASE,
		
			Group2,
			HALT = Group2, RESET, RTS, RTE, NOP, CHKX, CHKV,

			EOpcodeMax
		};

		static const unsigned char ExecTime[];

		enum EDirection
		{
			ToRegister,
			ToAdMode,
		};

		typedef union 
		{
			uWORD SR;
			struct { uWORD IntLvl:3, Z:1, N:1, X:1, V:1;} Flags;
		} StateRegister;

		struct InstDecode
		{
			union 
			{
				uWORD Instruction;
				struct { uWORD f2:8, f1:3, f0:5; } Opcode;

				struct { uWORD OP:4, AM:3, REG:3, DIR:1, OPC:5; }					Type0;
				struct { uWORD RS:8, M:3, OPC:5; }									Type1; // SSAVE, SLOAD, SSWAP
				struct { uWORD REGB:3, REGA:3, OPT:2, M:3, OPC:5; }					Type2; // FOP
				struct { uWORD N:5, UNU:2, REG:3, M:1, OPC:5; }						Type3; // FTOI, ITOF
				struct { uWORD OP:4, AM:3, CND:3, M:1, OPC:5; }						Type4; // JMP/BRA, JSR/BSR
				struct { uWORD REGB:3, REGA:3, AM:2, M:3, OPC:5; }					Type5; // SHIFT, BOP
				struct { uWORD OP:5, REGA:3, M:3, OPC:5; }							Type6; // SHIFTI, BOPI
				struct { uWORD OP:8, REG:3, OPC:5; }								Type12; // MOVI
		
				struct { uWORD REG:3, UNU:2, WLS:1, WHS:1, LS:1, OPC:3, POPC:5; }	Type7; // SWP
				struct { uWORD OP:4, AM:3, UNU:1, OPC:3, POPC:5; }					Type8; // NOT, TRAP, NEG, VBASE
				struct { uWORD OP:8, OPC:3, POPC:5; }								Type9; // INTMASK

				struct { uWORD OPC:8, POPC:8; }										Type10;
				struct { uWORD REG:3, UNU:3, M:2, OPC:3, POPC:5; }					Type11; // EXT
			} Helper;

			unsigned char SrcRegister;
			unsigned char DstRegister;
			unsigned char Register;
			unsigned char AdMode;
			uLONG Op;
			EOpcode Opcode;
			unsigned char 
				LongOP:1,
				LongInst:1;
			unsigned char Dir;
			unsigned char CycleCount;
			unsigned char InstructionSize;
		};

		InstDecode		ExecInstruction;
		InstDecode		FetchedInstruction;
		DataRegister	R[NbRegs+1];	// general registers + 1 for volatile operands
		DataRegister	S[NbRegs];		// backup registers
		DataRegister	PC;
		__m256i			mCache_Add;
		__m256i			mCache_Inst;

		uLONG			CurOPAddress;
		DevonMMU*		MMU;
		EVector			PendingInterrupt;
		EVector			CurException;
		uLONG			CurExceptionVectorAddress;
		uLONG			VectorTableBase;
		uLONG			NextInstAdd;
		uWORD			IntMask;
		StateRegister	SR;
		EStackingStatus	StackingStatus;
		unsigned char	CacheIdx;

		unsigned char 
			bInstructionPreFetched:1,
			bInstructionExtensionPreFetched:1,
			bMemWriteOperand:1,
			bTrace:1,
			bHalt:1;

		inline bool MReadWord(uWORD & Word, const uLONG Address);
		inline bool MWriteWord(const uWORD Word, const uLONG Address);
		bool FetchInstruction(const uLONG Add);
		inline bool FetchInstructionExtension(const uLONG Add);
		void Exception(const EVector ExceptionVector);
		inline void TestReg(const int RegIndex);

	public:
		CPU(DevonMMU * InMMU);

		void Reset();
		void HardReset();
		void Halt(bool NewHalt);
		void Trace();
		void Interrupt(int InterruptionLvl);

		void Tick_Exception_ComputeVectorAdd();
		void Tick_Exception_GetVector0();
		void Tick_Exception_GetVector1();
		void Tick_Exception_StackRet0();
		void Tick_Exception_StackRet1();
		void Tick_Exception_StackSR();
		void Tick_Exception_Jmp();
		void Tick_GetInst0();
		void Tick_GetInst1();
		void Tick_PreDecrGet();
		void Tick_GetOp0();
		void Tick_GetOp1();
		void Tick_Decode();
		void Tick_Exec();
		void Tick_PreDecrPut();
		void Tick_PutRes0();
		void Tick_PutRes1();

		void (Devon::CPU::*Tick)() = nullptr;
	};
}