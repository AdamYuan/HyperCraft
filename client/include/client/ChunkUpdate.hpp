#pragma once

#include <block/Block.hpp>
#include <cinttypes>
#include <common/Position.hpp>
#include <optional>

namespace hc::client {

enum class ChunkUpdateType { kLocal, kRemote };

template <typename Data> struct ChunkUpdate {
	std::optional<Data> local, remote;
	template <ChunkUpdateType Type> inline std::optional<Data> &Get() {
		return Type == ChunkUpdateType::kLocal ? local : remote;
	}
	template <ChunkUpdateType Type> inline const std::optional<Data> &Get() const {
		return Type == ChunkUpdateType::kLocal ? local : remote;
	}
	inline std::optional<Data> &Get(ChunkUpdateType type) { return type == ChunkUpdateType::kLocal ? local : remote; }
	inline const std::optional<Data> &Get(ChunkUpdateType type) const {
		return type == ChunkUpdateType::kLocal ? local : remote;
	}
	inline Data GetNew() const { return local.has_value() ? local.value() : remote.value(); }
	inline const std::optional<Data> &GetOld() const { return remote; }
};

} // namespace hc::client
