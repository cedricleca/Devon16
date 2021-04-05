#pragma once

#include <memory>
#include "../WavFileReader/wav_file_reader.h"

struct WaveToolWindow
{
	bool Show = false;
	std::unique_ptr<sakado::WavFileReader> WavFileReaderPtr;

	bool LoadWave(const std::string & filename)
	{
		try
		{
			WavFileReaderPtr = std::make_unique<sakado::WavFileReader>(filename);
			Show = true;
			return true;
		}
		catch(std::exception e)
		{
			Show = false;
			return false;
		}
	}

	void Draw(const char * title)
    {
		if(WavFileReaderPtr)
		{
			ImGui::SetNextWindowSize(ImVec2(500,400), ImGuiCond_FirstUseEver);
			ImGui::Begin(title, &Show);

			ImGui::Text("FmtSize %d", WavFileReaderPtr->FmtSize);
			ImGui::Text("FmtID %d", WavFileReaderPtr->FmtID);
			ImGui::Text("NumChannels %d", WavFileReaderPtr->NumChannels);
			ImGui::Text("SampleRate %d", WavFileReaderPtr->SampleRate);
			ImGui::Text("BytesPerSec %d", WavFileReaderPtr->BytesPerSec);
			ImGui::Text("BlockAlign %d", WavFileReaderPtr->BlockAlign);
			ImGui::Text("BitsPerSample %d", WavFileReaderPtr->BitsPerSample);
			ImGui::Text("DataSize %d", WavFileReaderPtr->DataSize);
			ImGui::Text("BytesPerSample %d", WavFileReaderPtr->BytesPerSample);
			ImGui::Text("NumData %d", WavFileReaderPtr->NumData);

	        ImGui::End();
		}
	}
};
