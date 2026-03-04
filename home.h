#pragma once

#include "functions.h"
#include <shobjidl.h>
#include <string>
#include <limits>

// global state -------------------------------------------------------------
// library path is remembered across runs; if none saved try the standard Music folder
static std::string g_musicLibrary = []() {
    std::string p = loadLibraryConfig();
    if (p.empty())
        p = getDefaultMusicFolder();
    return p;
}();


// --- Declarations ---
void showHomeScreen();
void playMusic();
std::string selectMusicLibrary(const std::wstring& title);


// --- Functions ---
void playMusic() {
    sf::Music music;
    std::vector<fs::path> tracks, playOrder;

    // make sure library exists; otherwise ask user again
    if (g_musicLibrary.empty() || !fs::is_directory(g_musicLibrary)) {
        if (!g_musicLibrary.empty())
            std::cout << "Previously saved library not found: " << g_musicLibrary << "\n";
        std::cout << "Please select a music library folder.\n";
        std::string sel = selectMusicLibrary(L"Select Music Library");
        if (sel.empty())
            return; // cancelled or error
        g_musicLibrary = sel;
        saveLibraryConfig(g_musicLibrary);
    }

    std::string lib = g_musicLibrary;

    // if library contains subfolders, let user pick a playlist
    if (fs::is_directory(lib)) {
        lib = choosePlaylist(lib);
    }

    // --- Read tracks ---
    if (!readTracks(tracks, lib)) {
        std::cerr << "Failed to read tracks from " << lib << "\n";
        return;
    }
    if (tracks.empty()) {
        std::cerr << "No audio files found in " << lib << "\n";
        return;
    }
    playOrder = tracks; // start in original order
    printPlaylist(playOrder, lib, 0);

    PlayMode mode = PlayMode::Sequential;
    int current = 0;
    bool done = false; // set true to exit main loop and return to home screen

    while (!done && !playOrder.empty()) {
        // show playlist with current track highlighted
        printPlaylist(playOrder, lib, current);

        // --- Load track ---
        if (!loadTrack(music, playOrder[current])) break;
        music.play();

        bool manualChange = false; // set true if user skipped or changed playmode -> skip
        while (music.getStatus() != sf::Music::Stopped) {
            if (applyControls(music, tracks, playOrder, current, mode)) {
                // either user skipped or shuffle reset happened
                manualChange = true;
                // reprint playlist with current highlight
                printPlaylist(playOrder, lib, current);
            }
            if (GetAsyncKeyState(VK_ESCAPE) & 1) {
                // exit immediately
                music.stop();
                done = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        if (!manualChange) {
            // normal end
            if (mode == PlayMode::Repeat) {
                // do nothing, current stays same
            } else {
                current++;
                if (current >= (int)playOrder.size()) {
                    if (mode == PlayMode::Sequential) {
                        // reached end of playlist: exit loop (return to home later)
                        done = true;
                    } else if (mode == PlayMode::Shuffle) {
                        // re‑shuffle and continue
                        playOrder = tracks;
                        std::shuffle(playOrder.begin(), playOrder.end(), rng);
                        current = 0;
                    }
                }
            }
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 1) {
            // user wants to exit to home screen
            done = true;
        }
    }
}

std::string selectMusicLibrary(const std::wstring& title) {
    std::string result;

    // Инициализация COM
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        std::cerr << "Ошибка инициализации COM\n";
        return result;
    }

    IFileDialog* pFileDialog = nullptr;

    // Создаём диалог
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&pFileDialog));
    if (SUCCEEDED(hr)) {
        // Устанавливаем флаг выбора папок
        DWORD dwOptions;
        if (SUCCEEDED(pFileDialog->GetOptions(&dwOptions))) {
            pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        }

        // Заголовок окна
        pFileDialog->SetTitle(title.c_str());

        // Показываем диалог
        hr = pFileDialog->Show(NULL);
        if (SUCCEEDED(hr)) {
            IShellItem* pItem = nullptr;
            if (SUCCEEDED(pFileDialog->GetResult(&pItem))) {
                PWSTR pszFilePath = nullptr;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                    // Конвертируем в std::string (UTF-8)
                    char buffer[MAX_PATH];
                    WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, buffer, MAX_PATH, NULL, NULL);
                    result = buffer;
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileDialog->Release();
    }

    CoUninitialize();
    return result;
}

void showHomeScreen() {
    char choice = 0;
    while (choice != '3') {
        std::cout << "=== Welcome to WinMusic ===\n\n";
        std::cout << "Current library: " << (g_musicLibrary.empty() ? "<none>" : g_musicLibrary) << "\n\n";
        std::cout << "(1) Play Music\n";
        std::cout << "(2) Select Music Library\n";
        std::cout << "(3) Exit\n";

        do {
            choice = std::cin.get();
        } while (choice != '1' && choice != '2' && choice != '3');

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case '1':
                playMusic();
                break;
            case '2': {
                std::string sel = selectMusicLibrary(L"Select Music Library");
                if (!sel.empty()) {
                    g_musicLibrary = sel;
                    saveLibraryConfig(g_musicLibrary);
                    std::cout << "Library set to: " << g_musicLibrary << "\n";
                }
                break;
            }
            case '3':
                std::cout << "Goodbye!\n";
                break;
        }
        std::cout << "\n";
    }
}