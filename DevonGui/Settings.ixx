module;

#include <stdio.h>
#include <thread>
#include <mutex>
#include <fstream>
#include <streambuf>
#include <optional>
#include <cstring>

export module Settings;

namespace Settings
{
	export std::optional<std::string> ReadTxtFile(const std::string & FileName)
	{
		std::optional<std::string> Ret;
		std::ifstream tfile(FileName.c_str());
		if(tfile.good())
		{
			std::string t = std::string((std::istreambuf_iterator<char>(tfile)), std::istreambuf_iterator<char>());
			if(t.size() > 0)
				t.resize(t.size() - 1);
			Ret = t;
			tfile.close();
		}

		return Ret;
	}

	export std::string CartridgeFileName;
	export std::string DASFileName;
	export std::string ROMFileName = ".\\ROM.dro";

	std::string Old_CartridgeFileName;
	std::string Old_DASFileName;
	std::string Old_ROMFileName;

	struct Values
	{
		bool bShowWhiteSpaces = true;
		bool Show_Setup_Window = true;
		bool Show_MeshEditor_Window = true;
		bool Show_PShaderEditor_Window = true;
		bool Show_VShaderEditor_Window = true;
		bool Show_CRTShaderEditor_Window = true;
		bool Show_MeshTree_Window = false;
		bool bPerspective = true;
		bool bRunTime = true;
		float TransZ = 400.f;
		float TimeCoeff = 1.f;
		float FOV = 70.f;
		float Scanline = .3f;
		float Roundness = .6f;
		float BorderSharpness = .006f;
		float Vignetting = .6f;
		float Brightness = 1.7f;
		float Contrast = 0.f;
		float Sharpness = .1f;
		float GridDep = .85f;
		float GhostAmount = .15f;
		float ChromaAmount = .4f;
		float BloomAmount = .3f;
		float BloomRadius = 28.f;
		float Volume = .6f;
		float TEditorPalette = 0.f;

		bool operator == (Values & Rhs) { return 0 == std::memcmp(this, &Rhs, sizeof(Values)); }
	};

	export Values Current;
	Values OldSettings;

	std::atomic<bool> SaveIniFileRequest = false;
	std::atomic<bool> SavingIniFile = false;
	std::mutex SaveIniMutex;
	static const char * IniName = "Settings.ini";
	void SaveIniFileThreadFunc()
	{
		SaveIniFileRequest = false;
		SavingIniFile = true;

		do
		{
			FILE * f = nullptr;
			fopen_s(&f, IniName, "wt");
			if(f)
			{
				SaveIniMutex.lock();
				fprintf(f, "CartridgeFileName=%s\n",				CartridgeFileName.c_str());
				fprintf(f, "DASFileName=%s\n",						DASFileName.c_str());
				fprintf(f, "ROMFileName=%s\n",						ROMFileName.c_str());
				fprintf(f, "bShowWhiteSpaces=%d\n",					Current.bShowWhiteSpaces);
				fprintf(f, "Show_Setup_Window=%d\n",				Current.Show_Setup_Window);
				fprintf(f, "Show_MeshEditor_Window=%d\n",			Current.Show_MeshEditor_Window);
				fprintf(f, "Show_PShaderEditor_Window=%d\n",		Current.Show_PShaderEditor_Window);
				fprintf(f, "Show_VShaderEditor_Window=%d\n",		Current.Show_VShaderEditor_Window);
				fprintf(f, "Show_CRTShaderEditor_Window=%d\n",		Current.Show_CRTShaderEditor_Window);
				fprintf(f, "Show_MeshTree_Window=%d\n",				Current.Show_MeshTree_Window);
				fprintf(f, "bPerspective=%d\n",						Current.bPerspective);
				fprintf(f, "bRunTime=%d\n",							Current.bRunTime);
				fprintf(f, "TransZ=%f\n",							Current.TransZ);
				fprintf(f, "TimeCoeff=%f\n",						Current.TimeCoeff);
				fprintf(f, "FOV=%f\n",								Current.FOV);
				fprintf(f, "Scanline=%f\n",							Current.Scanline);
				fprintf(f, "Roundness=%f\n",						Current.Roundness);
				fprintf(f, "BorderSharpness=%f\n",					Current.BorderSharpness);
				fprintf(f, "Vignetting=%f\n",						Current.Vignetting);
				fprintf(f, "Brightness=%f\n",						Current.Brightness);
				fprintf(f, "Contrast=%f\n",							Current.Contrast);
				fprintf(f, "Sharpness=%f\n",						Current.Sharpness);
				fprintf(f, "GridDep=%f\n",							Current.GridDep);
				fprintf(f, "GhostAmount=%f\n",						Current.GhostAmount);
				fprintf(f, "ChromaAmount=%f\n",						Current.ChromaAmount);
				fprintf(f, "BloomAmount=%f\n",						Current.BloomAmount);
				fprintf(f, "BloomRadius=%f\n",						Current.BloomRadius);
				fprintf(f, "Volume=%f\n",							Current.Volume);
				fprintf(f, "TEditorPalette=%f\n",					Current.TEditorPalette);
				SaveIniMutex.unlock();

				fclose(f);
			}

//			Sleep(1000);
		}
		while(SaveIniFileRequest);

		SavingIniFile = false;
	}

	export void SaveIniSettings()
	{
		if(Current == OldSettings
		   && Old_CartridgeFileName == CartridgeFileName
		   && Old_DASFileName == DASFileName
		   && Old_ROMFileName == ROMFileName
		   )
			return;

		SaveIniFileRequest = true;

		if(SavingIniFile)
			return;

		OldSettings = Current;
		Old_CartridgeFileName = CartridgeFileName;
		Old_DASFileName = DASFileName;
		Old_ROMFileName = ROMFileName;

		std::thread WorkThread(SaveIniFileThreadFunc);
		WorkThread.detach();
	}

	export void LoadIniSettings()
	{
		if(auto IniOptional = ReadTxtFile(IniName))
		{
			std::string IniStr = IniOptional.value();

			size_t line = 0;
			do
			{
				auto TryGetBool = [IniStr, line](const char * fmt, bool & Value)
				{
					int IntValue;
					if(sscanf_s(IniStr.c_str() + line, fmt, &IntValue) == 1)
						Value = IntValue;
				};

				auto TryGetFloat = [IniStr, line](const char * fmt, float & Value)
				{
					float FloatValue;
					if(sscanf_s(IniStr.c_str() + line, fmt, &FloatValue) == 1)
						Value = FloatValue;
				};

				auto TryGetString = [IniStr, line](const char * fmt, std::string & Value)
				{
					const char * Start = IniStr.c_str() + line;
					if(0 == strncmp(Start, fmt, strlen(fmt)))
					{
						const char * ValStart = Start + strlen(fmt);
						if(const auto * br = strchr(ValStart, '\n'))
						{
							if(br - ValStart < 255)
							{
								char StrValue[256];
								strncpy_s(StrValue, ValStart, br - ValStart);
								StrValue[br - ValStart] = 0;
								Value = StrValue;
							}
						}
					}
				};

				TryGetString("CartridgeFileName=",				CartridgeFileName);
				TryGetString("DASFileName=",					DASFileName);
				TryGetString("ROMFileName=",					ROMFileName);

				TryGetBool("bShowWhiteSpaces=%d",				Current.bShowWhiteSpaces);
				TryGetBool("Show_Setup_Window=%d",				Current.Show_Setup_Window);
				TryGetBool("Show_MeshEditor_Window=%d",			Current.Show_MeshEditor_Window);
				TryGetBool("Show_PShaderEditor_Window=%d",		Current.Show_PShaderEditor_Window);
				TryGetBool("Show_VShaderEditor_Window=%d",		Current.Show_VShaderEditor_Window);
				TryGetBool("Show_CRTShaderEditor_Window=%d",	Current.Show_CRTShaderEditor_Window);
				TryGetBool("Show_MeshTree_Window=%d",			Current.Show_MeshTree_Window);
				TryGetBool("bPerspective=%d",					Current.bPerspective);
				TryGetBool("bRunTime=%d",						Current.bRunTime);

				TryGetFloat("TransZ=%f",						Current.TransZ);
				TryGetFloat("TimeCoeff=%f",						Current.TimeCoeff);
				TryGetFloat("FOV=%f",							Current.FOV);
				TryGetFloat("Scanline=%f",						Current.Scanline);
				TryGetFloat("Roundness=%f",						Current.Roundness);
				TryGetFloat("BorderSharpness=%f",				Current.BorderSharpness);
				TryGetFloat("Vignetting=%f",					Current.Vignetting);
				TryGetFloat("Brightness=%f",					Current.Brightness);
				TryGetFloat("Contrast=%f",						Current.Contrast);
				TryGetFloat("Sharpness=%f",						Current.Sharpness);
				TryGetFloat("GridDep=%f",						Current.GridDep);
				TryGetFloat("GhostAmount=%f",					Current.GhostAmount);
				TryGetFloat("ChromaAmount=%f",					Current.ChromaAmount);
				TryGetFloat("BloomAmount=%f",					Current.BloomAmount);
				TryGetFloat("BloomRadius=%f",					Current.BloomRadius);
				TryGetFloat("Volume=%f",						Current.Volume);
				TryGetFloat("TEditorPalette=%f",				Current.TEditorPalette);

				line = IniStr.find("\n", line) + 1;
			}
			while(line != 0);
		}
	}

};