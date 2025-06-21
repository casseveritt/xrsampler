#include <android/asset_manager.h>
#include <android/log.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

bool LoadGltfModelFromAsset(AAssetManager* assetManager, const std::string& assetPath, tinygltf::Model* model, bool isBinary) {
  // Set the global asset_manager pointer used by TinyGLTF
  tinygltf::asset_manager = assetManager;

  tinygltf::TinyGLTF loader;
  std::string err, warn;
  bool ret = false;

  if (isBinary) {
    ret = loader.LoadBinaryFromFile(model, &err, &warn, assetPath);
  } else {
    ret = loader.LoadASCIIFromFile(model, &err, &warn, assetPath);
  }

  if (!warn.empty()) {
    __android_log_print(ANDROID_LOG_WARN, "TinyGLTF", "Warn: %s", warn.c_str());
  }
  if (!err.empty()) {
    __android_log_print(ANDROID_LOG_ERROR, "TinyGLTF", "Err: %s", err.c_str());
  }

  return ret;
}