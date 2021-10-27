#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <cinttypes>

enum class ClientMessages : uint8_t {
	kConnect,
	kDisconnect,
	kPing,
	kPosition,
	kRequestChunk,
	kChat,
	kCommand
};

enum class ServerMessages : uint8_t {
	kValidate,
	kDisconnect,
	kLoadChunk,
	kUpdateBlock,
	kPosition,
	kCommand
};

class Message {};

#endif
