del /F /Q F:\Desktop\VM2\Devon16\*.*
del /F /Q F:\Desktop\VM2\Devon16_32\*.*

xcopy /Y x64\Release\DevonGui.exe F:\Desktop\VM2\Devon16\
xcopy /Y DevonGui\*.das F:\Desktop\VM2\Devon16\
xcopy /Y DevonGui\*.bin F:\Desktop\VM2\Devon16\
xcopy /Y DevonGui\*.dro F:\Desktop\VM2\Devon16\

call "F:\Program Files\7-Zip\7z.exe" a F:\Desktop\VM2\devon16_win64.zip F:\Desktop\VM2\Devon16\*.*

xcopy /Y Release\DevonGui.exe F:\Desktop\VM2\Devon16_32\
xcopy /Y DevonGui\*.das F:\Desktop\VM2\Devon16_32\
xcopy /Y DevonGui\*.bin F:\Desktop\VM2\Devon16_32\
xcopy /Y DevonGui\*.dro F:\Desktop\VM2\Devon16_32\

call "F:\Program Files\7-Zip\7z.exe" a F:\Desktop\VM2\devon16_win32.zip F:\Desktop\VM2\Devon16_32\*.*

pause
