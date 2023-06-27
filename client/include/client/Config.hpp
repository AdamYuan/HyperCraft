#ifndef HYPERCRAFT_CLIENT_CONFIG_HPP
#define HYPERCRAFT_CLIENT_CONFIG_HPP

#include <cinttypes>

namespace hc::client {

constexpr const char *kAppName = "HyperCraft";
constexpr uint32_t kDefaultWidth = 1280, kDefaultHeight = 720;
constexpr uint32_t kFrameCount = 3;

constexpr float kCameraNear = 0.01f, kCameraFar = 640.0f;

constexpr uint32_t kWorldMaxLoadRadius = 20;

} // namespace hc::client

#endif
