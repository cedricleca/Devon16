#pragma once

#include "atlimage.h"
#include "atltypes.h"

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
		
		ImGui::Text("palette entries %d", BMPImage.GetMaxColorTableEntries() );
		ImGui::Text("%d x %d", BMPImage.GetWidth(), BMPImage.GetHeight() );
		
		if(ImGui::Button("Export Palette"))
		{
			char filename[ MAX_PATH ];
			OPENFILENAMEA ofn;
			ZeroMemory( &filename, sizeof( filename ) );
			ZeroMemory( &ofn,      sizeof( ofn ) );
			ofn.lStructSize  = sizeof( ofn );
			ofn.hwndOwner    = NULL;  // If you have a window to center over, put its HANDLE here
			ofn.lpstrFilter  = "Devon ASM Files (.das)\0*.das\0Any File\0*.*\0";
			ofn.lpstrFile    = filename;
			ofn.nMaxFile     = MAX_PATH;
			ofn.lpstrTitle   = "Select a File";
			ofn.Flags        = OFN_DONTADDTORECENT;
  
			if (GetSaveFileNameA( &ofn ))
			{
				FILE * f;
				fopen_s(&f, filename, "wt");
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
		}

		ImGui::SameLine();
		ImGui::SetNextItemWidth(50.f);
		ImGui::DragInt("Colors to Export", &NbColorsToExport, 1, 1, 256);

		if(ImGui::Button("Export BitPlanes"))
		{
			char filename[ MAX_PATH ];
			OPENFILENAMEA ofn;
			ZeroMemory( &filename, sizeof( filename ) );
			ZeroMemory( &ofn,      sizeof( ofn ) );
			ofn.lStructSize  = sizeof( ofn );
			ofn.hwndOwner    = NULL;  // If you have a window to center over, put its HANDLE here
			ofn.lpstrFilter  = "Binary Files (.bin)\0*.bin\0Any File\0*.*\0";
			ofn.lpstrFile    = filename;
			ofn.nMaxFile     = MAX_PATH;
			ofn.lpstrTitle   = "Select a File";
			ofn.Flags        = OFN_DONTADDTORECENT;

			if (GetSaveFileNameA( &ofn ))
			{
				FILE * f;
				fopen_s(&f, filename, "wb");
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
									for(int  PalIdx = 0; PalIdx < BMPPalette.size(); PalIdx++)
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
		}

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

