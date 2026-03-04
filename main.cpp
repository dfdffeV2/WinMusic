#include "home.h"

int main() {
    // prepare the console for UTF-8 output so non-ASCII filenames render
    setlocale(LC_ALL, "Russian_Russia.65001"); // RU + UTF‑8 locale
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    showHomeScreen();

    return 0;
}