cmake_minimum_required(VERSION 3.16)
project(WinMusic)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML COMPONENTS audio REQUIRED)

add_executable(WinMusic
    main.cpp
    functions.cpp
)

target_link_libraries(WinMusic
    sfml-audio
)
