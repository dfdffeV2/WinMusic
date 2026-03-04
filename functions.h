#pragma once

// --- Directives ---
#include <SFML/Audio.hpp>
#include <iostream>
#include <filesystem>
#include <windows.h>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>
#include <random>

namespace fs = std::filesystem;

// debounce time for keys to avoid double‑toggling when held down
const int debounceMs = 200;


// --- PlayMode ---
enum class PlayMode {
    Sequential,
    Repeat,   // loop current track
    Shuffle
};


// one generator shared across calls so shuffle order changes only when requested
extern std::mt19937 rng;


// --- Config Helpers ---
std::string getDefaultMusicFolder();              // returns standard Music folder path or empty
std::string loadLibraryConfig();
void saveLibraryConfig(const std::string& path);


// --- Declarations ---
// currentIndex if >=0 will be highlighted when printing
bool printPlaylist(const std::vector<fs::path>& tracks, const std::string& playlist, int currentIndex = -1);

bool applyControls(sf::Music& m,
                   const std::vector<fs::path>& tracks,
                   std::vector<fs::path>& playOrder,
                   int& current,
                   PlayMode& mode);

// helper to let user pick a subfolder of library (playlist)
std::string choosePlaylist(const std::string& libraryPath);

bool readTracks(std::vector<fs::path>& out, const fs::path& dir);
bool loadTrack(sf::Music& m, const fs::path& path);