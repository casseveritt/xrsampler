# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

project("dreadful")

add_subdirectory(
        ${CMAKE_CURRENT_SOURCE_DIR}/app/src/main/cpp)

