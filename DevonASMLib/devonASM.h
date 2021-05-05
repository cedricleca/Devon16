#pragma once

#include <numeric>
#include <string>
#include <iostream>
#include <stdio.h>
#include <pegtl.hpp>
#include "DASM.h"

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;
using namespace Devon;

namespace DevonASM
{
	enum EMnemonic
	{
		ADD, MUL, SUB, DIV, MOD, CMP,
		MOV, MOVI, XOR, OR, AND, MOVB,
		SSAVE, SLOAD, SSWAP,
		FADD, FMUL, FMOD, FCMP, FPOW, FMOV, FTOI, ITOF,
		JMP, JSR, BRA, BSR,
		ASL, ASR, LSL, LSR, ROL, ROR, ROXL, ROXR, ASLI, ASRI, LSLI, LSRI, ROLI, RORI, ROXLI, ROXRI,
		BCLR, BSET, BNOT, BTST, BCPY, BCLRI, BSETI, BNOTI, BTSTI, BCPYI,
		SWP, EXT4, EXT8, EXT16, EXT20, TRAP, NOT, NEG,
		INTMASK, VBASE,
		HALT, RESET, RTS, RTE, NOP, CHKX, CHKV,

		Max
	};

	CPU::EOpcode Mnemonic2Opcode[EMnemonic::Max] =
	{
		CPU::EOpcode::ADD, CPU::EOpcode::MUL, CPU::EOpcode::SUB, CPU::EOpcode::DIV, CPU::EOpcode::MOD, CPU::EOpcode::CMP,
		CPU::EOpcode::MOV, CPU::EOpcode::MOVI, CPU::EOpcode::XOR, CPU::EOpcode::OR, CPU::EOpcode::AND, CPU::EOpcode::MOVB,
		CPU::EOpcode::SOP, CPU::EOpcode::SOP, CPU::EOpcode::SOP,
		CPU::EOpcode::FOP, CPU::EOpcode::FOP, CPU::EOpcode::FOP, CPU::EOpcode::FOP, CPU::EOpcode::FOP, CPU::EOpcode::FOP, CPU::EOpcode::FTOI, CPU::EOpcode::ITOF,
		CPU::EOpcode::JMP, CPU::EOpcode::JSR, CPU::EOpcode::JMP, CPU::EOpcode::JSR,
		CPU::EOpcode::SHIFT, CPU::EOpcode::SHIFT, CPU::EOpcode::SHIFT, CPU::EOpcode::SHIFT, CPU::EOpcode::SHIFT, CPU::EOpcode::SHIFT, CPU::EOpcode::SHIFT, CPU::EOpcode::SHIFT,
		CPU::EOpcode::SHIFTI, CPU::EOpcode::SHIFTI, CPU::EOpcode::SHIFTI, CPU::EOpcode::SHIFTI, CPU::EOpcode::SHIFTI, CPU::EOpcode::SHIFTI, CPU::EOpcode::SHIFTI, CPU::EOpcode::SHIFTI,
		CPU::EOpcode::BOP, CPU::EOpcode::BOP, CPU::EOpcode::BOP, CPU::EOpcode::BOP, CPU::EOpcode::BOP,
		CPU::EOpcode::BOPI, CPU::EOpcode::BOPI, CPU::EOpcode::BOPI, CPU::EOpcode::BOPI, CPU::EOpcode::BOPI,
		CPU::EOpcode::SWP, CPU::EOpcode::EXT, CPU::EOpcode::EXT, CPU::EOpcode::EXT, CPU::EOpcode::EXT, CPU::EOpcode::TRAP, CPU::EOpcode::NOT, CPU::EOpcode::NEG,
		CPU::EOpcode::INTMASK, CPU::EOpcode::VBASE,
		CPU::EOpcode::HALT, CPU::EOpcode::RESET, CPU::EOpcode::RTS, CPU::EOpcode::RTE, CPU::EOpcode::NOP, CPU::EOpcode::CHKX, CPU::EOpcode::CHKV,
	};

	struct blk : star<blank> {};
	struct blk_ : plus<blank> {};

	template <EMnemonic M> struct opc_string {};
	template<> struct opc_string<EMnemonic::ADD>		: TAO_PEGTL_ISTRING("add")		{};
	template<> struct opc_string<EMnemonic::MUL>		: TAO_PEGTL_ISTRING("mul")		{};
	template<> struct opc_string<EMnemonic::SUB>		: TAO_PEGTL_ISTRING("sub")		{};
	template<> struct opc_string<EMnemonic::DIV>		: TAO_PEGTL_ISTRING("div")		{};
	template<> struct opc_string<EMnemonic::MOD>		: TAO_PEGTL_ISTRING("mod")		{};
	template<> struct opc_string<EMnemonic::CMP>		: TAO_PEGTL_ISTRING("cmp")		{};
	template<> struct opc_string<EMnemonic::MOV>		: TAO_PEGTL_ISTRING("mov")		{};
	template<> struct opc_string<EMnemonic::MOVI>		: TAO_PEGTL_ISTRING("movi")		{};
	template<> struct opc_string<EMnemonic::XOR>		: TAO_PEGTL_ISTRING("xor")		{};
	template<> struct opc_string<EMnemonic::OR>			: TAO_PEGTL_ISTRING("or")		{};
	template<> struct opc_string<EMnemonic::AND>		: TAO_PEGTL_ISTRING("and")		{};
	template<> struct opc_string<EMnemonic::MOVB>		: TAO_PEGTL_ISTRING("movb")		{};
	template<> struct opc_string<EMnemonic::SSAVE>		: TAO_PEGTL_ISTRING("ssave")	{};
	template<> struct opc_string<EMnemonic::SLOAD>		: TAO_PEGTL_ISTRING("sload")	{};
	template<> struct opc_string<EMnemonic::SSWAP>		: TAO_PEGTL_ISTRING("sswap")	{};
	template<> struct opc_string<EMnemonic::JMP>		: TAO_PEGTL_ISTRING("jmp")		{};
	template<> struct opc_string<EMnemonic::JSR>		: TAO_PEGTL_ISTRING("jsr")		{};
	template<> struct opc_string<EMnemonic::BRA>		: TAO_PEGTL_ISTRING("bra")		{};
	template<> struct opc_string<EMnemonic::BSR>		: TAO_PEGTL_ISTRING("bsr")		{};
	template<> struct opc_string<EMnemonic::ROL>		: TAO_PEGTL_ISTRING("rol")		{};
	template<> struct opc_string<EMnemonic::ROLI>		: TAO_PEGTL_ISTRING("roli")		{};
	template<> struct opc_string<EMnemonic::ROR>		: TAO_PEGTL_ISTRING("ror")		{};
	template<> struct opc_string<EMnemonic::RORI>		: TAO_PEGTL_ISTRING("rori")		{};
	template<> struct opc_string<EMnemonic::ROXL>		: TAO_PEGTL_ISTRING("roxl")		{};
	template<> struct opc_string<EMnemonic::ROXLI>		: TAO_PEGTL_ISTRING("roxli")	{};
	template<> struct opc_string<EMnemonic::ROXR>		: TAO_PEGTL_ISTRING("roxr")		{};
	template<> struct opc_string<EMnemonic::ROXRI>		: TAO_PEGTL_ISTRING("roxri")	{};
	template<> struct opc_string<EMnemonic::ASL>		: TAO_PEGTL_ISTRING("asl")		{};
	template<> struct opc_string<EMnemonic::ASLI>		: TAO_PEGTL_ISTRING("asli")		{};
	template<> struct opc_string<EMnemonic::ASR>		: TAO_PEGTL_ISTRING("asr")		{};
	template<> struct opc_string<EMnemonic::ASRI>		: TAO_PEGTL_ISTRING("asri")		{};
	template<> struct opc_string<EMnemonic::LSL>		: TAO_PEGTL_ISTRING("lsl")		{};
	template<> struct opc_string<EMnemonic::LSLI>		: TAO_PEGTL_ISTRING("lsli")		{};
	template<> struct opc_string<EMnemonic::LSR>		: TAO_PEGTL_ISTRING("lsr")		{};
	template<> struct opc_string<EMnemonic::LSRI>		: TAO_PEGTL_ISTRING("lsri")		{};
	template<> struct opc_string<EMnemonic::BCLR>		: TAO_PEGTL_ISTRING("bclr")		{};
	template<> struct opc_string<EMnemonic::BCLRI>		: TAO_PEGTL_ISTRING("bclri")	{};
	template<> struct opc_string<EMnemonic::BCPY>		: TAO_PEGTL_ISTRING("bcpy")		{};
	template<> struct opc_string<EMnemonic::BCPYI>		: TAO_PEGTL_ISTRING("bcpyi")	{};
	template<> struct opc_string<EMnemonic::BNOT>		: TAO_PEGTL_ISTRING("bnot")		{};
	template<> struct opc_string<EMnemonic::BNOTI>		: TAO_PEGTL_ISTRING("bnoti")	{};
	template<> struct opc_string<EMnemonic::BSET>		: TAO_PEGTL_ISTRING("bset")		{};
	template<> struct opc_string<EMnemonic::BSETI>		: TAO_PEGTL_ISTRING("bseti")	{};
	template<> struct opc_string<EMnemonic::BTST>		: TAO_PEGTL_ISTRING("btst")		{};
	template<> struct opc_string<EMnemonic::BTSTI>		: TAO_PEGTL_ISTRING("btsti")	{};
	template<> struct opc_string<EMnemonic::SWP>		: TAO_PEGTL_ISTRING("swp")		{};
	template<> struct opc_string<EMnemonic::EXT4>		: TAO_PEGTL_ISTRING("ext4")		{};
	template<> struct opc_string<EMnemonic::EXT8>		: TAO_PEGTL_ISTRING("ext8")		{};
	template<> struct opc_string<EMnemonic::EXT16>		: TAO_PEGTL_ISTRING("ext16")	{};
	template<> struct opc_string<EMnemonic::EXT20>		: TAO_PEGTL_ISTRING("ext20")	{};
	template<> struct opc_string<EMnemonic::TRAP>		: TAO_PEGTL_ISTRING("trap")		{};
	template<> struct opc_string<EMnemonic::INTMASK>	: TAO_PEGTL_ISTRING("intmask")	{};
	template<> struct opc_string<EMnemonic::VBASE>		: TAO_PEGTL_ISTRING("vbase")	{};
	template<> struct opc_string<EMnemonic::HALT>		: TAO_PEGTL_ISTRING("halt")		{};
	template<> struct opc_string<EMnemonic::RESET>		: TAO_PEGTL_ISTRING("reset")	{};
	template<> struct opc_string<EMnemonic::RTS>		: TAO_PEGTL_ISTRING("rts")		{};
	template<> struct opc_string<EMnemonic::RTE>		: TAO_PEGTL_ISTRING("rte")		{};
	template<> struct opc_string<EMnemonic::NOP>		: TAO_PEGTL_ISTRING("nop")		{};
	template<> struct opc_string<EMnemonic::CHKX>		: TAO_PEGTL_ISTRING("chkx")		{};
	template<> struct opc_string<EMnemonic::CHKV>		: TAO_PEGTL_ISTRING("chkv")		{};
	template<> struct opc_string<EMnemonic::NOT>		: TAO_PEGTL_ISTRING("not")		{};
	template<> struct opc_string<EMnemonic::NEG>		: TAO_PEGTL_ISTRING("neg")		{};

	/*
	template<> struct opc_string<EMnemonic::FADD>		: TAO_PEGTL_ISTRING("fadd") {};
	template<> struct opc_string<EMnemonic::FCMP>		: TAO_PEGTL_ISTRING("fcmp") {};
	template<> struct opc_string<EMnemonic::FMOD>		: TAO_PEGTL_ISTRING("fmod") {};
	template<> struct opc_string<EMnemonic::FMOV>		: TAO_PEGTL_ISTRING("fmov") {};
	template<> struct opc_string<EMnemonic::FMUL>		: TAO_PEGTL_ISTRING("fmul") {};
	template<> struct opc_string<EMnemonic::FPOW>		: TAO_PEGTL_ISTRING("fpow") {};
	template<> struct opc_string<EMnemonic::FTOI>		: TAO_PEGTL_ISTRING("ftoi") {};
	template<> struct opc_string<EMnemonic::ITOF>		: TAO_PEGTL_ISTRING("itof") {};
	*/

	struct symbolreference;
	struct regindex : digit {};
	struct regSP : TAO_PEGTL_ISTRING("sp") {};
	struct decimalvalue : plus<digit> {};
	struct decimalinteger : seq<opt<sor<one<'+'>, one<'-'>>>, decimalvalue> {};
	struct hexavalue : plus<xdigit> {};
	struct hexainteger : seq<one<'0'>, one<'x'>, hexavalue> {};
	struct binaryvalue : plus<range<'0', '1'>> {};
	struct binaryinteger : seq<one<'%'>, binaryvalue> {};
	struct charvalue : any {};
	struct escapecharvalue : any {};
	struct charinteger : seq<one<'\''>, if_then_else<one<'\\'>, escapecharvalue, charvalue>, one<'\''>> {};
	struct integerToAdd : sor<hexainteger, decimalinteger, binaryinteger, charinteger, symbolreference> {};
	struct integer : seq<integerToAdd, star<blk, one<'+'>, blk, integerToAdd>> {};

	template <CPU::EAdMode AM> struct AMCode {};
	template<> struct AMCode<CPU::EAdMode::Reg> : sor<regSP, seq<sor<one<'r'>, one<'R'>>, regindex>> {};
	template<> struct AMCode<CPU::EAdMode::XReg> : seq<one<'('>, AMCode<CPU::EAdMode::Reg>, one<')'>> {};
	template<> struct AMCode<CPU::EAdMode::XRegDec> : seq<one<'-'>, one<'('>, AMCode<CPU::EAdMode::Reg>, one<')'>> {};
	template<> struct AMCode<CPU::EAdMode::XRegInc> : seq<one<'('>, AMCode<CPU::EAdMode::Reg>, one<')'>, one<'+'>> {};
	template<> struct AMCode<CPU::EAdMode::Imm4> : seq<one<'#'>, one<'!'>, integer> {};
	template<> struct AMCode<CPU::EAdMode::Imm20> : seq<one<'#'>, integer> {};
	template<> struct AMCode<CPU::EAdMode::XEA20L> : seq<one<'$'>, integer> {};

	struct AdModeErr : alpha {};

	struct admodeop0 : sor<AMCode<CPU::EAdMode::Reg>,
		AMCode<CPU::EAdMode::XRegDec>,
		AMCode<CPU::EAdMode::XRegInc>,
		AMCode<CPU::EAdMode::XReg>,
		AMCode<CPU::EAdMode::XEA20L>,
		AMCode<CPU::EAdMode::Imm4>,
		AMCode<CPU::EAdMode::Imm20>,
		AdModeErr
	> {};
	struct admodeop1 : sor<AMCode<CPU::EAdMode::Reg>, 
		AMCode<CPU::EAdMode::XRegDec>,
		AMCode<CPU::EAdMode::XRegInc>,
		AMCode<CPU::EAdMode::XReg>,
		AMCode<CPU::EAdMode::XEA20L>,
		AdModeErr
	> {};
	struct registerop : AMCode<CPU::EAdMode::Reg> {};

	struct comment : seq<one<';'>, until<eol, seven>> {};

	struct wop : seq<one<'.'>, sor<one<'w'>, one<'W'>>> {};

	struct cndP : sor<one<'p'>, one<'P'>> {};
	struct cndN : sor<one<'n'>, one<'N'>> {};
	struct cndZ : sor<one<'z'>, one<'Z'>> {};
	struct JmpCndErr : alpha {};
	struct JmpCndFlag : sor<cndP, cndN, cndZ, JmpCndErr> {};
	struct cnd : seq<one<'.'>, JmpCndFlag, opt<JmpCndFlag>, opt<JmpCndFlag>> {};

	struct cndW : sor<one<'w'>, one<'W'>> {};
	struct cndH : sor<one<'h'>, one<'H'>> {};
	struct cndL : sor<one<'l'>, one<'L'>> {};
	struct swpFlagErr : alpha {};
	struct swpFlag : sor<cndW, cndH, cndL, swpFlagErr> {};
	struct swpFlags : seq<one<'.'>, swpFlag, opt<swpFlag>, opt<swpFlag>> {};

	template<EMnemonic M> struct inst0 : seq<opc_string<M>, blk> {};
	template<EMnemonic M> struct inst1 : seq<opc_string<M>, opt<wop>, opt<cnd>, blk_, admodeop0, blk> {};
	template<EMnemonic M> struct inst2toregister : seq<opc_string<M>, opt<wop>, blk_, admodeop0, blk, one<','>, blk, registerop, blk> {};
	template<EMnemonic M> struct inst2fromregister : seq<opc_string<M>, opt<wop>, blk_, registerop, blk, one<','>, blk, admodeop1, blk> {};
	template<EMnemonic M> struct inst2 : sor<inst2fromregister<M>, inst2toregister<M>> {};
	struct instSWP : seq<opc_string<EMnemonic::SWP>, opt<swpFlags>, blk_, admodeop0, blk> {};
	struct instErr : alpha {};

	struct inst : sor<inst2<EMnemonic::ADD>,
		inst2<EMnemonic::MUL>,
		inst2<EMnemonic::SUB>,
		inst2<EMnemonic::DIV>,
		inst2<EMnemonic::MOD>,
		inst2<EMnemonic::CMP>,
		inst2<EMnemonic::MOV>,
		inst2<EMnemonic::MOVI>,
		inst2<EMnemonic::XOR>,
		inst2<EMnemonic::OR>,
		inst2<EMnemonic::AND>,
		inst2<EMnemonic::MOVB>,
		inst1<EMnemonic::SSAVE>,
		inst1<EMnemonic::SLOAD>,
		inst1<EMnemonic::SSWAP>,
		inst1<EMnemonic::JMP>,
		inst1<EMnemonic::JSR>,
		inst1<EMnemonic::BRA>,
		inst1<EMnemonic::BSR>,
		inst2<EMnemonic::ROL>,
		inst2<EMnemonic::ROLI>,
		inst2<EMnemonic::ROR>,
		inst2<EMnemonic::RORI>,
		inst2<EMnemonic::ROXL>,
		inst2<EMnemonic::ROXLI>,
		inst2<EMnemonic::ROXR>,
		inst2<EMnemonic::ROXRI>,
		inst2<EMnemonic::ASL>,
		inst2<EMnemonic::ASLI>,
		inst2<EMnemonic::ASR>,
		inst2<EMnemonic::ASRI>,
		inst2<EMnemonic::LSL>,
		inst2<EMnemonic::LSLI>,
		inst2<EMnemonic::LSR>,
		inst2<EMnemonic::LSRI>,
		inst2<EMnemonic::BCLR>,
		inst2<EMnemonic::BCLRI>,
		inst2<EMnemonic::BCPY>,
		inst2<EMnemonic::BCPYI>,
		inst2<EMnemonic::BNOT>,
		inst2<EMnemonic::BNOTI>,
		inst2<EMnemonic::BSET>,
		inst2<EMnemonic::BSETI>,
		inst2<EMnemonic::BTST>,
		inst2<EMnemonic::BTSTI>,
		instSWP,
		inst1<EMnemonic::EXT4>,
		inst1<EMnemonic::EXT8>,
		inst1<EMnemonic::EXT16>,
		inst1<EMnemonic::EXT20>,
		inst1<EMnemonic::TRAP>,
		inst1<EMnemonic::INTMASK>,
		inst1<EMnemonic::VBASE>,
		inst0<EMnemonic::HALT>,
		inst0<EMnemonic::RESET>,
		inst0<EMnemonic::RTS>,
		inst0<EMnemonic::RTE>,
		inst0<EMnemonic::NOP>,
		inst0<EMnemonic::CHKX>,
		inst0<EMnemonic::CHKV>,
		inst1<EMnemonic::NOT>,
		inst1<EMnemonic::NEG>,

		/*
		inst2<EMnemonic::FADD>,
		inst2<EMnemonic::FCMP>,
		inst2<EMnemonic::FMOD>,
		inst2<EMnemonic::FMOV>,
		inst2<EMnemonic::FMUL>,
		inst2<EMnemonic::FPOW>,
		inst2<EMnemonic::FTOI>,
		inst2<EMnemonic::ITOF>,
		*/

		instErr
	> {};

	struct directive_origin : seq<TAO_PEGTL_ISTRING("org"), blk_, opt<one<'$'>>, integer> {};
	struct directive_entry : seq<TAO_PEGTL_ISTRING("entry"), blk_, opt<one<'$'>>, integer> {};
	struct quotedstringCharValue : if_then_else<at<one<'"'>>, failure, if_then_else<one<'\\'>, escapecharvalue, charvalue>> {};
	struct quotedstring : seq<one<'"'>, star<quotedstringCharValue>, one<'"'>> {};
	struct byteintValue : integer {};
	struct filename : star<if_then_else<at<one<'"'>>, failure, seven>> {};
	struct directive_bytedata : seq<blk, opt<one<','>>, blk, sor<byteintValue, quotedstring>> {};
	struct directive_worddata : seq<blk, opt<one<','>>, blk, integer> {};
	struct directive_longdata : seq<blk, opt<one<','>>, blk, integer> {};
	struct directive_byte : seq<TAO_PEGTL_ISTRING("byte"), blk_, star<directive_bytedata>> {};
	struct directive_word : seq<TAO_PEGTL_ISTRING("word"), blk_, star<directive_worddata>> {};
	struct directive_long : seq<TAO_PEGTL_ISTRING("long"), blk_, star<directive_longdata>> {};
	struct symbolreference : identifier {};
	struct symbolvalue : identifier {};
	struct directive_define : seq<TAO_PEGTL_ISTRING("define"), blk_, symbolvalue, sor<seq<blk, one<'='>, blk>, blk_>, integer> {};
	struct directive_include : seq<TAO_PEGTL_ISTRING("include"), blk_, one<'"'>, filename, one<'"'>> {};
	struct directive_incbin : seq<TAO_PEGTL_ISTRING("incbin"), blk_, one<'"'>, filename, one<'"'>> {};
	struct directive_romexport : TAO_PEGTL_ISTRING("romexport") {};
	struct directive_align : seq<TAO_PEGTL_ISTRING("align"), blk_, integer> {};
	struct directive : sor<directive_origin,
		directive_define,
		directive_include,
		directive_incbin,
		directive_entry,
		directive_byte,
		directive_word,
		directive_long,
		directive_romexport,
		directive_align
	> {};

	struct linebody : seq<blk, opt<sor<directive, inst>>, if_then_else<at<one<';'>>, comment, until<eol, seven>>> {};
	struct label : seq<at<seq<symbolvalue, one<':'>>>, seq<symbolvalue, one<':'>>> {};
	struct line : seq<opt<label>, linebody> {};
	struct grammar : star<line> {};

	template< typename Rule > struct action : nothing< Rule > {};
	
	template<> struct action< AdModeErr >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			if(ASM.CurPass == 0)
				ASM.ErrorMessage(ErrorBadAdMode);
		}
	};

	template<EMnemonic M> struct action< inst2toregister<M> >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.LastDirection = CPU::EDirection::ToRegister;
		}
	};
	template<EMnemonic M> struct action< inst2fromregister<M> >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.LastDirection = CPU::EDirection::ToAdMode;
		}
	};
	template<> struct action< cndP >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.CndP = true;
		}
	};
	template<> struct action< cndN >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.CndN = true;
		}
	};
	template<> struct action< cndZ >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.CndZ = true;
		}
	};
	template<> struct action< JmpCndErr >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			if(ASM.CurPass == 0)
				ASM.ErrorMessage(ErrorBadJmpCnd);
		}
	};
	
	template<> struct action< instErr >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			if(ASM.CurPass == 0)
				ASM.ErrorMessage(ErrorMalformedInstruction);
		}
	};

	template<> struct action< cndW >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.CndW = true;
		}
	};
	template<> struct action< cndH >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.CndH = true;
		}
	};
	template<> struct action< cndL >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.CndL = true;
		}
	};
	template<> struct action< swpFlagErr >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			if(ASM.CurPass == 0)
				ASM.ErrorMessage(ErrorBadSWPFlag);
		}
	};

	template<> struct action< wop >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.LastOPLong = false;
			ASM.ExplicitShortOP = true;
		}
	};
	template<EMnemonic M> struct action< inst2<M> >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.AssembleInstruction(M);
			ASM.AdvanceAddress();
			ASM.NewInstruction();
		}
	};
	template<EMnemonic M> struct action< inst1<M> >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.AssembleInstruction(M);
			ASM.AdvanceAddress();
			ASM.NewInstruction();
		}
	};
	template<EMnemonic M> struct action< inst0<M> >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.AssembleInstruction(M);
			ASM.AdvanceAddress();
			ASM.NewInstruction();
		}
	};
	template<> struct action< instSWP >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.AssembleInstruction(EMnemonic::SWP);
			ASM.AdvanceAddress();
			ASM.NewInstruction();
		}
	};

	template<> struct action< directive >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.LastCharAvailable = false;
		}
	};

	template<> struct action< directive_origin >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.NextCodeChunk();
		}
	};

	template<> struct action< directive_entry >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.Entry = ASM.LastInt;
		}
	};

	template<> struct action< directive_define >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.Symbols[ASM.LastSymbol] = ASM.LastInt;
		}
	};
		

	template<> struct action< filename >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.LastFileName = in.string();
		}
	};
		
	template<> struct action< directive_align >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			if(ASM.LastCharAvailable)
				ASM.AddByte(0);

			if(ASM.LastInt != 0 )
			{
				while(ASM.CurAddress % ASM.LastInt != 0)
					ASM.AddWord(0);
			}
		}
	};

	template<> struct action< directive_romexport >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.ROMExport = true;
		}
	};

	template<> struct action< directive_incbin >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			try
			{
				const std::string filepath = Assembler::FilePath(ASM.IncludePathStack.front()) + '\\' + ASM.LastFileName;
				ASM.Incbin(filepath.c_str());
			}
			catch(std::runtime_error err)
			{
				std::cout << err.what() << "\n";
				ASM.ErrorMessage(ErrorIncbinFileFailed);
				return;
			}
		}
	};
	
	template<> struct action< directive_include >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			const std::string filepath = Assembler::FilePath(ASM.IncludePathStack.front()) + '\\' + ASM.LastFileName;
			ASM.IncludePathStack.push_back(filepath);
			try
			{
				pegtl::file_input<> inFile( filepath );
				try
				{
					pegtl::parse< DevonASM::grammar, DevonASM::action >(inFile, ASM);
				}
				catch (pegtl::parse_error & err)
				{
					ASM.IncludePathStack.pop_back();
					std::cout << err.what() << "\n";
					return;
				}
			}
			catch (std::system_error & err)
			{
				ASM.IncludePathStack.pop_back();
				std::cout << err.what() << "\n";
				ASM.ErrorMessage(ErrorIncludeFileFailed);
				return;
			}

			ASM.IncludePathStack.pop_back();
			ASM.LastFileName = "";
		}
	};
	
	template<> struct action< directive_byte >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			if(ASM.LastCharAvailable)
				ASM.AddByte(0);
		}
	};
	template<> struct action< byteintValue >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.AddByte(ASM.LastInt);
		}
	};
	template<> struct action< quotedstringCharValue >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.AddByte(ASM.LastInt);
		}
	};
	template<> struct action< directive_worddata >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.AddWord(ASM.LastInt);
		}
	};
	template<> struct action< directive_longdata >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.AddWord(ASM.LastInt>>16);
			ASM.AddWord(ASM.LastInt&0xFFFF);
		}
	};

	template<> struct action< registerop >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.LastRegisterOp = ASM.LastInt;
			//std::cout << "  OPREG = " << in.string() << '\n';
		}
	};
	template<> struct action< regSP >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.LastInt = 7;
		}
	};
	template<> struct action< regindex >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			std::string::size_type sz;
			ASM.LastInt = std::stoi(in.string(), &sz);
		}
	};
	template <CPU::EAdMode AM> struct action< AMCode<AM> >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.LastAdmode = AM;
			ASM.LastAdmodeOP = ASM.LastInt;
			//std::cout << "    AM = " << int(AM) << " = " << in.string() <<'\n';
		}
	};
	template<> struct action< admodeop0 >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.LastAdmode0 = ASM.LastAdmode;
			//std::cout << "    admode0 = " << in.string() << '\n';
		}
	};
	template<> struct action< admodeop1 >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.LastAdmode1 = ASM.LastAdmode;
			//std::cout << "    admode1 = " << in.string() << '\n';
		}
	};

	template<> struct action< symbolvalue >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.LastSymbol = in.string();
		}
	};

	template<> struct action< symbolreference >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.SymbolsIt = ASM.Symbols.find(in.string());
			if(ASM.SymbolsIt != ASM.Symbols.end())
			{
				ASM.LastInt = ASM.SymbolsIt->second;
			}
			else
			{
				if (ASM.CurPass == 0)
				{
					ASM.LastInt = -1;
				}
				else if (ASM.CurPass == 1)
				{
					ASM.ErrorMessage(ErrorUnknownSymbol);
				}
			}
		}
	};

	template<> struct action< integer >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.LastInt = std::accumulate(ASM.IntsToAdd.begin(), ASM.IntsToAdd.end(), 0);
			ASM.IntsToAdd.clear();
		}
	};
	template<> struct action< integerToAdd >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.IntsToAdd.push_back(ASM.LastInt);
		}
	};
	template<> struct action< decimalinteger >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			std::string::size_type sz;
			ASM.LastInt = std::stoi(in.string(), &sz);
		}
	};
	template<> struct action< hexainteger >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			std::string::size_type sz;
			ASM.LastInt = std::stoul(in.string(), &sz, 0);
		}
	};
	template<> struct action< binaryvalue >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			std::string::size_type sz;
			ASM.LastInt = std::stoul(in.string(), &sz, 2);
		}
	};
	template<> struct action< charvalue >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.LastInt = in.string()[0];
		}
	};
	template<> struct action< escapecharvalue >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			switch(in.string()[0])
			{
			case 'a':	ASM.LastInt = 0x07;		break;
			case 'b':	ASM.LastInt = 0x08;		break;
			case 'f':	ASM.LastInt = 0x0C;		break;
			case 'n':	ASM.LastInt = 0x0A;		break;
			case 'r':	ASM.LastInt = 0x0D;		break;
			case 't':	ASM.LastInt = 0x09;		break;
			case 'v':	ASM.LastInt = 0x0B;		break;
			case '0':	ASM.LastInt = 0x00;		break;
			case '\\':	ASM.LastInt = 0x5C;		break;
			case '\'':	ASM.LastInt = 0x27;		break;
			case '"':	ASM.LastInt = 0x22;		break;
			case '?':	ASM.LastInt = 0x3F;		break;
			}
		}
	};
	template<> struct action< label >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
			ASM.Symbols[ASM.LastSymbol] = ASM.CurAddress;
		}
	};
	template<> struct action< line >
	{
		template< typename Input > static void apply( const Input& in, Assembler & ASM)
		{
            const auto p = in.position();
			if(ASM.DumpErrorsThisLine(p.source, p.line))
				std::cout<<"    "<<in.string();
		}
	};

} // DevonASM
