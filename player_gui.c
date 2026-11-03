/*
 * MP8! Hardcore Player GUI
 * 
 * Copyright (c) 2026 SimpleTools Studio. All rights reserved.
 * 
 * Licensed under the SimpleTools Studio HalfOpenSource License (SHL) v1.0.
 * You may read and study the source code, but commercial use, distribution, 
 * and reverse engineering are strictly prohibited.
 * 
 * See the LICENSE file in the project root for full terms.
 */

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

#define ID_LISTBOX 101
#define ID_BTN_PLAY 102
#define ID_BTN_NEXT 103
#define ID_EDIT_PASS 104
#define ID_BTN_OPEN 105

HWND g_hListBox;
HWND g_hBtnPlay;
HWND g_hEditPass;
char g_playlist[MAX_PATH * 20][MAX_PATH];
int g_song_count = 0;
char g_current_dir[MAX_PATH] = ".";

// Global handle for the current subprocess to allow forceful termination
HANDLE g_hCurrentProcess = NULL;

typedef struct {
    volatile int is_playing;
    HANDLE thread_handle;
    volatile int thread_running;
} PlayerContext;
PlayerContext g_player = {0, NULL, 0};

DWORD WINAPI PlayThreadProc(LPVOID lpParam) {
    int index = (int)(intptr_t)lpParam;
    if (index < 0 || index >= g_song_count) return 0;

    char pass[128] = {0};
    GetWindowTextA(g_hEditPass, pass, sizeof(pass));

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "\"mp8.exe\" play \"%s\" -k \"%s\"", g_playlist[index], pass);

    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE; 

    if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        // Save the subprocess handle
        g_hCurrentProcess = pi.hProcess;

        g_player.is_playing = 1;
        SetWindowTextA(g_hBtnPlay, "Pause");

        WaitForSingleObject(pi.hProcess, INFINITE);

        // Clean up handles after playback finishes or is terminated
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        g_hCurrentProcess = NULL; 
    } else {
        g_player.is_playing = 0;
        SetWindowTextA(g_hBtnPlay, "Play");
        return 0;
    }

    g_player.is_playing = 0;
    SetWindowTextA(g_hBtnPlay, "Play");

    if (g_player.thread_running && index + 1 < g_song_count) {
        PostMessage(g_hBtnPlay, WM_COMMAND, ID_BTN_NEXT, 0);
    }
    
    return 0;
}

int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) {
    if (uMsg == BFFM_INITIALIZED) {
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
    }
    return 0;
}

void LoadPlaylistFromDir(const char* dir) {
    SendMessageA(g_hListBox, LB_RESETCONTENT, 0, 0);
    g_song_count = 0;

    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*.mp8", dir);

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(search_path, &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            snprintf(g_playlist[g_song_count], MAX_PATH, "%s\\%s", dir, findData.cFileName);
            SendMessageA(g_hListBox, LB_ADDSTRING, 0, (LPARAM)findData.cFileName);
            g_song_count++;
        }
    } while (FindNextFileA(hFind, &findData) && g_song_count < MAX_PATH * 20);
    FindClose(hFind);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateWindowA("BUTTON", "Select Folder", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 10, 120, 30, hwnd, (HMENU)ID_BTN_OPEN, GetModuleHandle(NULL), NULL);

        g_hListBox = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
            10, 50, 360, 220, hwnd, (HMENU)ID_LISTBOX, GetModuleHandle(NULL), NULL);

        CreateWindowA("STATIC", "Password:", WS_CHILD | WS_VISIBLE, 10, 280, 60, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
        g_hEditPass = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "2026",
            WS_CHILD | WS_VISIBLE | ES_PASSWORD | ES_AUTOHSCROLL,
            75, 280, 130, 24, hwnd, (HMENU)ID_EDIT_PASS, GetModuleHandle(NULL), NULL);

        g_hBtnPlay = CreateWindowA("BUTTON", "Play",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 320, 100, 30, hwnd, (HMENU)ID_BTN_PLAY, GetModuleHandle(NULL), NULL);

        CreateWindowA("BUTTON", "Next",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            120, 320, 100, 30, hwnd, (HMENU)ID_BTN_NEXT, GetModuleHandle(NULL), NULL);

        LoadPlaylistFromDir(".");
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BTN_OPEN) {
            BROWSEINFOA bi = {0};
            bi.hwndOwner = hwnd;
            bi.lpszTitle = "Select a folder containing .mp8 files:";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            bi.lpfn = BrowseCallbackProc;
            bi.lParam = (LPARAM)g_current_dir;

            LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
            if (pidl != NULL) {
                if (SHGetPathFromIDListA(pidl, g_current_dir)) {
                    LoadPlaylistFromDir(g_current_dir);
                }
                CoTaskMemFree(pidl);
            }
        }
        else if (LOWORD(wParam) == ID_BTN_PLAY) {
            if (!g_player.is_playing) {
                if (g_song_count == 0) {
                    MessageBoxA(hwnd, "No .mp8 files found in the selected folder.", "Info", MB_OK | MB_ICONINFORMATION);
                    break;
                }
                int idx = (int)SendMessageA(g_hListBox, LB_GETCURSEL, 0, 0);
                if (idx == LB_ERR) idx = 0;

                g_player.thread_running = 1;
                g_player.thread_handle = CreateThread(NULL, 0, PlayThreadProc, (LPVOID)(intptr_t)idx, 0, NULL);
            } else {
                // On Pause: clear the running flag and forcefully kill the subprocess
                g_player.thread_running = 0;
                if (g_hCurrentProcess != NULL) {
                    TerminateProcess(g_hCurrentProcess, 0); // Force kill mp8.exe
                    g_hCurrentProcess = NULL;
                }
            }
        }
        else if (LOWORD(wParam) == ID_BTN_NEXT) {
            if (g_song_count == 0) break;
            g_player.thread_running = 0;
            
            // Force kill the currently playing process before switching tracks
            if (g_hCurrentProcess != NULL) {
                TerminateProcess(g_hCurrentProcess, 0);
                g_hCurrentProcess = NULL;
            }

            if (g_player.thread_handle) {
                WaitForSingleObject(g_player.thread_handle, 2000);
                CloseHandle(g_player.thread_handle);
            }

            int idx = (int)SendMessageA(g_hListBox, LB_GETCURSEL, 0, 0);
            if (idx == LB_ERR || idx + 1 >= g_song_count) idx = 0;
            else idx++;

            SendMessageA(g_hListBox, LB_SETCURSEL, idx, 0);
            g_player.thread_running = 1;
            g_player.thread_handle = CreateThread(NULL, 0, PlayThreadProc, (LPVOID)(intptr_t)idx, 0, NULL);
        }
        break;

    case WM_DESTROY:
        g_player.thread_running = 0;
        if (g_hCurrentProcess != NULL) {
            TerminateProcess(g_hCurrentProcess, 0);
            g_hCurrentProcess = NULL;
        }
        if (g_player.thread_handle) {
            WaitForSingleObject(g_player.thread_handle, 2000);
            CloseHandle(g_player.thread_handle);
        }
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "MP8HardcoreGUI";
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowA("MP8HardcoreGUI", "MP8! Hardcore Player", 
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 410, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    CoUninitialize();
    return 0;
}