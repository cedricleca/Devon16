// ImGui - standalone example application for DirectX 11
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include "windows.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

//#include <dsound.h>
//#define DIRECTINPUT_VERSION 0x0800

#include <math.h>
#include <tchar.h>
#include <string>
#include <sstream>
#include <atomic>
#include <algorithm>    // std::transform
#include <iterator>
#include <thread>
#include <sys/types.h>
#include <sys/stat.h> 
#include <fstream>
#include <streambuf>

#include "..\DevonASMLib\DASM.h"
#include "..\devon\DevonMachine.h"
#include "MemoryEditor.h"
#include "DisassemblyWindow.h"
#include "LogWindow.h"
#include "TextEditor.h"
#include "PictureToolWindow.h"
#include "ImFileDialog.h"
#include "resource1.h"

import SoundTools;
import GLTools;
import Settings;

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

// Data
static DevonASM::Assembler ASM;
static const char * IniName = "DevonGui.ini";
static std::string DCAExportName;
static std::string DROExportName;
time_t ROMFileTimeStamp;
time_t CARTFileTimeStamp;

std::vector<char> ROMBuf;
std::vector<char> CartridgeBuf;
static bool AssemblySuccess = false;
static bool AssemblyDone = false;

static DevonMachine Machine;

class ScopedRedirect
{
public:
	ScopedRedirect(std::ostream& inOriginal, std::stringstream& inRedirect) :
		mOriginal(inOriginal),
		mOldBuffer(inOriginal.rdbuf(inRedirect.rdbuf()))
	{ }
	ScopedRedirect(std::ostream& inOriginal, std::ostream& inRedirect) :
		mOriginal(inOriginal),
		mOldBuffer(inOriginal.rdbuf(inRedirect.rdbuf()))
	{ }

	~ScopedRedirect()
	{
		mOriginal.rdbuf(mOldBuffer);
	}    

private:
	ScopedRedirect(const ScopedRedirect&);
	ScopedRedirect& operator=(const ScopedRedirect&);

	std::ostream & mOriginal;
	std::streambuf * mOldBuffer;
};

bool LoadROM(const std::string & ROMFileName, std::vector<char> & OutBuf)
{
	std::ifstream input( ROMFileName, std::ios::binary );
	if(input.is_open())
	{
		OutBuf = { std::istreambuf_iterator<char>(input), {} };
		return true;
	}
	return false;
}

MemoryEditor::u16 ReadMMUWord(MemoryEditor::u16* data, size_t off)
{
	Devon::uWORD Val;
	Machine.MMU.ReadWord<DevonMMU::NoFail>(Val, (uLONG)off);
	return Val;
}
void WriteMMUWord(MemoryEditor::u16* data, size_t off, MemoryEditor::u16 d)
{
	Machine.MMU.WriteWord<DevonMMU::NoFail>(d, (uLONG)off);
}

void PlugCartridge(LogWindow & Log)
{
	if(Settings::CartridgeFileName.length() > 0)
	{
		if(LoadROM(Settings::CartridgeFileName, CartridgeBuf))
		{
			Machine.MMU.PlugCartrige(CartridgeBuf);
			Log.AddLog("Cartridge file loaded\n");
			Machine.HardReset();
		}
		else
		{
			Log.AddLog("Can't load Cartridge file %s\n", Settings::CartridgeFileName);
			Log.Show = true;
		}
	}
}

std::atomic<bool> ROMChangeRequest = false;
void PlugROM(LogWindow & Log)
{
	if(Settings::ROMFileName.length() > 0)
	{
		if(LoadROM(Settings::ROMFileName, ROMBuf))
		{
			Log.AddLog("ROM file loaded\n");
			ROMChangeRequest = true;
		}
		else
		{
			Log.AddLog("Can't load ROM file %s\n", Settings::ROMFileName.c_str());
			Log.Show = true;
		}
	}
}

void SetROMFile()
{
	ifd::FileDialog::Instance().Open("OpenDroDialog", "Open a DRO cartridge file", "DRO file (*.dro){.dro},.*");
}

void SetDCAFile()
{
	ifd::FileDialog::Instance().Open("OpenDcaDialog", "Open a DCA cartridge file", "DCA file (*.dca){.dca},.*");
}

void ExportCartridge()
{
	ifd::FileDialog::Instance().Save("SaveDcaDialog", "Save a Devon Cartridge file", "DCA file (*.dca){.dca},.*");
}

void LaunchImageTool()
{
	ifd::FileDialog::Instance().Open("OpenImgDialog", "Open a BMP file", "BMP file (*.bmp){.bmp},.*");
}

void ExportROM()
{
	ifd::FileDialog::Instance().Save("SaveDroDialog", "Save a Devon Rom file", "DRO file (*.dro){.dro},.*");
}

void SaveDASFile(TextEditor & Teditor, bool bForceDialog=false)
{
	if(Settings::DASFileName.empty() || bForceDialog)
		ifd::FileDialog::Instance().Save("SaveDasDialog", "Save a Devon ASM Source file", "DAS file (*.das){.das},.*");

	if(!Settings::DASFileName.empty())
		Teditor.SaveText(Settings::DASFileName);
}

std::string CheckFileExtension(const std::string & Filename, const std::string & Extension)
{
	std::string ExistingExtension = Filename.substr(Filename.size()-4);
	std::string LowExtension;
	std::transform(ExistingExtension.begin(), ExistingExtension.end(), std::back_inserter(LowExtension), [](unsigned char c){ return std::tolower(c); });

	std::string Ret = Filename;
	if(LowExtension != Extension)
		Ret.append(Extension);
	return Ret;
}

bool UnplugCartridgeRequest = false;
std::atomic<bool> CartridgeReadyToPlugin = false;
void ExportCartridge(std::string Filename, LogWindow & LogWindow)
{
	DCAExportName = CheckFileExtension(Filename, ".dca");
	std::stringstream outstream;
	ScopedRedirect redirect(std::cout, outstream);
	bool Success = ASM.ExportROMFile(DCAExportName.c_str(), 0x20000, 0x20000, 0x200);
	LogWindow.AddLog("%s\n", outstream.str().c_str());

	if(Success)
	{
		Settings::Lock(true);
		Settings::CartridgeFileName = DCAExportName;
		Settings::Lock(false);
	}
}

void ExportROM(std::string Filename, LogWindow & LogWindow)
{
	DROExportName = CheckFileExtension(Filename, ".dro");
	std::stringstream outstream;
	ScopedRedirect redirect(std::cout, outstream);
	bool Success = ASM.ExportROMFile(DROExportName.c_str(), 0x0, 0x10000);
	LogWindow.AddLog("%s\n", DROExportName.c_str());

	if(Success)
	{
		Settings::Lock(true);
		Settings::ROMFileName = DROExportName;
		Settings::Lock(false);
	}
	else
	{
		LogWindow.AddLog(outstream.str().c_str());
		LogWindow.Show = true;
	}
}

struct logstream : public std::ostream, std::streambuf
{
	LogWindow & Log;
	logstream(LogWindow & _Log) : Log(_Log), std::ostream(this) {}
	int overflow(int c) override
	{
		Log.AddLog(c);
		return 0;
	}
};

void AssembleAndExport(LogWindow & LogWindow)
{
	logstream LogStream(LogWindow);
	ScopedRedirect redirect(std::cout, LogStream);
	AssemblySuccess = ASM.AssembleFile(Settings::DASFileName.c_str()) && (ASM.NbErrors == 0);
	AssemblyDone = true;
	LogWindow.Show = true;

	if(AssemblySuccess)
	{
		std::string outfname = Settings::DASFileName;
		size_t idx = outfname.find(".das");
		if(idx >= 0)
		{
			if(ASM.ROMExport)
			{
				outfname.replace(idx, 4, ".dro");
				ExportROM(outfname, LogWindow);
			}
			else
			{
				outfname.replace(idx, 4, ".dca");
				ExportCartridge(outfname, LogWindow);
			}
		}
	}
}

std::atomic<bool> StartCompileThread = false;
bool IsWorkThreadBusy() { return StartCompileThread; }
std::atomic<bool> WorkThreadQuit = false;
void WorkThreadFunc(LogWindow* _LogWindow)
{
	int NbIterToFileCheck = 0;
	while(!WorkThreadQuit)
	{
		Sleep(200);
		if(StartCompileThread)
		{
			AssembleAndExport(*_LogWindow);
			StartCompileThread = false;
		}

		// check if rom files have changed and need relaoding
		if(NbIterToFileCheck == 0)
		{
			struct stat st;
			Settings::Lock(true);
			stat(Settings::ROMFileName.c_str(), &st);
			Settings::Lock(false);
			time_t ROMftime = st.st_mtime;
			if(ROMftime > ROMFileTimeStamp)
			{
				PlugROM(*_LogWindow);
				ROMFileTimeStamp = ROMftime;
			}

			Settings::Lock(true);
			stat(Settings::CartridgeFileName.c_str(), &st);
			Settings::Lock(false);
			time_t CARTftime = st.st_mtime;
			if(CARTftime > CARTFileTimeStamp)
			{
				CartridgeReadyToPlugin = true;
				CARTFileTimeStamp = CARTftime;
			}

			NbIterToFileCheck = 5;
		}
		else
		{
			NbIterToFileCheck--;
		}
	}
}

int main(int argn, char**arg)
{
	// Setup window
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
	const char* glsl_version = "#version 100";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
	// GL 3.2 + GLSL 150
	const char* glsl_version = "#version 150";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

	auto HasArg = [argn, arg](const char * ToTest)->bool 
	{
		for(int i = 0; i < argn; i++)
			if(0 == std::strcmp(arg[i], ToTest))
				return true;
		return false;
	};

	// Create window with graphics context
	int W = 1280;
	int H = 720;
	GLFWmonitor * Monitor = HasArg("-fullscreen") ? glfwGetPrimaryMonitor() : nullptr;
	if(Monitor)
	{
		if(auto * VideoMode = glfwGetVideoMode(Monitor))
		{
			W = VideoMode->width;
			H = VideoMode->height;
		}
	}

	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
	char WinTitle[] = "Devon16 (F11 to switch UI, F9 to reset";
	GLFWwindow* window = glfwCreateWindow(W, H, WinTitle, Monitor, nullptr);
	if (window == nullptr)
		return 1;

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

						 // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
	bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
	bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
	bool err = gladLoadGL() == 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
	bool err = gladLoadGL(glfwGetProcAddress) == 0; // glad2 recommend using the windowing library loader instead of the (optionally) bundled one.
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
	bool err = false;
	glbinding::Binding::initialize();
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
	bool err = false;
	glbinding::initialize([](const char* name) { return (glbinding::ProcAddress)glfwGetProcAddress(name); });
#else
	bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
	if (err)
	{
		fprintf(stderr, "Failed to initialize OpenGL loader!\n");
		return 1;
	}

	TextEditor Teditor;
	auto lang = TextEditor::LanguageDefinition::DevonASM();
	Teditor.SetLanguageDefinition(lang);

	if(auto hwndFound = FindWindow(nullptr, WinTitle))
	{
		DSoundTools::Init(hwndFound, Machine);

		HICON hIcon = LoadIcon(GetModuleHandleA(nullptr), MAKEINTRESOURCE(IDI_ICON1));
		SendMessage(hwndFound, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		SendMessage(hwndFound, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	}

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// ImFileDialog requires you to set the CreateTexture and DeleteTexture
	ifd::FileDialog::Instance().CreateTexture = GLTools::FileDiagCreateTex;

	ifd::FileDialog::Instance().DeleteTexture = [](void* tex) 
	{
		union { GLuint tex[2]; void * V = tex; } In;
		glDeleteTextures(1, &In.tex[0]);
	};

	ImGui::PushStyleColor(ImGuiCol_TitleBg,				ImColor(0xFF015AE3).Value);
	ImGui::PushStyleColor(ImGuiCol_TitleBgActive,		ImColor(0xFF015AE3 + 0x00111111).Value);
	ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed,	ImColor(0xFF015AE3 - 0x00001111).Value);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered,		ImColor(0xFF717C4C).Value);
	ImGui::PushStyleColor(ImGuiCol_Button,				ImColor(0xFF717C4C - 0x00001111).Value);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,		ImColor(0xFF717C4C + 0x00222211).Value);
	ImGui::PushStyleColor(ImGuiCol_WindowBg,			ImColor(0xEE2E261F).Value);
	ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,			ImColor(0xEE2E261F / 2).Value);

	// Load Fonts
	ImFont * HackFont = io.Fonts->AddFontFromFileTTF("HackRegular.TTF", 16.0f);

	bool Show_UI = false;
	bool Show_Disassembly_Window = true;
	bool Show_Memory_Window = false;
	bool Show_SymbolList_Window = false;
	bool Show_TextEditor_Window = true;
	MemoryEditor MemWindow;
	MemWindow.OptMidRowsCount = 0;
	MemWindow.OptAddrDigitsCount = 5;
	MemWindow.ReadFn = ReadMMUWord;
	MemWindow.WriteFn = WriteMMUWord;
	DisassemblyWindow DisassemblyWindow;
	LogWindow GeneralLog;
	GeneralLog.Show = true;
	LogWindow LogWindow;
	TextEditor PShaderEditor;
	PShaderEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
	TextEditor VShaderEditor;
	VShaderEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
	TextEditor CRTShaderEditor;
	CRTShaderEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());

	Settings::LoadIniSettings();

	if(auto txt = Settings::ReadTxtFile("CRTShader.glsl"))
		CRTShaderEditor.SetText(txt.value());
	if(auto txt = Settings::ReadTxtFile(Settings::DASFileName.c_str()))
		Teditor.SetText(txt.value());

	GLTools::GlVersion();

	auto CompileShaders = [&GeneralLog, &PShaderEditor, &VShaderEditor, &CRTShaderEditor]()
	{
		GLTools::ReCompileShaders(GLTools::ShaderId::CRT, GeneralLog, CRTShaderEditor.GetText().c_str(), GLTools::PPVS);
	};

	CompileShaders();
	GLTools::CompileShaders(GLTools::ShaderId::BloomH, GeneralLog, GLTools::BloomHPS, GLTools::PPVS);
	GLTools::CompileShaders(GLTools::ShaderId::BloomV, GeneralLog, GLTools::BloomVPS, GLTools::PPVS);
	GLTools::CompileShaders(GLTools::ShaderId::Final, GeneralLog, GLTools::FinalPS, GLTools::PPVS);
	GLTools::ConstructQuadBuffers();
	GLTools::ConstructRenderTargets(1280, 720);

	auto SetTEditorSettings = [](TextEditor & TE)
	{
		switch(int(Settings::Current.TEditorPalette))
		{
		case 0:	TE.SetPalette(TextEditor::GetDarkPalette());		break;
		case 1:	TE.SetPalette(TextEditor::GetLightPalette());		break;
		case 2:	TE.SetPalette(TextEditor::GetRetroBluePalette());	break;
		}
		TE.SetShowWhitespaces(int(Settings::Current.TEditorPalette));
	};
	SetTEditorSettings(Teditor);
	SetTEditorSettings(CRTShaderEditor);

	PlugROM(LogWindow);
	PlugCartridge(LogWindow);

	{
		struct stat st;
		Settings::Lock(true);
		stat(Settings::ROMFileName.c_str(), &st);
		Settings::Lock(false);
		time_t ROMftime = st.st_mtime;
		ROMFileTimeStamp = ROMftime;

		Settings::Lock(true);
		stat(Settings::CartridgeFileName.c_str(), &st);
		Settings::Lock(false);
		time_t CARTftime = st.st_mtime;
		CARTFileTimeStamp = CARTftime;
	}

	ImVec4 clear_col = ImColor(61, 61, 61);

	std::thread WorkThread(WorkThreadFunc, &LogWindow);	

	auto SetDASFileName = [](const std::string & name) 
	{
		Settings::Lock(true);
		Settings::DASFileName = name;
		Settings::Lock(false);
	};

	bool bQuitRequest = false;
	// Main loop
	while(!glfwWindowShouldClose(window) && !bQuitRequest)
	{
		glfwPollEvents();

		Settings::SaveIniSettings();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (Show_UI)
		{
			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("New .das", ""))
						SetDASFileName("");
					if (ImGui::MenuItem("Open .das...", "", false, !IsWorkThreadBusy()))
						ifd::FileDialog::Instance().Open("OpenDasDialog", "Open a Devon ASM Source file", "DAS file (*.das){.das},.*");
					if (ImGui::MenuItem("Save .das", "Ctrl-S"))
						SaveDASFile(Teditor);
					if (ImGui::MenuItem("Save .das As...", ""))
						SaveDASFile(Teditor, true);
					if (ImGui::MenuItem("Quit", ""))
						bQuitRequest = true;

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Edit"))
				{
					bool ro = Teditor.IsReadOnly();
					if (ImGui::MenuItem("Read-only mode", nullptr, &ro))
						Teditor.SetReadOnly(ro);
					ImGui::Separator();

					if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && Teditor.CanUndo()))
						Teditor.Undo();
					if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && Teditor.CanRedo()))
						Teditor.Redo();

					ImGui::Separator();

					if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, Teditor.HasSelection()))
						Teditor.Copy();
					if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !ro && Teditor.HasSelection()))
						Teditor.Cut();
					if (ImGui::MenuItem("Delete", "Del", nullptr, !ro && Teditor.HasSelection()))
						Teditor.Delete();
					if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
						Teditor.Paste();

					ImGui::Separator();

					if (ImGui::MenuItem("Select all", nullptr, nullptr))
						Teditor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(Teditor.GetTotalLines(), 0));

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Assembler"))
				{
					if (!Settings::DASFileName.empty() && ImGui::MenuItem("Assemble", "Ctrl+F7", nullptr, !IsWorkThreadBusy()))
						StartCompileThread = true;

					if (ImGui::MenuItem("Symbols", "", Show_SymbolList_Window, AssemblySuccess && AssemblyDone))
						Show_SymbolList_Window = !Show_SymbolList_Window;

					if (ImGui::MenuItem("Export Cartridge (.dca) File", "", false, AssemblySuccess && AssemblyDone))
						ExportCartridge();

					if (ImGui::MenuItem("Export Rom (.dro) File", "", false, AssemblySuccess && AssemblyDone))
						ExportROM();

					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Machine"))
				{
					if (ImGui::MenuItem("Set ROM File", ""))
						SetROMFile();

					if (ImGui::MenuItem("Set Cartridge File", ""))
						SetDCAFile();

					if (ImGui::MenuItem("General Reset", "F9"))
						Machine.HardReset();

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Tools"))
				{
					if (ImGui::MenuItem("Image Tool", "", false, !PictureToolWindow::GetShow()))
						LaunchImageTool();

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("View"))
				{
					Settings::Lock(true);

					if (ImGui::MenuItem(".das Editor", "", Show_TextEditor_Window))
						Show_TextEditor_Window = !Show_TextEditor_Window;

					if (ImGui::MenuItem("Output", "", LogWindow.Show))
						LogWindow.Show = !LogWindow.Show;

					if (ImGui::MenuItem("Memory", "", Show_Memory_Window))
						Show_Memory_Window = !Show_Memory_Window;

					if (ImGui::MenuItem("CPU Control", "", Show_Disassembly_Window))
						Show_Disassembly_Window = !Show_Disassembly_Window;

					if (ImGui::MenuItem("Symbols", "", Show_SymbolList_Window, AssemblySuccess && AssemblyDone))
						Show_SymbolList_Window = !Show_SymbolList_Window;

					if (ImGui::MenuItem("CRT Shader Editor", "", Settings::Current.Show_CRTShaderEditor_Window))
						Settings::Current.Show_CRTShaderEditor_Window = !Settings::Current.Show_CRTShaderEditor_Window;				

					ImGui::Separator();

					if (ImGui::MenuItem("Dark palette"))
					{
						Teditor.SetPalette(TextEditor::GetDarkPalette());
						CRTShaderEditor.SetPalette(TextEditor::GetDarkPalette());
						Settings::Current.TEditorPalette = 0;
					}
					if (ImGui::MenuItem("Light palette"))
					{
						Teditor.SetPalette(TextEditor::GetLightPalette());
						CRTShaderEditor.SetPalette(TextEditor::GetLightPalette());
						Settings::Current.TEditorPalette = 1;
					}
					if (ImGui::MenuItem("Retroblue palette"))
					{
						Teditor.SetPalette(TextEditor::GetRetroBluePalette());
						CRTShaderEditor.SetPalette(TextEditor::GetRetroBluePalette());
						Settings::Current.TEditorPalette = 2;
					}
					if (ImGui::MenuItem("Show Whitespaces", "Ctrl+8", Settings::Current.bShowWhiteSpaces))
					{
						Settings::Current.bShowWhiteSpaces = !Settings::Current.bShowWhiteSpaces;
						Teditor.SetShowWhitespaces(Settings::Current.bShowWhiteSpaces);
						CRTShaderEditor.SetShowWhitespaces(Settings::Current.bShowWhiteSpaces);
					}

					Settings::Lock(false);

					ImGui::EndMenu();
				}

				ImGui::EndMainMenuBar();
			}

			//bool bShowTestWindow = true;
			//ImGui::ShowDemoWindow(&bShowTestWindow);

			//        ImGui::SetNextWindowSize(ImVec2(600,400));
			ImGui::Begin("Devon");

			if(ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F5, false))
				CompileShaders();

			if (ImGui::Button("Set ROM (.dro) File"))
				SetROMFile();

			if (!Settings::ROMFileName.empty())
			{
				ImGui::SameLine();
				ImGui::Text(Settings::ROMFileName.c_str());
			}

			if (ImGui::Button("Set Cartridge (.dca) File"))
				SetDCAFile();

			if (!Settings::CartridgeFileName.empty())
			{
				ImGui::SameLine();
				ImGui::Text(Settings::CartridgeFileName.c_str());

				if(ImGui::Button("Unplug Cartridge"))
					UnplugCartridgeRequest = true;
			}

			ImGui::Separator();
			if (LogWindow.Show)
				LogWindow.Draw("Output");

			if (Show_Memory_Window)
			{
				MemWindow.DrawWindow("Memory", 0, 0xA0000);
				Show_Memory_Window = MemWindow.Open;
			}

			if (Show_Disassembly_Window)
			{
				DisassemblyWindow.DrawWindow("CPU Control", Machine.MMU, Machine.CPU, 0);
				Show_Disassembly_Window = DisassemblyWindow.Open;
			}

			if (!Settings::DASFileName.empty() && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F7, false))
			{
				SaveDASFile(Teditor);
				StartCompileThread = true;
			}

			if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_S, false))
			{
				SaveDASFile(Teditor);
				CRTShaderEditor.SaveText("CRTShader.glsl");
			}

			if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_8, false))
			{
				Settings::Lock(true);
				Settings::Current.bShowWhiteSpaces = !Settings::Current.bShowWhiteSpaces;
				Teditor.SetShowWhitespaces(Settings::Current.bShowWhiteSpaces);
				CRTShaderEditor.SetShowWhitespaces(Settings::Current.bShowWhiteSpaces);
				Settings::Lock(false);
			}

			if (Show_SymbolList_Window && AssemblySuccess)
			{
				ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
				ImGui::Begin("Symbol List", &Show_SymbolList_Window);
				ImGui::BeginChild("##scrolling", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
				std::map<std::string, int>::iterator SymbolsIt;

				ImGui::Columns(3);

				for (SymbolsIt = ASM.Symbols.begin(); SymbolsIt != ASM.Symbols.end(); SymbolsIt++)
					ImGui::Text(SymbolsIt->first.c_str());

				ImGui::NextColumn();
				for (SymbolsIt = ASM.Symbols.begin(); SymbolsIt != ASM.Symbols.end(); SymbolsIt++)
					ImGui::Text("0x%06x", SymbolsIt->second);

				ImGui::NextColumn();
				for (SymbolsIt = ASM.Symbols.begin(); SymbolsIt != ASM.Symbols.end(); SymbolsIt++)
					ImGui::Text("%d", SymbolsIt->second);

				ImGui::Columns(1);

				ImGui::EndChild();
				ImGui::End();
			}

			Settings::Lock(true);
			ImGui::SliderFloat("Sound Volume",		&Settings::Current.Volume, 0.0f, 1.0f, "%.2f");
			ImGui::Text("");
			int OscIdx = 0;
			const char * OscLabels[] = {"Osc0","Osc1","Osc2","Osc3"};
			for(auto & Oscillo : Machine.JKev.OscilloTab)
			{
				std::array<float, 80> X;
				for(int i = 0; i < 80; i++)
					X[i] = Oscillo.Data[(Oscillo.OscilloWriteCursor + i) % Oscillo.Data.size()];
				ImGui::SameLine(0, 15.f);
				ImGui::PlotLines(OscLabels[OscIdx], X.data(), (int)X.size(), 0, nullptr, -128.0f, 128.0f, ImVec2(80.f, 80.0f));
				++OscIdx;
			}

			ImGui::Text("CRT effect:");
			ImGui::SliderFloat("Scanline",			&Settings::Current.Scanline, 0.f, 1.f);
			ImGui::SliderFloat("Roundness",			&Settings::Current.Roundness, 0.f, 1.f);
			ImGui::SliderFloat("BorderSharpness",	&Settings::Current.BorderSharpness, 0.02f, .0006f, "%.5f");
			ImGui::SliderFloat("Vignetting",		&Settings::Current.Vignetting, 0.f, 1.f);
			ImGui::SliderFloat("Brightness",		&Settings::Current.Brightness, 0.0f, 2.f);
			ImGui::SliderFloat("Contrast",			&Settings::Current.Contrast, -1.0f, 1.f);
			ImGui::SliderFloat("Sharpness",			&Settings::Current.Sharpness, 0.0f, 2.f);
			ImGui::SliderFloat("GridDep",			&Settings::Current.GridDep, 0.0f, 1.f);
			ImGui::SliderFloat("GhostAmount",		&Settings::Current.GhostAmount, 0.0f, 1.f);
			ImGui::SliderFloat("ChromaAmount",		&Settings::Current.ChromaAmount, 0.0f, 1.f);
			ImGui::SliderFloat("BloomAmount",		&Settings::Current.BloomAmount, 0.0f, 1.f);
			ImGui::SliderFloat("BloomRadius",		&Settings::Current.BloomRadius, 1.f, 100.f);
			Settings::Lock(false);

			if (!PictureToolWindow::GetShow() && ImGui::Button("Image Tool"))
				LaunchImageTool();

			ImGui::End();

			auto OnClosedFileDialog = [&](const char * label, const std::function<void( const std::string & )> & f)
			{
				if (ifd::FileDialog::Instance().IsDone(label)) 
				{
					if (ifd::FileDialog::Instance().HasResult()) 
						f(std::move(ifd::utf8_encode(ifd::FileDialog::Instance().GetResult())));

					ifd::FileDialog::Instance().Close();
				}
			};

			// open DRO file dialog result
			OnClosedFileDialog("OpenDroDialog", [&](const std::string & filename)  
			{
				Settings::Lock(true);
				Settings::ROMFileName = filename;
				Settings::Lock(false);
				ROMFileTimeStamp = 0; // trig a DRO relaod
			});

			// open DCA file dialog result
			OnClosedFileDialog("OpenDcaDialog", [&](const std::string & filename)  
			{
				Settings::Lock(true);
				Settings::CartridgeFileName = filename;
				Settings::Lock(false);
				CARTFileTimeStamp = 0; // trig a CART reload
			});

			// save DCA file dialog result
			OnClosedFileDialog("SaveDcaDialog", [&](const std::string & filename)  
			{
				ExportCartridge(filename.c_str(), LogWindow);
			});

			// DRO file dialog result
			OnClosedFileDialog("SaveDroDialog", [&](const std::string & filename)  
			{
				ExportROM(filename.c_str(), LogWindow);
			});

			// BMP file dialog result
			OnClosedFileDialog("OpenImgDialog", [&](const std::string & filename)  
			{
				if(PictureToolWindow::LoadBMP(filename.c_str()))
					PictureToolWindow::SetShow(true);
			});

			// DAS file dialog result
			OnClosedFileDialog("OpenDasDialog", [&](const std::string & filename)  
			{
				SetDASFileName(filename);
				Show_SymbolList_Window = false;
				AssemblyDone = false;

				if(auto DASStr = Settings::ReadTxtFile(Settings::DASFileName.c_str()))
				{
					Teditor.SetText(DASStr.value());
					Show_TextEditor_Window = true;
				}
			});

			// DAS file dialog result
			OnClosedFileDialog("SaveDasDialog", [&](const std::string & filename)  
			{
				SetDASFileName(filename);
				SaveDASFile(Teditor);
				if(!Settings::DASFileName.empty())
					Teditor.SaveText(Settings::DASFileName);
			});

			if(PictureToolWindow::GetShow())
				PictureToolWindow::Draw("Image Tool");

			if (Show_TextEditor_Window)
				TextEditor::EditorWindow(Teditor, Settings::DASFileName, &Show_TextEditor_Window);

			if(Settings::Current.Show_CRTShaderEditor_Window)
			{
				Settings::Lock(true);
				TextEditor::EditorWindow(CRTShaderEditor, "CRT Shader (F5 to compile)", &Settings::Current.Show_CRTShaderEditor_Window);
				Settings::Lock(false);
			}
		}

		if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F9, false))
			Machine.HardReset();

		if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F11, false))
			Show_UI = !Show_UI;

		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		GLTools::UpdateRenderTargets(display_w, display_h); // maybe resize

															// Machine evaluation
		if(CartridgeReadyToPlugin)
		{
			PlugCartridge(LogWindow);
			CartridgeReadyToPlugin = false;
		}

		if(UnplugCartridgeRequest)
		{
			Settings::Lock(true);
			Settings::CartridgeFileName = "";
			Settings::Lock(false);

			Machine.MMU.UnplugCartridge();
			Machine.HardReset();
			UnplugCartridgeRequest = false;
		}

		if(ROMChangeRequest)
		{
			Machine.MMU.SetROM(ROMBuf);
			Machine.HardReset();
			ROMChangeRequest = false;
		}

		Machine.Cortico.SetOutputSurface((unsigned char *)GLTools::PrimaryBuffer.data());
		Machine.KeyB.PushKeyEvents(!Show_UI);
		Machine.TickFrame();

		DSoundTools::Render(Machine, Settings::Current.Volume);

		GLTools::SetupPostProcessRenderStates();
		GLTools::UpdatePrimaryTexture();
		GLTools::SetRenderTarget(GLTools::RenderTextureId::CRT);
		GLTools::RenderCRT(GLTools::RenderTextureId::Primary, display_w, display_h, 
						   Settings::Current.Scanline, 
						   Settings::Current.Roundness, 
						   Settings::Current.BorderSharpness,
						   Settings::Current.Vignetting,
						   Settings::Current.Brightness,
						   Settings::Current.Contrast,
						   Settings::Current.Sharpness,
						   Settings::Current.GridDep,
						   Settings::Current.GhostAmount,
						   Settings::Current.ChromaAmount
		);

		// Bloom H
		GLTools::SetRenderTarget(GLTools::RenderTextureId::BloomH);
		GLTools::RenderBloomH(GLTools::RenderTextureId::CRT, display_w, display_h, GLTools::PrimaryH, Settings::Current.BloomRadius);

		// Bloom V
		GLTools::SetRenderTarget(GLTools::RenderTextureId::BloomV);
		GLTools::RenderBloomV(GLTools::RenderTextureId::BloomH, display_w, display_h, GLTools::PrimaryH, Settings::Current.BloomRadius);

		GLTools::SetRenderTarget(GLTools::RenderTextureId::Max);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		GLTools::RenderFinal(GLTools::RenderTextureId::CRT, GLTools::RenderTextureId::BloomV, display_w, display_h, Settings::Current.BloomAmount);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}

	WorkThreadQuit = true;
	WorkThread.join();

	DSoundTools::Release();

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	GLTools::DestroyShaders(GLTools::ShaderId::CRT);
	GLTools::DestroyShaders(GLTools::ShaderId::BloomH);
	GLTools::DestroyShaders(GLTools::ShaderId::BloomV);
	GLTools::DestroyShaders(GLTools::ShaderId::Final);
	GLTools::DestroyBuffers();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
