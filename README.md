# xrsampler

OpenXR sample applications for Android using OpenGL ES rendering.

## Overview

xrsampler is a collection of demonstration applications showcasing how to build XR (Extended Reality / VR) applications for Android devices using the OpenXR API standard. The project provides both a helper library for working with OpenXR and multiple sample applications that demonstrate different aspects of XR development.

## Key Components

### xrh Library (OpenXR Helper)

The `xrh` library is a C++20 abstraction layer that simplifies working with the OpenXR API. It provides:

- **Instance Management**: Create OpenXR instances with required and optional extensions
- **Session Management**: Establish and manage XR sessions with graphics binding
- **Space Management**: Handle spatial reference frames (Local, Stage, View)
- **Swapchain Management**: Manage image acquisition, rendering, and submission
- **Layer Composition**: Support for Projection, Quad, Cylinder, Equirect, and Cube layers
- **Graphics Integration**: OpenGL ES context integration via EGL for Android
- **Frame Loop**: Simplified begin/end frame pattern for rendering
- **Math Support**: Vector, quaternion, pose, and matrix operations via `xrhlinear.h`

Key classes:
- `InstanceOb`: OpenXR instance creation and system detection
- `SessionOb`: Session management, frame timing, and layer submission
- `SpaceOb`/`RefSpaceOb`: Spatial reference frame handling
- `SwapchainOb`: Image swapchain management
- `Layer`, `QuadLayer`, `ProjectionLayer`: Composition layer support

### Sample Applications

#### Awful Sample

An advanced demonstration that showcases realistic asset rendering in XR:

- **glTF Model Loading**: Uses tinygltf to load and parse glTF 2.0 models
- **GltfRenderer**: Comprehensive renderer with support for:
  - Vertex attributes (position, normal, texcoord)
  - Texture mapping
  - PBR materials
  - Model skinning
- **Example Asset**: Renders a cartoony rubber ducky model
- **Quad Layers**: Demonstrates positioning rendered content as quad layers in 3D space

This sample is ideal for understanding how to integrate complex 3D assets into an XR application.

#### Dreadful Sample

A simpler demonstration focused on core XR rendering fundamentals:

- **Basic Geometry**: Procedural geometry rendering without external model loading
- **Simplified Renderer**: Focuses on the essential OpenXR session and layer management
- **Core Loop**: Demonstrates the fundamental frame-by-frame XR rendering pattern

This sample serves as a good starting point for learning XR development basics.

## Technologies & Dependencies

### Core Technologies

- **OpenXR**: Industry-standard API for VR/AR applications
- **OpenGL ES 3**: Mobile-optimized graphics rendering
- **Android NDK/SDK**: Native development for Android
- **EGL**: OpenGL ES context management
- **CMake**: Build system with Android Studio integration

### Third-Party Libraries

- **tinygltf**: Header-only glTF 2.0 model loading library
- **GLM**: Math library for 3D graphics (matrices, vectors, quaternions)
- **Game Activity**: Modern Android native app framework
- **STB**: Image loading utilities (via tinygltf)

## Architecture

The typical application flow follows this pattern:

```
Android App Lifecycle
        ↓
App Initialization (APP_CMD_INIT_WINDOW)
        ↓
xr::App Constructor
        ├─→ Renderer (EGL + OpenGL ES setup)
        ├─→ xrh::InstanceOb (OpenXR instance)
        ├─→ xrh::SessionOb (OpenXR session)
        ├─→ xrh::RefSpaceOb (Reference space)
        ├─→ xrh::SwapchainOb (Image swapchain)
        └─→ Content Renderer (GltfRenderer for Awful)
        
Frame Loop
        ↓
xr::App::frame()
        ├─→ begin_frame() - Start frame timing
        ├─→ acquire_and_wait_image() - Get swapchain image
        ├─→ Render content to swapchain
        ├─→ Create composition layers
        └─→ end_frame() - Submit layers for display
```

## Building

See [BUILDING.md](BUILDING.md) for detailed build instructions.

### Quick Start

1. Install Android Studio
2. Set environment variables:
   ```bash
   export ANDROID_HOME=~/Android/Sdk
   export PATH=~/android-studio/jbr/bin:$PATH
   ```
3. Build the samples:
   ```bash
   cd src/samples/Awful  # or Dreadful
   ./gradlew clean
   ./gradlew assembleDebug
   ```

## Target Platforms

- **Android**: Devices with OpenXR runtime support (Meta Quest, HTC Vive Focus, etc.)
- **Graphics API**: OpenGL ES 3.0+
- **Language**: C++20

## Project Structure

```
xrsampler/
├── include/openxr/          # OpenXR headers
├── src/
│   ├── xrh/                 # OpenXR helper library
│   │   ├── include/         # Public headers
│   │   └── src/             # Implementation
│   ├── samples/
│   │   ├── Awful/           # Advanced glTF rendering sample
│   │   └── Dreadful/        # Basic rendering sample
│   └── tinygltf/            # Third-party glTF library
├── BUILDING.md              # Build instructions
├── LICENSE.md               # License information
└── README.md                # This file
```

## License

See [LICENSE.md](LICENSE.md) for license information.
