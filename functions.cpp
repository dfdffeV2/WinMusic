#include "functions.h"
#include <fstream>
#include <limits>


// config file used to remember last-selected library
static const char* kConfigFileName = "library.cfg";

// generator used by shuffle operations
std::mt19937 rng{ std::random_device{}() };


// --- Config Helpers ---

std::string getDefaultMusicFolder() {
    // typical location under user profile
    const char* up = std::getenv("USERPROFILE");
    if (!up) return {};
    std::string p = std::string(up) + "\\Music";
    if (fs::exists(p) && fs::is_directory(p))
        return p;
    return {};
}

std::string loadLibraryConfig() {
    std::ifstream in(kConfigFileName);
    if (!in)
        return {};
    std::string path;
    std::getline(in, path);
    return path;
}

void saveLibraryConfig(const std::string& path) {
    std::ofstream out(kConfigFileName);
    if (out)
        out << path;
}

// ask user to pick one of the subfolders under libraryPath (or "0" for root)
std::string choosePlaylist(const std::string& libraryPath) {
    std::vector<fs::path> subs;
    for (auto const& e : fs::directory_iterator(libraryPath)) {
        if (e.is_directory())
            subs.push_back(e.path());
    }
    if (subs.empty())
        return libraryPath;

    std::cout << "Available playlists:\n";
    std::cout << "  0: <entire library>\n";
    for (size_t i = 0; i < subs.size(); ++i)
        std::cout << "  " << i+1 << ": " << subs[i].filename().u8string() << "\n";
    std::cout << "Enter number: ";
    int choice = -1;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return libraryPath;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (choice <= 0 || choice > (int)subs.size())
        return libraryPath;
    return subs[choice-1].string();
}


// --- Functions ---
bool applyControls(sf::Music& m,
                   const std::vector<fs::path>& tracks,
                   std::vector<fs::path>& playOrder,
                   int& current,
                   PlayMode& mode)
{
    // --- Media Keys ---
    if (GetAsyncKeyState(VK_MEDIA_NEXT_TRACK) & 1) {
        current = (current + 1) % playOrder.size();
        m.stop();
        return true;
    } else if (GetAsyncKeyState(VK_MEDIA_PREV_TRACK) & 1) {
        current = (current + playOrder.size() - 1) % playOrder.size();
        m.stop();
        return true;
    } else if (GetAsyncKeyState(VK_MEDIA_PLAY_PAUSE) & 1) {
        if (m.getStatus() == sf::Music::Playing) m.pause();
        else if (m.getStatus() == sf::Music::Paused) m.play();
        Sleep(debounceMs);
        return false;
    }

    // --- Playmode Keys ---
    if (GetAsyncKeyState(VK_F1) & 1) {
        mode = PlayMode::Sequential;
        m.setLoop(false);
        fs::path now = playOrder.empty() ? fs::path() : playOrder[current];
        playOrder = tracks;
        if (!now.empty()) {
            auto it = std::find(playOrder.begin(), playOrder.end(), now);
            if (it != playOrder.end())
                current = static_cast<int>(std::distance(playOrder.begin(), it));
            else
                current = 0;
        }
        std::cout << "Playmode: Sequential\n";
        printPlaylist(playOrder, "(sequential)", current);
        Sleep(debounceMs);
    }
    else if (GetAsyncKeyState(VK_F2) & 1) {
        mode = PlayMode::Repeat;
        m.setLoop(true);
        std::cout << "Playmode: Repeat track\n";
        Sleep(debounceMs);
    }
    else if (GetAsyncKeyState(VK_F3) & 1) {
        mode = PlayMode::Shuffle;
        playOrder = tracks;
        std::shuffle(playOrder.begin(), playOrder.end(), rng);
        current = 0;
        m.stop();
        std::cout << "Playmode: Shuffle\n";
        printPlaylist(playOrder, "(shuffled)", current);
        Sleep(debounceMs);
        return true;
    }

    return false;
}

bool readTracks(std::vector<fs::path>& out, const fs::path& dir) {
    if (!fs::is_directory(dir)) return false;
    for (auto const& e : fs::recursive_directory_iterator(dir)) {
        if (!e.is_regular_file()) continue;
        auto ext = e.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".mp3" || ext == ".ogg" || ext == ".wav")
            out.push_back(e.path());
    }

    // sort alphabetically by filename so playback order always matches listing
    std::sort(out.begin(), out.end(), [](const fs::path &a, const fs::path &b) {
        return a.filename().u8string() < b.filename().u8string();
    });

    return true;
}

bool printPlaylist(const std::vector<fs::path>& tracks, const std::string& playlist, int currentIndex) {
    if (tracks.empty()) {
        std::cerr << "No tracks found\n";
        return false;
    }

    std::cout << "Playlist: " << playlist << "\n";
    for (int i = 0; i < (int)tracks.size(); ++i) {
        if (i == currentIndex)
            std::cout << "-> ";
        else
            std::cout << "   ";
        std::cout << i + 1 << ": " << tracks[i].filename().u8string() << "\n";
    }
    return true;
}

bool loadTrack(sf::Music& m, const fs::path& path) {
    if (!m.openFromFile(path.u8string())) {
        std::cerr << "Failed to load: " << path.u8string() << '\n';
        return false;
    }
    std::cout << "Playing: " << path.filename().u8string() << "\n";
    return true;
}
