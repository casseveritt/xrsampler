add_library(xrh STATIC
    src/xrh.cpp
    include/linear.h
    include/xrhlinear.h
    include/xrh.h
)

target_include_directories(xrh PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Search for package provided by the OpenXR dependency
find_package(OpenXR REQUIRED CONFIG)

# definitions for Android NDK
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DANDROID)
add_definitions(-DXR_USE_GRAPHICS_API_OPENGL_ES=1)
add_definitions(-DXR_USE_PLATFORM_ANDROID=1)


target_link_libraries(xrh 

        # OpenXR loader support
        OpenXR::openxr_loader

        EGL
        GLESv3
)
