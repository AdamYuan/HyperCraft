#pragma once

#include <block/Block.hpp>
#include <common/Position.hpp>

#include <span>
#include <vector>

namespace hc {

struct ChunkSetBlockEntry {
	InnerIndex3 index{};
	block::Block old_block, new_block;
};

struct ChunkSetSunlightEntry {
	InnerIndex2 index{};
	InnerPos1 old_sunlight{}, new_sunlight{};
};

struct ChunkBlockEntry {
	InnerIndex3 index{};
	block::Block block;
};

struct ChunkSunlightEntry {
	InnerIndex2 index{};
	InnerPos1 sunlight{};
};

struct ChunkEntry {
	std::vector<ChunkBlockEntry> blocks;
	std::vector<ChunkSunlightEntry> sunlights;
};

class PackedChunkBlockEntry {
private:
	InnerIndex3 m_index{};
	block::Block m_block;

public:
	inline PackedChunkBlockEntry(InnerIndex3 index, block::Block block) : m_index{index}, m_block{block} {}
	inline explicit PackedChunkBlockEntry(ChunkBlockEntry entry) : m_index{entry.index}, m_block{entry.block} {}
	inline explicit PackedChunkBlockEntry(uint32_t packed_data) : m_index(packed_data >> 16u) {
		m_block.SetData(packed_data & 0xffffu);
	}
	inline uint32_t GetPackedData() const { return (m_index << 16u) | m_block.GetData(); }
	inline ChunkBlockEntry Unpack() const { return {.index = GetIndex(), .block = GetBlock()}; }
	inline static std::vector<ChunkBlockEntry> Unpack(std::span<const PackedChunkBlockEntry> packed_entries) {
		std::vector<ChunkBlockEntry> ret(packed_entries.size());
		for (std::size_t i = 0; i < packed_entries.size(); ++i)
			ret[i] = packed_entries[i].Unpack();
		return ret;
	}
	inline InnerIndex3 GetIndex() const { return m_index; }
	inline block::Block GetBlock() const { return m_block; }
};
static_assert(sizeof(PackedChunkBlockEntry) == 4);

class PackedChunkSunlightEntry {
private:
	uint16_t m_data{};

public:
	inline PackedChunkSunlightEntry(InnerIndex2 index, InnerPos1 sunlight) : m_data((index << 6u) | sunlight) {}
	inline explicit PackedChunkSunlightEntry(ChunkSunlightEntry entry)
	    : PackedChunkSunlightEntry(entry.index, entry.sunlight) {}
	inline explicit PackedChunkSunlightEntry(uint16_t packed_data) : m_data{packed_data} {}
	inline uint16_t GetPackedData() const { return m_data; }
	inline ChunkSunlightEntry Unpack() const { return {.index = GetIndex(), .sunlight = GetSunlight()}; }
	inline static std::vector<ChunkSunlightEntry> Unpack(std::span<const PackedChunkSunlightEntry> packed_entries) {
		std::vector<ChunkSunlightEntry> ret(packed_entries.size());
		for (std::size_t i = 0; i < packed_entries.size(); ++i)
			ret[i] = packed_entries[i].Unpack();
		return ret;
	}
	inline InnerIndex2 GetIndex() const { return m_data >> 6u; }
	inline InnerPos1 GetSunlight() const { return m_data & 0x3fu; }
};
static_assert((kChunkSize + 1) <= (1 << 6) && kChunkSize * kChunkSize <= (1 << 10));
static_assert(sizeof(PackedChunkSunlightEntry) == 2);

struct PackedChunkEntry {
	std::vector<PackedChunkBlockEntry> blocks;
	std::vector<PackedChunkSunlightEntry> sunlights;
};

} // namespace hc
