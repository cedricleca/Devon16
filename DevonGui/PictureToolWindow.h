#pragma once

#include "atlimage.h"
#include "atltypes.h"
#include "ImFileDialog.h"
#include <functional>

struct PictureToolWindow
{
	bool Show = false;
	int NbColorsToExport = 32;
	int NbBitPlanesToExport = 5;

	CImage BMPImage;
	std::vector<RGBQUAD> BMPPalette;
	bool LoadBMP(const char * filename)
	{
		if(E_FAIL != BMPImage.Load(filename))
		{
			NbColorsToExport = BMPImage.GetMaxColorTableEntries();
			NbBitPlanesToExport = BMPImage.GetBPP();
			BMPPalette.resize(NbColorsToExport);
			BMPImage.GetColorTable(0, NbColorsToExport, BMPPalette.data());
			return true;
		}
		return false;
	}
	
    void Draw(const char* title)
    {
        ImGui::SetNextWindowSize(ImVec2(500,400), ImGuiCond_FirstUseEver);
        ImGui::Begin(title, &Show);
		
		ImGui::Text("%d x %d, ", BMPImage.GetWidth(), BMPImage.GetHeight() );
		ImGui::SameLine();
		ImGui::Text("%d colors", BMPImage.GetMaxColorTableEntries() );

		const int nbwordsperline = ((BMPImage.GetWidth() - 1) / 16) + 1;
		const int BPLSize = nbwordsperline * BMPImage.GetHeight();
		ImGui::Text("each bitplane is %d (0x%x) words long", BPLSize, BPLSize);

		auto OnClosedFileDialog = [&](const char * label, const std::function<void( const std::string & )> & f)
		{
			if (ifd::FileDialog::Instance().IsDone(label)) 
			{
				if (ifd::FileDialog::Instance().HasResult()) 
					f(std::move(ifd::utf8_encode(ifd::FileDialog::Instance().GetResult())));

				ifd::FileDialog::Instance().Close();
			}
		};

		if(ImGui::Button("Export Palette"))
			ifd::FileDialog::Instance().Save("SavePaletteDialog", "Save palette as a Devon ASM File", "DAS file (*.das){.das},.*");

		OnClosedFileDialog("SavePaletteDialog", [&](const std::string & filename)  
		{
			try
			{
				FILE * f;
				fopen_s(&f, filename.c_str(), "wt");
				if(f)
				{
					for(int i = 0; i < NbColorsToExport; i++)
					{
						const auto & Entry = BMPPalette[i];
						const int col = ((Entry.rgbRed>>4)<<8) | ((Entry.rgbGreen>>4)<<4) | (Entry.rgbBlue>>4);
						fprintf(f, "\tword 0x%03x\n", col);
					}
	
					fclose(f);
				}
			}
			catch(const std::exception & err)
			{
				std::cout << err.what() << '/n';
			}
		});

		ImGui::SameLine();
		ImGui::SetNextItemWidth(50.f);
		ImGui::DragInt("Colors to Export", &NbColorsToExport, 1, 1, 256);

		if(ImGui::Button("Export BitPlanes"))
			ifd::FileDialog::Instance().Save("ExportBitPlanesDialog", "Export bitplanes as a Binary File", "BIN file (*.bin){.bin},.*");

		OnClosedFileDialog("ExportBitPlanesDialog", [&](const std::string & filename)  
		{
			try
			{
				FILE * f;
				fopen_s(&f, filename.c_str(), "wb");
				if(f)
				{
					for(int bpl = 0; bpl < NbBitPlanesToExport; bpl++)
					{
						for(int y = 0; y < BMPImage.GetHeight(); y++)
						{
							for(int x = 0; x < BMPImage.GetWidth();)
							{
								unsigned short out = 0;
								for(int w = 0; w < 16 && x < BMPImage.GetWidth(); w++, x++)
								{
									COLORREF CRef = BMPImage.GetPixel(x, y);
									for(unsigned int PalIdx = 0; PalIdx < BMPPalette.size(); PalIdx++)
									{
										if(GetBValue(CRef) == BMPPalette[PalIdx].rgbBlue
											&& GetGValue(CRef) == BMPPalette[PalIdx].rgbGreen
											&& GetRValue(CRef) == BMPPalette[PalIdx].rgbRed
											)
										{
											if(PalIdx & (1 << bpl))
												out |= 1 << (15-w);

											break;
										}
									}
								}
								out = (out<<8) | (out>>8);
								fwrite(&out, sizeof(out), 1, f);
							}
						}
					}

					fclose(f);
				}
			}
			catch(const std::exception & err)
			{
				std::cout << err.what() << '/n';
			}
		});

		ImGui::SameLine();
		ImGui::SetNextItemWidth(50.f);
		ImGui::DragInt("BitPlanes to Export", &NbBitPlanesToExport, 1, 1, 8);

		ImGui::Separator();

		for(int i = 0; i < 16; i++)
		{
			for(int j = 0; j < 16; j++)
			{
				const uint32_t Index = i*16 + j;
				if(Index < BMPPalette.size())
				{
					auto & Entry = BMPPalette[Index];
					ImVec4 Col(	float(Entry.rgbRed & 0xf0) / 255.0f,
								float(Entry.rgbGreen & 0xf0) / 255.0f,
								float(Entry.rgbBlue & 0xf0) / 255.0f, 
								1.f
								);
					ImGui::ColorButton("MyColor##3b", Col, ImGuiColorEditFlags_NoAlpha, ImVec2(20,20));
				}
				else
				{
					ImGui::ColorButton("MyColor##3b", ImVec4(), ImGuiColorEditFlags_NoAlpha, ImVec2(20,20));
				}

				if(j < 15)
					ImGui::SameLine();
			}
		}

        ImGui::End();
    }
};

