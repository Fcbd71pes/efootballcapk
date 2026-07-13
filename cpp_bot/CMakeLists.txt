cmake_minimum_required(VERSION 3.14)
project(efootball_bot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

# Find packages
find_package(TgBot CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/config.cpp
    src/lang.cpp
    src/db.cpp
    src/user_cmds.cpp
    src/admin_cmds.cpp
    src/handlers.cpp
    src/utils.cpp
)

if(ANDROID)
    # Add JNI wrapper for Android
    list(APPEND SOURCES ../android_app/app/src/main/cpp/native-lib.cpp)
    # Build as a shared library
    add_library(efootballbot SHARED ${SOURCES})
    target_link_libraries(efootballbot PRIVATE log TgBot::TgBot unofficial::sqlite3::sqlite3)
    target_include_directories(efootballbot PRIVATE src)
else()
    # Build as an executable for desktop
    add_executable(efootball_bot ${SOURCES})
    target_link_libraries(efootball_bot PRIVATE TgBot::TgBot unofficial::sqlite3::sqlite3)
    target_include_directories(efootball_bot PRIVATE src)
endif()
