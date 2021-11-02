#ifndef CUBECRAFT3_COMMON_MESSAGE_HPP
#define CUBECRAFT3_COMMON_MESSAGE_HPP

#include <algorithm>
#include <cinttypes>
#include <string>
#include <string_view>
#include <vector>

enum class ClientMessages : uint8_t { kConnect, kDisconnect, kPing, kPosition, kRequestChunk, kChat, kCommand };
enum class ServerMessages : uint8_t { kValidate = 128, kDisconnect, kLoadChunk, kUpdateBlock, kPosition, kCommand };

#endif
