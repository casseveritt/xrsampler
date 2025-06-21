#pragma once

#include <android/asset_manager.h>

#include <string>

#include "tiny_gltf.h"

bool LoadGltfModelFromAsset(AAssetManager* assetManager, const std::string& assetPath, tinygltf::Model* model,
                            bool isBinary = false);