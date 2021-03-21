#pragma once

#include "..\devon\devonMMU.h"

struct DisassemblyWindow
{
    bool            Open=true;                                   // = true   // set to false when DrawWindow() was closed. ignore if not using DrawWindow
	int				NextJump;
	unsigned int	OldPC;

    DisassemblyWindow()
	{
		NextJump = -1;
		OldPC = 0;
	}

	void DrawWindow(const char* title, DevonMMU & MMU, Devon::CPU & CPU, uLONG base_display_addr = 0x0000)
    {
        ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(FLT_MAX, FLT_MAX));

        Open = true;
        if (ImGui::Begin(title, &Open, ImGuiWindowFlags_NoScrollbar))
        {
			for(int i = 0; i < 8; i++)
			{
				ImGui::Text("R%d  %08x  %d", i, CPU.R[i].u, CPU.R[i].s);
				ImGui::SameLine(230.0f);
				ImGui::Text("S%d  %08x  %d", i, CPU.S[i].u, CPU.S[i].s);
			}
			ImGui::Separator();
			ImGui::Text("PC %08x  %d", CPU.R[CPU::PC].u, CPU.R[CPU::PC].s);
			ImGui::SameLine(230.0f);
			ImGui::Text("VBase %08x  %d", CPU.VectorTableBase, CPU.VectorTableBase);
			ImGui::Text("SR %08x  N=%d  Z=%d  X=%d  V=%d  Halt=%d  IntLvl=%d  IntMask=%02x", CPU.SR.SR, CPU.SR.Flags.N?1:0, CPU.SR.Flags.Z?1:0, CPU.SR.Flags.X?1:0, CPU.SR.Flags.V?1:0, CPU.bHalt?1:0, CPU.SR.Flags.IntLvl, CPU.IntMask);

			ImGui::Separator();
			/*
			if(ImGui::Button("Hard Reset"))
				CPU.HardReset();

			ImGui::SameLine();
				*/
			if(ImGui::Button("Reset"))
				CPU.Reset();

			ImGui::SameLine();
			if(ImGui::Button("Reset & Halt"))
			{
				CPU.Reset();
				CPU.Halt(true);
			}

			ImGui::SameLine();
			if(!CPU.bHalt && ImGui::Button("Halt"))
			{
				CPU.Halt(true);
			}
			else if(CPU.bHalt)
			{
				if(ImGui::Button("Run (Ctrl-F11)"))
					CPU.Halt(false);

				if(ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(VK_F11, false))
					CPU.Halt(false);

				ImGui::SameLine();
				if(ImGui::Button("Step (F11)") || ImGui::IsKeyPressed(VK_F11, false))
					CPU.Trace();
			}

			int Jump = NextJump;

			ImGui::SameLine();
			if(ImGui::Button("Goto PC"))
			{
				Jump = CPU.R[Devon::CPU::PC].u - 3;
				NextJump = -1;
			}

			ImGui::Separator();
			ImGui::BeginChild("##scrolling", ImVec2(0, 0));
			ImDrawList* draw_list = ImGui::GetWindowDrawList();

			if(Jump >= 0)
				ImGui::SetScrollY(Jump * ImGui::GetTextLineHeight());

			ImGuiListClipper clipper(0xA0000, ImGui::GetTextLineHeight());
			while(clipper.Step())
			{
				unsigned int CurAdr = clipper.DisplayStart;
				for(int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
				{
					if(CurAdr == CPU.R[Devon::CPU::PC].u)
					{
						ImVec2 pos = ImGui::GetCursorScreenPos();
						draw_list->AddRectFilled(pos, ImVec2(pos.x + ImGui::GetWindowWidth(), pos.y + ImGui::GetFontSize() + 1), IM_COL32(200, 20, 0, 140));
					}

					std::string Line;
					int InstSize = DisassembleInstruction(Line, CurAdr, (DevonMMU &)MMU);
					ImGui::Text("%05X    %s", CurAdr, Line.c_str());

					ImGui::SameLine(320.0f);
					uWORD InstData;
					MMU.ReadWord<DevonMMU::NoFail>(InstData, CurAdr);
					ImGui::TextDisabled("%04X", InstData);

					if(InstSize > 1)
					{
						ImGui::SameLine();
						MMU.ReadWord<DevonMMU::NoFail>(InstData, CurAdr+1);
						ImGui::TextDisabled("%04X ", InstData);
					}

					CurAdr += InstSize;
				}

				NextJump = -1;
				if(OldPC != CPU.R[Devon::CPU::PC].u)
				{
					if(CPU.R[Devon::CPU::PC].u > CurAdr - 4 || CPU.R[Devon::CPU::PC].u < (unsigned)clipper.DisplayStart)
					{
						NextJump = CPU.R[Devon::CPU::PC].u - 3;
					}
				}
				OldPC = CPU.R[Devon::CPU::PC].u;
			}

		
			ImGui::EndChild();
		}

		ImGui::End();
    }

	int DisassembleInstruction(std::string & out, uLONG addr, DevonMMU & MMU)
	{
		Devon::CPU::InstDecode Inst;
		MMU.ReadWord<DevonMMU::NoFail>(Inst.Helper.Instruction, addr);

		if(Inst.Helper.Opcode.f0 < 31)
			Inst.Opcode = Devon::CPU::EOpcode(Inst.Helper.Opcode.f0 + Devon::CPU::EOpcode::Group0);
		else if(Inst.Helper.Opcode.f1 < 7)
			Inst.Opcode = Devon::CPU::EOpcode(Inst.Helper.Opcode.f1 + Devon::CPU::EOpcode::Group1);
		else
			Inst.Opcode = Devon::CPU::EOpcode(Inst.Helper.Opcode.f2 + Devon::CPU::EOpcode::Group2);

		Inst.AdMode = Devon::CPU::EAdMode::EAdModeMax;
		switch(Inst.Opcode)
		{
		case Devon::CPU::EOpcode::ADD: 
		case Devon::CPU::EOpcode::MUL: 
		case Devon::CPU::EOpcode::SUB: 
		case Devon::CPU::EOpcode::DIV: 
		case Devon::CPU::EOpcode::MOD: 
		case Devon::CPU::EOpcode::CMP: 
		case Devon::CPU::EOpcode::MOV: 
		case Devon::CPU::EOpcode::XOR: 
		case Devon::CPU::EOpcode::OR: 
		case Devon::CPU::EOpcode::AND: 
		case Devon::CPU::EOpcode::MOVB: 
			Inst.AdMode = Inst.Helper.Type0.AM;
			Inst.Op = Inst.Helper.Type0.OP;
			Inst.Dir = Inst.Helper.Type0.DIR;
			Inst.Register = Inst.Helper.Type0.REG;
			break;
		case Devon::CPU::EOpcode::SOP: 
			Inst.AdMode = Devon::CPU::EAdMode::Imm8;		
			Inst.Op = Inst.Helper.Type1.RS;
			break;
		case Devon::CPU::EOpcode::JMP: 
		case Devon::CPU::EOpcode::JSR: 
			Inst.AdMode = Inst.Helper.Type4.AM;
			Inst.Op = Inst.Helper.Type4.OP;
			Inst.Dir = Devon::CPU::EDirection::ToAdMode;
			break;
		case Devon::CPU::EOpcode::SHIFT: 
		case Devon::CPU::EOpcode::BOP: 
			Inst.AdMode = Inst.Helper.Type5.AM;		
			Inst.Op = Inst.Helper.Type5.REGA<<1;
			break;
		case Devon::CPU::EOpcode::SHIFTI: 
		case Devon::CPU::EOpcode::BOPI: 
			Inst.AdMode = Devon::CPU::EAdMode::Imm8;		
			Inst.Op = Inst.Helper.Type6.OP;
			break;
		case Devon::CPU::EOpcode::SWP: 
			Inst.AdMode = Devon::CPU::EAdMode::Reg;
			Inst.Op = Inst.Helper.Type7.REG<<1;
			break;
		case Devon::CPU::EOpcode::VBASE: 
		case Devon::CPU::EOpcode::TRAP: 
		case Devon::CPU::EOpcode::NOT: 
		case Devon::CPU::EOpcode::NEG: 
			Inst.AdMode = Inst.Helper.Type8.AM;		
			Inst.Op = Inst.Helper.Type8.OP;
			break;
		case Devon::CPU::EOpcode::INTMASK: 
			Inst.AdMode = Devon::CPU::EAdMode::Imm8;		
			Inst.Op = Inst.Helper.Type9.OP;
			break;
		case Devon::CPU::EOpcode::EXT: 
			Inst.AdMode = Devon::CPU::EAdMode::Reg;
			Inst.Op = Inst.Helper.Type11.REG<<1;		
			break;
		case Devon::CPU::EOpcode::MOVI:
			Inst.AdMode = Devon::CPU::EAdMode::Imm8;
			Inst.Register = Inst.Helper.Type12.REG;		
			Inst.Op = Inst.Helper.Type12.OP;
			break;
		}

		switch(Inst.Opcode)
		{
		case Devon::CPU::EOpcode::ADD:		out = "add";		break;
		case Devon::CPU::EOpcode::MUL:		out = "mul";		break;
		case Devon::CPU::EOpcode::SUB:		out = "sub";		break;
		case Devon::CPU::EOpcode::DIV:		out = "div";		break;
		case Devon::CPU::EOpcode::MOD:		out = "mod";		break;
		case Devon::CPU::EOpcode::CMP:		out = "cmp";		break;
		case Devon::CPU::EOpcode::MOV:		out = "mov";		break;
		case Devon::CPU::EOpcode::MOVI:		out = "movi";		break;
		case Devon::CPU::EOpcode::XOR:		out = "xor";		break;
		case Devon::CPU::EOpcode::OR:		out = "or";			break;
		case Devon::CPU::EOpcode::AND:		out = "and";		break;
		case Devon::CPU::EOpcode::MOVB:		out = "movb";		break;
		case Devon::CPU::EOpcode::SWP:		out = "swp";		break;
		case Devon::CPU::EOpcode::TRAP:		out = "trap";		break;
		case Devon::CPU::EOpcode::NOT:		out = "not";		break;
		case Devon::CPU::EOpcode::NEG:		out = "neg";		break;
		case Devon::CPU::EOpcode::INTMASK:	out = "intmask";	break;
		case Devon::CPU::EOpcode::VBASE:	out = "vbase";		break;
		case Devon::CPU::EOpcode::HALT:		out = "halt";		break;
		case Devon::CPU::EOpcode::RESET:	out = "reset";		break;
		case Devon::CPU::EOpcode::RTS:		out = "rts";		break;
		case Devon::CPU::EOpcode::RTE:		out = "rte";		break;
		case Devon::CPU::EOpcode::NOP:		out = "nop";		break;
		case Devon::CPU::EOpcode::CHKX:		out = "chkx";		break;
		case Devon::CPU::EOpcode::CHKV:		out = "chkv";		break;

		case Devon::CPU::EOpcode::JMP:		
			if(Inst.Helper.Type4.M)
				out = "bra";
			else
				out = "jmp";
			break;

		case Devon::CPU::EOpcode::JSR:		
			if(Inst.Helper.Type4.M)
				out = "bsr";		
			else
				out = "jsr";		
			break;

		case Devon::CPU::EOpcode::SHIFT:
			switch(Inst.Helper.Type5.M)
			{
			case CPU::EShiftMode::ASR:	out = "asr";		break;
			case CPU::EShiftMode::LSL:	out = "lsl";		break;
			case CPU::EShiftMode::LSR:	out = "lsr";		break;
			case CPU::EShiftMode::ROL:	out = "rol";		break;
			case CPU::EShiftMode::ROR:	out = "ror";		break;
			case CPU::EShiftMode::ROXL:	out = "rol";		break;
			case CPU::EShiftMode::ROXR:	out = "ror";		break;
			}
			break;
		case Devon::CPU::EOpcode::SHIFTI:	
			switch(Inst.Helper.Type6.M)
			{
			case CPU::EShiftMode::ASR:	out = "asri";		break;
			case CPU::EShiftMode::LSL:	out = "lsli";		break;
			case CPU::EShiftMode::LSR:	out = "lsri";		break;
			case CPU::EShiftMode::ROL:	out = "roli";		break;
			case CPU::EShiftMode::ROR:	out = "rori";		break;
			case CPU::EShiftMode::ROXL:	out = "roxli";		break;
			case CPU::EShiftMode::ROXR:	out = "roxri";		break;
			}
			break;
		case Devon::CPU::EOpcode::BOP:		
			switch(Inst.Helper.Type5.M)
			{
			case CPU::EBOPMode::BCLR:	out = "bclr";		break;
			case CPU::EBOPMode::BSET:	out = "bset";		break;
			case CPU::EBOPMode::BNOT:	out = "bnot";		break;
			case CPU::EBOPMode::BTST:	out = "btst";		break;
			case CPU::EBOPMode::BCPY:	out = "bcpy";		break;
			}
			break;
		case Devon::CPU::EOpcode::BOPI:		
			switch(Inst.Helper.Type6.M)
			{
			case CPU::EBOPMode::BCLR:	out = "bclri";		break;
			case CPU::EBOPMode::BSET:	out = "bseti";		break;
			case CPU::EBOPMode::BNOT:	out = "bnoti";		break;
			case CPU::EBOPMode::BTST:	out = "btsti";		break;
			case CPU::EBOPMode::BCPY:	out = "bcpyi";		break;
			}
			break;
		case Devon::CPU::EOpcode::SOP:
			switch(Inst.Helper.Type1.M)
			{
			case CPU::ESOPMode::SSAVE:	out = "ssave";		break;
			case CPU::ESOPMode::SLOAD:	out = "sload";		break;
			case CPU::ESOPMode::SSWAP:	out = "sswap";		break;
			}
			break;
		case Devon::CPU::EOpcode::EXT:
			switch(Inst.Helper.Type11.M)
			{
			case CPU::EEXTMode::EXT4:	out = "ext4";		break;
			case CPU::EEXTMode::EXT8:	out = "ext8";		break;
			case CPU::EEXTMode::EXT16:	out = "ext16";		break;
			case CPU::EEXTMode::EXT20:	out = "ext20";		break;
			}
			break;
		}

		switch(Inst.Opcode)
		{
		case Devon::CPU::EOpcode::MOV:
		case Devon::CPU::EOpcode::XOR:
		case Devon::CPU::EOpcode::OR:
		case Devon::CPU::EOpcode::AND:
			if(Inst.Helper.Type0.AM == CPU::EAdMode::Reg
				|| Inst.Helper.Type0.AM == CPU::EAdMode::XReg
				|| Inst.Helper.Type0.AM == CPU::EAdMode::XRegInc
				|| Inst.Helper.Type0.AM == CPU::EAdMode::XRegDec
				)
			{
				if((Inst.Helper.Type0.OP & 1) == 0)
					out += ".w";
			}
			break;
		case Devon::CPU::EOpcode::NOT:
			if(Inst.Helper.Type0.AM == CPU::EAdMode::Reg
				|| Inst.Helper.Type0.AM == CPU::EAdMode::XReg
				|| Inst.Helper.Type0.AM == CPU::EAdMode::XRegInc
				|| Inst.Helper.Type0.AM == CPU::EAdMode::XRegDec
				)
			{
				if((Inst.Helper.Type8.OP & 1) == 0)
					out += ".w";
			}
			break;

		case Devon::CPU::EOpcode::JMP:		
		case Devon::CPU::EOpcode::JSR:		
			if(Inst.Helper.Type4.CND != 0)
			{
				out += ".";
				if((Inst.Helper.Type4.CND & CPU::ECndFlag::P) != 0)
					out += "p";
				if((Inst.Helper.Type4.CND & CPU::ECndFlag::N) != 0)
					out += "n";
				if((Inst.Helper.Type4.CND & CPU::ECndFlag::Z) != 0)
					out += "z";
			}
			break;

		case Devon::CPU::EOpcode::SWP:		
			if(Inst.Helper.Type7.LS != 0 ||Inst.Helper.Type7.WHS != 0 ||Inst.Helper.Type7.WLS != 0)
			{
				out += ".";
				if(Inst.Helper.Type7.LS != 0)
					out += "w";
				if(Inst.Helper.Type7.WHS != 0)
					out += "h";
				if(Inst.Helper.Type7.WLS != 0)
					out += "l";
			}
			break;

		case Devon::CPU::EOpcode::BOP:
		case Devon::CPU::EOpcode::SHIFT:
			if(Inst.Helper.Type5.AM != CPU::EAdMode::Reg)
				out += ".w";
			break;
		}

		if(Inst.AdMode == CPU::EAdMode::XEA20W)
			out += ".w";

		while(out.size() < 8)
			out += " ";

		switch(Inst.Opcode)
		{
		case Devon::CPU::EOpcode::ADD: 
		case Devon::CPU::EOpcode::MUL: 
		case Devon::CPU::EOpcode::SUB: 
		case Devon::CPU::EOpcode::DIV: 
		case Devon::CPU::EOpcode::MOD: 
		case Devon::CPU::EOpcode::CMP: 
		case Devon::CPU::EOpcode::MOV: 
		case Devon::CPU::EOpcode::XOR: 
		case Devon::CPU::EOpcode::OR: 
		case Devon::CPU::EOpcode::AND: 
		case Devon::CPU::EOpcode::MOVB: 
			if(Inst.Dir == Devon::CPU::EDirection::ToAdMode)
				out += "r" + std::to_string(Inst.Register) + ",";
			break;

		case Devon::CPU::EOpcode::SHIFT: 
		case Devon::CPU::EOpcode::BOP: 
			out += "r" + std::to_string(Inst.Helper.Type5.REGB) + ",";
			break;
		}

		uLONG LongOP = 0;
		if(Inst.AdMode == Devon::CPU::EAdMode::Imm20
			|| Inst.AdMode == Devon::CPU::EAdMode::XEA20W
			|| Inst.AdMode == Devon::CPU::EAdMode::XEA20L
			)
		{
			uWORD Extra = 0;
			MMU.ReadWord<DevonMMU::NoFail>(Extra, addr+1);
			LongOP = (Inst.Op<<16) | Extra;
		}
		else if(Inst.AdMode == Devon::CPU::EAdMode::Imm8)
		{
			LongOP = Inst.Op;
		}

		std::stringstream stream;
		stream << std::hex << LongOP;
		std::string LongOPHexStr( stream.str() );

		switch(Inst.AdMode)
		{
		case Devon::CPU::EAdMode::Reg:
			out += "r" + std::to_string(Inst.Op>>1);
			break;
		case Devon::CPU::EAdMode::XReg:
			out += "(r" + std::to_string(Inst.Op>>1) + ")";
			break;
		case Devon::CPU::EAdMode::XRegInc:
			out += "(r" + std::to_string(Inst.Op>>1) + ")+";
			break;
		case Devon::CPU::EAdMode::XRegDec:
			out += "-(r" + std::to_string(Inst.Op>>1) + ")";
			break;
		case Devon::CPU::EAdMode::Imm4:
			if((Inst.Opcode == Devon::CPU::EOpcode::JMP || Inst.Opcode == Devon::CPU::EOpcode::JSR)
				&& Inst.Helper.Type4.M
				)
			{
				std::stringstream stream;
				if(Inst.Op & 0x8)
					stream << std::hex << (addr + sLONG(Inst.Op | 0xFFFFFFF0));
				else
					stream << std::hex << (addr + Inst.Op);
				std::string JmpAddHexStr( stream.str() );
				out += "-> 0x" + JmpAddHexStr;
			}
			else if(Inst.Opcode == Devon::CPU::EOpcode::ADD
				|| Inst.Opcode == Devon::CPU::EOpcode::MUL
				|| Inst.Opcode == Devon::CPU::EOpcode::SUB
				|| Inst.Opcode == Devon::CPU::EOpcode::DIV
				|| Inst.Opcode == Devon::CPU::EOpcode::MOD
				|| Inst.Opcode == Devon::CPU::EOpcode::CMP
				|| Inst.Opcode == Devon::CPU::EOpcode::JMP
				|| Inst.Opcode == Devon::CPU::EOpcode::JSR
				)
			{
				if(Inst.Op < 8)
					out += "#!" + std::to_string(Inst.Op);
				else
					out += "#!-" + std::to_string(16-Inst.Op);
			}
			else
			{
				std::stringstream stream;
				stream << std::hex << Inst.Op;
				std::string HexStr( stream.str() );
				out += "#!0x" + HexStr;
			}
			break;
		case Devon::CPU::EAdMode::Imm20:
			if((Inst.Opcode == Devon::CPU::EOpcode::JMP || Inst.Opcode == Devon::CPU::EOpcode::JSR)
				&& Inst.Helper.Type4.M // relative mode
				)
			{
				std::stringstream stream;
				if(LongOP & 0x80000)
					stream << std::hex << (addr + sLONG(LongOP | 0xFFF00000));
				else
					stream << std::hex << (addr + LongOP);
				std::string JmpAddHexStr( stream.str() );
				out += "-> 0x" + JmpAddHexStr;
			}
			else
				out += "#0x" + LongOPHexStr;
			break;
		case Devon::CPU::EAdMode::XEA20L:
		case Devon::CPU::EAdMode::XEA20W:
			out += "$0x" + LongOPHexStr;
			break;
		case Devon::CPU::EAdMode::Imm8:
			if(Inst.Opcode == Devon::CPU::EOpcode::INTMASK
				||Inst.Opcode == Devon::CPU::EOpcode::SOP
				)
			{
				out += "#%";
				for(int i = 7; i >= 0; i--)
					out += ((Inst.Op>>i)&1) != 0 ? "1" : "0";
			}
			else if(Inst.Opcode == Devon::CPU::EOpcode::MOVI)
			{
				if(Inst.Op < 128)
					out += "#" + std::to_string(Inst.Op);
				else
					out += "#-" + std::to_string(256-Inst.Op);
			}
			else
				out += "#0x" + LongOPHexStr;
			break;
		}

		switch(Inst.Opcode)
		{
		case Devon::CPU::EOpcode::ADD: 
		case Devon::CPU::EOpcode::MUL: 
		case Devon::CPU::EOpcode::SUB: 
		case Devon::CPU::EOpcode::DIV: 
		case Devon::CPU::EOpcode::MOD: 
		case Devon::CPU::EOpcode::CMP: 
		case Devon::CPU::EOpcode::MOV: 
		case Devon::CPU::EOpcode::XOR: 
		case Devon::CPU::EOpcode::OR: 
		case Devon::CPU::EOpcode::AND: 
		case Devon::CPU::EOpcode::MOVB: 
			if(Inst.Dir == Devon::CPU::EDirection::ToRegister)
				out += ",r" + std::to_string(Inst.Register);
			break;

		case Devon::CPU::EOpcode::MOVI: 
			out += ",r" + std::to_string(Inst.Register);
			break;

		case Devon::CPU::EOpcode::SHIFTI: 
		case Devon::CPU::EOpcode::BOPI: 
			out += ",r" + std::to_string(Inst.Helper.Type6.REGA);
			break;
		}

		bool bLongInst = (Inst.AdMode == Devon::CPU::EAdMode::Imm20 
						|| Inst.AdMode == Devon::CPU::EAdMode::XEA20W 
						|| Inst.AdMode ==  Devon::CPU::EAdMode::XEA20L
						);

		return bLongInst ? 2 : 1;
	}
};
