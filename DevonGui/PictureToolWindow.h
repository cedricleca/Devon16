#pragma once

#include "pcx\pcx.h"

struct PictureToolWindow
{
	bool Show = false;
	PCX_Image PcxImage;
	int NbColorsToExport = 32;
	int NbBitPlanesToExport = 5;
	
    void Draw(const char* title)
    {
        ImGui::SetNextWindowSize(ImVec2(500,400), ImGuiCond_FirstUseEver);
        ImGui::Begin(title, &Show);
		
		ImGui::Columns(3, 0, false);
		ImGui::SetColumnWidth(0, 250.0f);
		ImGui::SetColumnWidth(1, 150.0f);

		ImGui::Text("version %d", PcxImage.hdr.version);
		ImGui::Text("encoding %d", PcxImage.hdr.encoding);
		ImGui::Text("bitsPerPixel %d", PcxImage.hdr.bitsPerPixel);
		ImGui::Text("res = %d x %d", PcxImage.width, PcxImage.height);
		ImGui::Text("bytesPerLine %d", PcxImage.hdr.bytesPerLine);
		ImGui::Text("paletteInfo %d", PcxImage.hdr.paletteInfo );
		
		ImGui::NextColumn();
		
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
						int col = ((PcxImage.pal[i].r>>4)<<8) | ((PcxImage.pal[i].g>>4)<<4) | (PcxImage.pal[i].b>>4);
						fprintf(f, "\tword 0x%03x\n", col);
					}
	
					fclose(f);
				}
			}
		}

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
						for(int y = 0; y < PcxImage.height; y++)
						{
							for(int x = 0; x < PcxImage.width;)
							{
								unsigned short out = 0;
								for(int w = 0; w < 16 && x < PcxImage.width; w++, x++)
								{
									unsigned char pixdata = PcxImage.bufr[y*PcxImage.hdr.bytesPerLine + x];
									if((pixdata & (1<<bpl)) != 0)
										out |= (1<<(15-w));
								}
								out = (out<<8) | (out>>8
									);
								fwrite(&out, sizeof(out), 1, f);
							}
						}
					}

					fclose(f);
				}
			}
		}

		ImGui::NextColumn();

		ImGui::DragInt("Colors to Export", &NbColorsToExport, 1, 1, 256);
		ImGui::DragInt("BitPlanes to Export", &NbBitPlanesToExport, 1, 1, 8);

		ImGui::Columns(1);
		ImGui::Separator();

		for(int i = 0; i < 16; i++)
		{
			for(int j = 0; j < 16; j++)
			{
				int Index = i*16 + j;
				ImVec4 Col;
				if(PcxImage.PaletteLoaded)
				{
					Col.x = float(PcxImage.pal[Index].r & 0xf0) / 255.0f;
					Col.y = float(PcxImage.pal[Index].g & 0xf0) / 255.0f;
					Col.z = float(PcxImage.pal[Index].b & 0xf0) / 255.0f;
				}
				else
				{
					Col.x = float(PcxImage.hdr.colormap[Index*3+0] & 0xf0) / 255.0f;
					Col.y = float(PcxImage.hdr.colormap[Index*3+1] & 0xf0) / 255.0f;
					Col.z = float(PcxImage.hdr.colormap[Index*3+2] & 0xf0) / 255.0f;
				}
				ImGui::ColorButton("MyColor##3b", Col, ImGuiColorEditFlags_NoAlpha, ImVec2(16,16));

				if(j < 15)
					ImGui::SameLine();
			}
		}

        ImGui::End();
    }
};

