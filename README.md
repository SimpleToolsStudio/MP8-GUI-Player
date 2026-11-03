# MP8! Hardcore Player GUI

A lightweight, pure Win32 C-based GUI frontend for the **MP8! Hardcore Audio Container**.

## Architecture: Process Isolation
This repository contains the **GUI source code** and the compiled **closed-source core binary** (`mp8.exe`). 

To keep the encryption algorithms isolated from the open-source frontend, the GUI does not link any decryption libraries. Instead, it invokes the core engine via subprocess creation (`CreateProcess`):
- **Open-Source Frontend**: You can read, study, and compile the Win32 GUI (`player_gui.c`).
- **Closed-Source Core**: The AES-256 encryption, KDF, and audio decoding are strictly handled by the proprietary `mp8.exe` binary.

## How to Build
You only need GCC (MinGW) and the Windows SDK. Double-click `build-gui.bat` or run:
```bat
gcc player_gui.c -o mp8_gui.exe -O2 -mwindows -lcomctl32 -lole32 -lshell32 -static -s
```

## Usage
1. Ensure `mp8.exe` (the core engine) is in the same directory as `mp8_gui.exe`.
2. Launch `mp8_gui.exe`.
3. Click **Select Folder** to scan a directory containing `.mp8` encrypted files.
4. Enter the password and click **Play**.
*(Note: If the password is incorrect, playback will not start. Please verify your password.)*

## ⚠️ License
This project is licensed under the **SimpleTools Studio HalfOpenSource License (SHL) v1.0**. 
You may view and study the open-source GUI source code (`player_gui.c`). However, **commercial use, distribution, and reverse engineering of the closed-source core binary (`mp8.exe`) are strictly prohibited**. 
See the [LICENSE](./LICENSE) file for full terms.
```