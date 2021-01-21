// ImGui - standalone example application for DirectX 11
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include <imgui.h>
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <dsound.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <math.h>
#include <tchar.h>
#include <string>
#include <sstream>
#include <atomic>
#include <algorithm>    // std::transform
#include <iterator>
#include <thread>
#include "..\DevonASMLib\DASM.h"
#include "..\devon\DevonMachine.h"
#include "MemoryEditor.h"
#include "DisassemblyWindow.h"
#include "LogWindow.h"
#include "TextEditor.h"
#include "PictureToolWindow.h"
#include "DXTools.h"
#include "DSoundTools.h"
#include <fstream>
#include <streambuf>

// Data
static DevonASM::Assembler ASM;
static std::string DASFileName;
static std::string CartridgeFileName;
static std::string ROMFileName = ".\\ROM.dro";
static const char * IniName = "DevonGui.ini";
static std::string DCAExportName;
static std::string DROExportName;
static int TEditorPalette = 0;
static PictureToolWindow PicToolWindow;

static unsigned char * ROM = nullptr;
static long ROMSize = 0;
static unsigned char * Cartridge = nullptr;
static long CartridgeSize = 0;
static bool AssemblySuccess = false;
static bool AssemblyDone = false;
static bool Show_TextEditor_Whitespaces = true;
static float Volume = 0.5f;
static float CRTRoundness = 0.15f;
static float CRTScanline = 0.05f;

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

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            ImGui_ImplDX11_InvalidateDeviceObjects();
            DXTools::CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            DXTools::CreateRenderTarget();
            ImGui_ImplDX11_CreateDeviceObjects();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

static void SaveIniSettingsToDisk()
{
    // Write .ini file
	FILE * f;
	fopen_s(&f, IniName, "wt");
	if(f)
	{
		if(CartridgeFileName.length() > 0)
			fprintf(f, "dcafile=%s\n", CartridgeFileName.c_str());
		if(DASFileName.length() > 0)
			fprintf(f, "dasfile=%s\n", DASFileName.c_str());
		if(ROMFileName.length() > 0)
			fprintf(f, "romfile=%s\n", ROMFileName.c_str());

		fprintf(f, "Palette=%d\n", TEditorPalette);
		fprintf(f, "ShowWhitespaces=%d\n", (int)Show_TextEditor_Whitespaces);
		fprintf(f, "Volume=%f\n", Volume);
		fprintf(f, "CRTRoundness=%f\n", CRTRoundness);
		fprintf(f, "CRTScanline=%f\n", CRTScanline);

		fclose(f);
	}
}

static void LoadIniSettingsFromDisk()
{
	FILE * f;
	fopen_s(&f, IniName, "rt");
	if(f)
	{
		fseek(f, 0, SEEK_END);
		size_t IniSize = ftell(f);
		rewind(f);
		char * IniDat = new char[IniSize];
		fread_s(IniDat, IniSize, 1, IniSize, f);
		fclose(f);

		const char* buf_end = IniDat + IniSize;
		for (const char* line_start = IniDat; line_start < buf_end; )
		{
			const char* line_end = line_start;
			while (line_end < buf_end && *line_end != '\n' && *line_end != '\r')
				line_end++;

			char Value[256];
			int IntValue;
			float FloatValue;
			if (sscanf_s(line_start, "dcafile=%s", Value, (unsigned)_countof(Value)) == 1)
                CartridgeFileName = Value;
            else if (sscanf_s(line_start, "dasfile=%s", Value, (unsigned)_countof(Value)) == 1)
                DASFileName = Value;
            else if (sscanf_s(line_start, "romfile=%s", Value, (unsigned)_countof(Value)) == 1)
                ROMFileName = Value;
			else if (sscanf_s(line_start, "Palette=%d", &IntValue) == 1)
				TEditorPalette = IntValue;
			else if (sscanf_s(line_start, "ShowWhitespaces=%d", &IntValue) == 1)
				Show_TextEditor_Whitespaces = IntValue;
			else if (sscanf_s(line_start, "Volume=%f", &FloatValue) == 1)
				Volume = FloatValue;
			else if (sscanf_s(line_start, "CRTRoundness=%f", &FloatValue) == 1)
				CRTRoundness = FloatValue;
			else if (sscanf_s(line_start, "CRTScanline=%f", &FloatValue) == 1)
				CRTScanline = FloatValue;

			line_start = line_end+1;
		}

		return;
	}
}

bool LoadROM(const char * ROMFileName, unsigned char* & ROM, long & ROMSize)
{
	FILE * f;
	fopen_s(&f, ROMFileName, "rb");
	if(f)
	{
		if(ROM != nullptr)
		{
			ROMSize = 0;
			delete ROM;
		}
		fseek(f, 0, SEEK_END);
		ROMSize = ftell(f);
		rewind(f);
		ROM = new unsigned char[ROMSize];
		fread_s(ROM, ROMSize, 1, ROMSize, f);
		fclose(f);
		return true;
	}

	return false;
}

MemoryEditor::u16 ReadMMUWord(MemoryEditor::u16* data, size_t off)
{
	Devon::uWORD Val=-1;
	Machine.MMU.ReadWord(Val, (uLONG)off, true);
	return Val;
}
void WriteMMUWord(MemoryEditor::u16* data, size_t off, MemoryEditor::u16 d)
{
	Machine.MMU.WriteWord(d, (uLONG)off, true);
}

void PlugCartridge(unsigned char * Cartridge, long CartridgeSize, LogWindow & Log)
{
	if(CartridgeFileName.length() > 0)
	{
		if(LoadROM(CartridgeFileName.c_str(), Cartridge, CartridgeSize))
		{
			Machine.MMU.PlugCartrige((uWORD*)Cartridge, CartridgeSize/sizeof(uWORD));
			Log.AddLog("Cartridge file loaded\n");
			Machine.HardReset();
		}
		else
		{
			Log.AddLog("Can't load Cartridge file %s\n", CartridgeFileName);
			Log.Show = true;
		}
	}
}

void PlugROM(unsigned char * ROM, long ROMSize, LogWindow & Log)
{
	if(ROMFileName.length() > 0)
	{
		if(LoadROM(ROMFileName.c_str(), ROM, ROMSize))
		{
			Machine.MMU.SetROM((uWORD*)ROM, ROMSize/sizeof(uWORD));
			Log.AddLog("ROM file loaded\n");
			Machine.HardReset();
		}
		else
		{
			Log.AddLog("Can't load ROM file %s\n", ROMFileName.c_str());
			Log.Show = true;
		}
	}
}

std::atomic<bool> DASLoadDone = false;
void SetDASFile()
{
	char filename[ MAX_PATH ];
	OPENFILENAMEA ofn;
	ZeroMemory( &filename, sizeof( filename ) );
	ZeroMemory( &ofn,      sizeof( ofn ) );
	ofn.lStructSize  = sizeof( ofn );
	ofn.hwndOwner    = nullptr;  // If you have a window to center over, put its HANDLE here
	ofn.lpstrFilter  = "Devon Asm Files\0*.das\0Any File\0*.*\0";
	ofn.lpstrFile    = filename;
	ofn.nMaxFile     = MAX_PATH;
	ofn.lpstrTitle   = "Select a File";
	ofn.Flags        = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
  
	if (GetOpenFileNameA( &ofn ))
	{
		DASFileName = filename;
		SaveIniSettingsToDisk();

		AssemblyDone = false;
		DASLoadDone = true;
	}
}

void SetROMFile(LogWindow & LogWindow)
{
	char filename[ MAX_PATH ];
	OPENFILENAMEA ofn;
	ZeroMemory( &filename, sizeof( filename ) );
	ZeroMemory( &ofn,      sizeof( ofn ) );
	ofn.lStructSize  = sizeof( ofn );
	ofn.hwndOwner    = nullptr;  // If you have a window to center over, put its HANDLE here
	ofn.lpstrFilter  = "Devon ROM Files\0*.dro\0Any File\0*.*\0";
	ofn.lpstrFile    = filename;
	ofn.nMaxFile     = MAX_PATH;
	ofn.lpstrTitle   = "Select a File";
	ofn.Flags        = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
  
	if (GetOpenFileNameA( &ofn ))
	{
		ROMFileName = filename;
		PlugROM(ROM, ROMSize, LogWindow);
		SaveIniSettingsToDisk();
	}
}

void SetDCAFile(LogWindow & LogWindow)
{
	char filename[ MAX_PATH ];
	OPENFILENAMEA ofn;
	ZeroMemory( &filename, sizeof( filename ) );
	ZeroMemory( &ofn,      sizeof( ofn ) );
	ofn.lStructSize  = sizeof( ofn );
	ofn.hwndOwner    = nullptr;  // If you have a window to center over, put its HANDLE here
	ofn.lpstrFilter  = "Devon Cartridge Files\0*.dca\0Any File\0*.*\0";
	ofn.lpstrFile    = filename;
	ofn.nMaxFile     = MAX_PATH;
	ofn.lpstrTitle   = "Select a File";
	ofn.Flags        = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
  
	if (GetOpenFileNameA( &ofn ))
	{
		CartridgeFileName = filename;
		SaveIniSettingsToDisk();
		PlugCartridge(Cartridge, CartridgeSize, LogWindow);
	}
}

std::atomic<bool> CartridgeReadyToPlugin = false;
void ExportCartridge(std::string Filename, LogWindow & LogWindow)
{
	DCAExportName = Filename;
	std::string Extension = DCAExportName.substr(DCAExportName.size()-4);
	std::string LowExtension;
	std::transform(Extension.begin(), Extension.end(), std::back_inserter(LowExtension), tolower);
	if(LowExtension != ".dca")
		DCAExportName.append(".dca");
	std::stringstream outstream;
	ScopedRedirect redirect(std::cout, outstream);
	bool Success = ASM.ExportROMFile(DCAExportName.c_str(), 0x20000, 0x20000, 0x200);
	LogWindow.AddLog("%s\n", outstream.str().c_str());

	if(Success)
	{
		CartridgeFileName = DCAExportName;
		SaveIniSettingsToDisk();
		CartridgeReadyToPlugin = true;
	}
}

void ExportCartridge(LogWindow & LogWindow)
{
	char filename[ MAX_PATH ];
	OPENFILENAMEA ofn;
	ZeroMemory( &filename, sizeof( filename ) );
	ZeroMemory( &ofn,      sizeof( ofn ) );
	ofn.lStructSize  = sizeof( ofn );
	ofn.hwndOwner    = nullptr;  // If you have a window to center over, put its HANDLE here
	ofn.lpstrFilter  = "Devon Cartridge Files (.dca)\0*.dca\0Any File\0*.*\0";
	ofn.lpstrFile    = filename;
	ofn.nMaxFile     = MAX_PATH;
	ofn.lpstrTitle   = "Select a File";
	ofn.Flags        = OFN_DONTADDTORECENT;
  
	if (GetSaveFileNameA( &ofn ))
		ExportCartridge(filename, LogWindow);
}

void ExportROM(std::string Filename, LogWindow & LogWindow)
{
	DROExportName = Filename;
	std::string Extension = DROExportName.substr(DROExportName.size()-4);
	std::string LowExtension;
	std::transform(Extension.begin(), Extension.end(), std::back_inserter(LowExtension), tolower);
	if(LowExtension != ".dro")
		DROExportName.append(".dro");
	std::stringstream outstream;
	ScopedRedirect redirect(std::cout, outstream);
	bool Success = ASM.ExportROMFile(DROExportName.c_str(), 0x0, 0x10000);
	LogWindow.AddLog("%s\n", DROExportName.c_str());

	if(Success)
	{
		ROMFileName = DROExportName;
		PlugROM(ROM, ROMSize, LogWindow);
	}
	else
	{
		LogWindow.AddLog(outstream.str().c_str());
		LogWindow.Show = true;
	}
}

void ExportROM(LogWindow & LogWindow)
{
	char filename[ MAX_PATH ];
	OPENFILENAMEA ofn;
	ZeroMemory( &filename, sizeof( filename ) );
	ZeroMemory( &ofn,      sizeof( ofn ) );
	ofn.lStructSize  = sizeof( ofn );
	ofn.hwndOwner    = nullptr;  // If you have a window to center over, put its HANDLE here
	ofn.lpstrFilter  = "Devon ROM Files (.dro)\0*.dro\0Any File\0*.*\0";
	ofn.lpstrFile    = filename;
	ofn.nMaxFile     = MAX_PATH;
	ofn.lpstrTitle   = "Select a File";
	ofn.Flags        = OFN_DONTADDTORECENT;
  
	if (GetSaveFileNameA( &ofn ))
		ExportROM(filename, LogWindow);
}

void SaveDASFile(TextEditor & Teditor, bool bForceDialog=false)
{
	if(DASFileName.empty() || bForceDialog)
	{
		char filename[ MAX_PATH ];
		OPENFILENAMEA ofn;
		ZeroMemory( &filename, sizeof( filename ) );
		ZeroMemory( &ofn,      sizeof( ofn ) );
		ofn.lStructSize  = sizeof( ofn );
		ofn.hwndOwner    = nullptr;  // If you have a window to center over, put its HANDLE here
		ofn.lpstrFilter  = "Devon ASM Files (.das)\0*.das\0Any File\0*.*\0";
		ofn.lpstrFile    = filename;
		ofn.nMaxFile     = MAX_PATH;
		ofn.lpstrTitle   = "Select a File";
		ofn.Flags        = OFN_DONTADDTORECENT;
  
		if (GetSaveFileNameA( &ofn ))
			DASFileName = filename;
	}

	if(!DASFileName.empty())
		Teditor.SaveText(DASFileName);
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
	AssemblySuccess = ASM.AssembleFile(DASFileName.c_str()) && (ASM.NbErrors == 0);
	AssemblyDone = true;
	LogWindow.Show = true;
	
	if(AssemblySuccess)
	{
		std::string outfname = DASFileName;
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

void LaunchImageTool()
{
	char filename[ MAX_PATH ];
	OPENFILENAMEA ofn;
	ZeroMemory( &filename, sizeof( filename ) );
	ZeroMemory( &ofn,      sizeof( ofn ) );
	ofn.lStructSize  = sizeof( ofn );
	ofn.hwndOwner    = nullptr;  // If you have a window to center over, put its HANDLE here
	ofn.lpstrFilter  = "PCX Files (.dca)\0*.pcx\0Any File\0*.*\0";
	ofn.lpstrFile    = filename;
	ofn.nMaxFile     = MAX_PATH;
	ofn.lpstrTitle   = "Select a File";
	ofn.Flags        = OFN_DONTADDTORECENT;
  
	if (GetOpenFileNameA( &ofn ))
	{
		if(PicToolWindow.PcxImage.Load(filename))
		{
			PicToolWindow.Show = true;
		}
	}
}

std::atomic<bool> StartCompileThread = false;
std::atomic<bool> StartOpenDASThread = false;
bool IsWorkThreadBusy() { return StartCompileThread || StartOpenDASThread; }
std::atomic<bool> WorkThreadQuit = false;
void WorkThreadFunc(LogWindow* _LogWindow)
{
	while (!WorkThreadQuit)
	{
		Sleep(200);
		if (StartCompileThread)
		{
			AssembleAndExport(*_LogWindow);
			StartCompileThread = false;
		}

		if (StartOpenDASThread)
		{
			SetDASFile();
			StartOpenDASThread = false;
		}
	}
}

int main(int, char**)
{
    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, _T("Devon16"), nullptr };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(_T("Devon16"), _T("Devon 16 (press F11 to switch control UI)"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

	TextEditor Teditor;
	auto lang = TextEditor::LanguageDefinition::DevonASM();
	Teditor.SetLanguageDefinition(lang);

	// Initialize Direct3D
    if(!DXTools::CreateDeviceD3D(hwnd))
    {
        DXTools::CleanupDeviceD3D();
        UnregisterClass(_T("Devon16"), wc.hInstance);
        return 1;
    }

	DSoundTools::Init(hwnd, Machine);

	// Show the window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup ImGui binding
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImGui::PushStyleColor(ImGuiCol_TitleBg,				ImColor(0xFF015AE3).Value);
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive,		ImColor(0xFF015AE3 + 0x00111111).Value);
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed,	ImColor(0xFF015AE3 - 0x00001111).Value);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,		ImColor(0xFF717C4C).Value);
    ImGui::PushStyleColor(ImGuiCol_Button,				ImColor(0xFF717C4C - 0x00001111).Value);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,		ImColor(0xFF717C4C + 0x00222211).Value);
    ImGui::PushStyleColor(ImGuiCol_WindowBg,			ImColor(0xEE2E261F).Value);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,			ImColor(0xEE2E261F / 2).Value);

	// Load Fonts
    // (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
    //ImGuiIO& io = ImGui::GetIO();
    //io.Fonts->AddFontDefault();
    //ImFont* pFont = io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());

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
	LogWindow LogWindow;

	LoadIniSettingsFromDisk();
	switch(TEditorPalette)
	{
	case 0:	Teditor.SetPalette(TextEditor::GetDarkPalette());		break;
	case 1:	Teditor.SetPalette(TextEditor::GetLightPalette());		break;
	case 2:	Teditor.SetPalette(TextEditor::GetRetroBluePalette());	break;
	}
	Teditor.SetShowWhitespaces(Show_TextEditor_Whitespaces);

	// text editor 
	{
		std::ifstream tfile(DASFileName.c_str());
		if (tfile.good())
		{
			std::string str((std::istreambuf_iterator<char>(tfile)), std::istreambuf_iterator<char>());
			Teditor.SetText(str);
			tfile.close();
		}
	}

	PlugROM(ROM, ROMSize, LogWindow);
	PlugCartridge(Cartridge, CartridgeSize, LogWindow);

	ImVec4 clear_col = ImColor(61, 61, 61);

	std::thread WorkThread(WorkThreadFunc, &LogWindow);	

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        
		ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

		if (Show_UI)
		{
			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("New .das", ""))
					{
						DASFileName = "";
						Teditor.SetText("");
					}
					if (ImGui::MenuItem("Open .das...", "", false, !IsWorkThreadBusy()))
					{
						StartOpenDASThread = true;
					}
					if (ImGui::MenuItem("Save .das", "Ctrl-S"))
					{
						SaveDASFile(Teditor);
					}
					if (ImGui::MenuItem("Save .das As...", ""))
					{
						SaveDASFile(Teditor, true);
					}

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
					if (!DASFileName.empty() && ImGui::MenuItem("Assemble", "Ctrl+F7", nullptr, !IsWorkThreadBusy()))
						StartCompileThread = true;

					if (ImGui::MenuItem("Symbols", "", Show_SymbolList_Window, AssemblySuccess && AssemblyDone))
						Show_SymbolList_Window = !Show_SymbolList_Window;

					if (ImGui::MenuItem("Export Cartridge (.dca) File", "", false, AssemblySuccess && AssemblyDone))
						ExportCartridge(LogWindow);

					if (ImGui::MenuItem("Export Rom (.dro) File", "", false, AssemblySuccess && AssemblyDone))
						ExportROM(LogWindow);

					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Machine"))
				{
					if (ImGui::MenuItem("Set ROM File", ""))
						SetROMFile(LogWindow);

					if (ImGui::MenuItem("Set Cartridge File", ""))
						SetDCAFile(LogWindow);

					if (ImGui::MenuItem("General Reset", "F9"))
						Machine.HardReset();

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Tools"))
				{
					if (ImGui::MenuItem("Image Tool", "", false, !PicToolWindow.Show))
						LaunchImageTool();

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("View"))
				{
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

					ImGui::Separator();

					if (ImGui::MenuItem("Dark palette"))
					{
						Teditor.SetPalette(TextEditor::GetDarkPalette());
						TEditorPalette = 0;
						SaveIniSettingsToDisk();
					}
					if (ImGui::MenuItem("Light palette"))
					{
						Teditor.SetPalette(TextEditor::GetLightPalette());
						TEditorPalette = 1;
						SaveIniSettingsToDisk();
					}
					if (ImGui::MenuItem("Retroblue palette"))
					{
						Teditor.SetPalette(TextEditor::GetRetroBluePalette());
						TEditorPalette = 2;
						SaveIniSettingsToDisk();
					}
					if (ImGui::MenuItem("Show Whitespaces", "Ctrl+8", Show_TextEditor_Whitespaces))
					{
						Show_TextEditor_Whitespaces = !Show_TextEditor_Whitespaces;
						Teditor.SetShowWhitespaces(Show_TextEditor_Whitespaces);
						SaveIniSettingsToDisk();
					}

					ImGui::EndMenu();
				}

				ImGui::EndMainMenuBar();
			}

			//		bool bShowTestWindow = true;
			//		ImGui::ShowTestWindow(&bShowTestWindow);

					//        ImGui::SetNextWindowSize(ImVec2(600,400));
			ImGui::Begin("Devon");

			if (ImGui::IsKeyPressed(VK_F9, false))
				Machine.HardReset();

			if (ImGui::Button("Set ROM (.dro) File"))
				SetROMFile(LogWindow);

			if (!ROMFileName.empty())
			{
				ImGui::SameLine();
				ImGui::Text(ROMFileName.c_str());
			}

			if (ImGui::Button("Set Cartridge (.dca) File"))
				SetDCAFile(LogWindow);

			if (!CartridgeFileName.empty())
			{
				ImGui::SameLine();
				ImGui::Text(CartridgeFileName.c_str());
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

			if (!DASFileName.empty() && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(VK_F7, false))
				StartCompileThread = true;

			if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed('S', false))
				SaveDASFile(Teditor);

			if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed('8', false))
			{
				Show_TextEditor_Whitespaces = !Show_TextEditor_Whitespaces;
				Teditor.SetShowWhitespaces(Show_TextEditor_Whitespaces);
				SaveIniSettingsToDisk();
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

			if(ImGui::SliderFloat("Sound Volume", &Volume, 0.0f, 1.0f, "%.2f")
				|| ImGui::SliderFloat("CRT Roundness", &CRTRoundness, 0.0f, 0.3f, "%.2f")
				|| ImGui::SliderFloat("CRT Scanline", &CRTScanline, 0.0f, 1.0f, "%.2f")
				)
				SaveIniSettingsToDisk();

			if (!PicToolWindow.Show && ImGui::Button("Image Tool"))
				LaunchImageTool();

			ImGui::End();

			if (PicToolWindow.Show)
				PicToolWindow.Draw("Image Tool");

			if (Show_TextEditor_Window)
				TextEditor::EditorWindow(Teditor, DASFileName, &Show_TextEditor_Window);

			if (CartridgeReadyToPlugin)
			{
				PlugCartridge(Cartridge, CartridgeSize, LogWindow);
				CartridgeReadyToPlugin = false;
			}

			if (DASLoadDone)
			{
				Show_SymbolList_Window = false;

				std::ifstream t(DASFileName.c_str());
				if (t.good())
				{
					std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
					Teditor.SetText(str);
					Show_TextEditor_Window = true;
					t.close();
				}
				DASLoadDone = false;
			}
		}

		if (ImGui::IsKeyPressed(VK_F11, false))
			Show_UI = !Show_UI;

		DXTools::UpdateConstants(CRTRoundness, CRTScanline);

		unsigned char * pTexels = DXTools::MapEmulationTexture();

		// Machine evaluation
		Machine.Cortico.SetOutputSurface(pTexels, 512, 512);
		Machine.KeyB.PushKeyEvents();
		Machine.TickFrame();

		DXTools::UnmapEmulationTexture();

		DSoundTools::Render(Machine, Volume);
		DXTools::Render((float*)&clear_col, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
		
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
    }

	WorkThreadQuit = true;
	WorkThread.join();

	DSoundTools::Release();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

	DXTools::CleanupDeviceD3D();
    UnregisterClass(_T("Devon16"), wc.hInstance);

    return 0;
}
