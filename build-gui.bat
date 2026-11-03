:: MP8! Hardcore Player GUI - Build Script
:: 
:: Copyright (c) 2026 SimpleTools Studio. All rights reserved.
:: Licensed under the SimpleTools Studio HalfOpenSource License (SHL) v1.0.
:: See the LICENSE file for full terms.
:: ========================================================

@echo off
echo Compiling MP8 GUI Player...
gcc player_gui.c -o mp8_gui.exe -O2 -mwindows -lcomctl32 -lole32 -lshell32 -static -s
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)
echo Build successful! Run mp8_gui.exe
pause