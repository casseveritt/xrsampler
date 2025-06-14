# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

project("dreadful")

# Creates your game shared library. The name must be the same as the
# one used for loading in your Kotlin/Java or AndroidManifest.txt files.
add_library(dreadful SHARED
        main.cpp
        AndroidOut.cpp
        Renderer.cpp
        Shader.cpp
        TextureAsset.cpp
        xrapp.cpp
        xrh.cpp)

# Searches for a package provided by the game activity dependency
find_package(game-activity REQUIRED CONFIG)

# Search for package provided by the OpenXR dependency
find_package(OpenXR REQUIRED CONFIG)

# definitions for Android NDK
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DANDROID)
add_definitions(-DXR_USE_GRAPHICS_API_OPENGL_ES=1)
add_definitions(-DXR_USE_PLATFORM_ANDROID=1)

# Configure libraries CMake uses to link your target library.
target_link_libraries(dreadful

        # OpenXR loader support
        OpenXR::openxr_loader

        # The game activity
        game-activity::game-activity

        # EGL and other dependent libraries required for drawing
        # and interacting with Android system
        EGL
        GLESv3
        jnigraphics
        android
        log)