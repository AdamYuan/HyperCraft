#pragma once

#include <block/Block.hpp>
#include <common/Position.hpp>

namespace hc {

struct ChunkBlockEntry {
	InnerIndex3 index{};
	block::Block block;
};

struct ChunkSunlightEntry {
	InnerIndex2 index;
	InnerPos1 sunlight;
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
