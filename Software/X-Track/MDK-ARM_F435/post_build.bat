@echo off
echo Starting post-build process...

:: 获取 Keil 传递的输出文件路径
:: Keil 会将 !L 的值作为参数传递给批处理文件
set OUTPUT_FILE=%1
echo Input file: %1

:: 提取文件名（不含路径和扩展名）
set FILE_NAME=%~n1
echo Extracted file name: %FILE_NAME%
:: 1) 生成 .bin
"C:\Keil_v5\ARM\ARMCLANG\bin\fromelf.exe" --bin --bincombined --bincombined_base=0x8000000 --output=Bin\%FILE_NAME%.bin "%OUTPUT_FILE%"

:: 2) 生成 .dis 反汇编
"C:\Keil_v5\ARM\ARMCLANG\bin\fromelf.exe" --text -a -c -o Bin\%FILE_NAME%.dis "%OUTPUT_FILE%"

:: 3) 你的第3条命令（示例：复制文件）
..\Tools\keil-build-viewer.exe

:: 4) 你的第4条命令（示例：调用外部工具）
