cmake_minimum_required(VERSION 3.10)
project(sit C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CURSES_NEED_WIDE)
find_package(Curses REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

add_executable(sit
  src/main.c
  src/audio.c
  src/ui.c
)

target_include_directories(sit PRIVATE
    src
    ${CURSES_INCLUDE_DIRS}
    ${PORTAUDIO_INCLUDE_DIRS}
)

target_link_libraries(sit PRIVATE
    ${CURSES_LIBRARIES}
    ${PORTAUDIO_LIBRARIES}
    m
)
