#ifndef HYPERCRAFT_CLIENT_CHUNK_HPP
#define HYPERCRAFT_CLIENT_CHUNK_HPP

#include <block/Block.hpp>
#include <block/Light.hpp>
#include <common/Position.hpp>
#include <common/Size.hpp>
#include <glm/glm.hpp>

#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <span>
#include <vector>

namespace hc::client {

class World;

enum class ChunkLockType : uint8_t {
	kBlockR = 0x01,
	kSunlightR = 0x10,
	kBlockRW = 0x02,
	kSunlightRW = 0x20,
	kBlockRSunlightRW = 0x21,
	kBlockRWSunlightR = 0x12,
	kAllR = 0x11,
	kAllRW = 0x22
};

class Chunk : public std::enable_shared_from_this<Chunk> {
private:
	using Block = block::Block;
	using Light = block::Light;

public:
	static constexpr uint32_t kSize = kChunkSize;

	inline const ChunkPos3 &GetPosition() const { return m_position; }

	// Creation
	inline explicit Chunk(const ChunkPos3 &position) : m_position{position} {}
	static inline std::shared_ptr<Chunk> Create(const ChunkPos3 &position) { return std::make_shared<Chunk>(position); }

	// Generated Flag
	inline void SetGeneratedFlag() { m_generated_flag.store(true, std::memory_order_release); }
	inline bool IsGenerated() const { return m_generated_flag.load(std::memory_order_acquire); }

private:
	const ChunkPos3 m_position{};

	Block m_blocks[kSize * kSize * kSize];
	InnerPos1 m_sunlight_heights[kSize * kSize]{};
	std::atomic_bool m_generated_flag{false};
	std::shared_mutex m_block_mutex, m_sunlight_mutex;

	template <ChunkLockType> friend class LockedChunk;

	template <ChunkLockType... ArgTypes> inline static auto fetch_locks() {
		static_assert(sizeof...(ArgTypes) == 0);
		return std::tuple<>{};
	}
	template <ChunkLockType Type, ChunkLockType... ArgTypes>
	inline static auto fetch_locks(auto &&chunks, auto &&...args);

	template <ChunkLockType... ArgTypes> inline static auto fetch_locked_chunks() {
		static_assert(sizeof...(ArgTypes) == 0);
		return std::tuple<>{};
	}
	template <ChunkLockType Type, ChunkLockType... ArgTypes>
	inline static auto fetch_locked_chunks(auto &&chunks, auto &&...args);

public:
	// chunks should ensure x-z-y order
	template <ChunkLockType... Types> inline static auto Lock(auto &&...chunks);
};

template <ChunkLockType Type> class LockedChunk {
public:
	struct EmptyLock {
		template <typename... Args> inline explicit EmptyLock(Args &&...) {}
		// Lockable
		inline static bool try_lock() { return true; }
		inline static void lock() {}
		inline static void unlock() {}
		inline static void release() {}
	};

private:
	inline static constexpr uint8_t kBlockBits = (static_cast<uint8_t>(Type) & 0xfu),
	                                kSunlightBits = (static_cast<uint8_t>(Type) & 0xf0u) >> 4u;
	inline static constexpr bool kCanWriteBlock = kBlockBits == 0x2u;
	inline static constexpr bool kCanReadBlock = kBlockBits;
	inline static constexpr bool kCanWriteSunlight = kSunlightBits == 0x2u;
	inline static constexpr bool kCanReadSunlight = kSunlightBits;

public:
	using BlockLock =
	    std::conditional_t<kCanWriteBlock, std::unique_lock<std::shared_mutex>,
	                       std::conditional_t<kCanReadBlock, std::shared_lock<std::shared_mutex>, EmptyLock>>;
	using SunlightLock =
	    std::conditional_t<kCanWriteSunlight, std::unique_lock<std::shared_mutex>,
	                       std::conditional_t<kCanReadSunlight, std::shared_lock<std::shared_mutex>, EmptyLock>>;

private:
	std::shared_ptr<Chunk> m_chunk;

	BlockLock m_block_lock;
	SunlightLock m_sunlight_lock;

	inline LockedChunk(const std::shared_ptr<Chunk> &chunk, BlockLock &&block_lock, SunlightLock &&sunlight_lock)
	    : m_chunk{chunk}, m_block_lock{std::move(block_lock)}, m_sunlight_lock{std::move(sunlight_lock)} {}

	using Block = block::Block;

	friend class Chunk;
	template <ChunkLockType> friend class LockedChunk;

public:
	inline const auto &GetChunkPtr() const { return m_chunk; }
	inline const auto &GetPosition() const { return m_chunk->GetPosition(); }

	inline void Unlock() {
		m_block_lock.unlock();
		m_sunlight_lock.unlock();
	}

	template <ChunkLockType AsType> inline LockedChunk<AsType> As() const {
		static_assert(kBlockBits >= LockedChunk<AsType>::kBlockBits &&
		              kSunlightBits >= LockedChunk<AsType>::kSunlightBits);
		static_assert(Type != AsType);
		return LockedChunk<AsType>(m_chunk, (typename LockedChunk<AsType>::BlockLock){},
		                           (typename LockedChunk<AsType>::SunlightLock){});
	}

	// Block Read
	[[nodiscard]] inline const Block *GetBlockData() const {
		static_assert(kCanReadBlock);
		return m_chunk->m_blocks;
	}
	template <typename T> inline Block GetBlock(T x, T y, T z) const {
		static_assert(kCanReadBlock);
		return m_chunk->m_blocks[ChunkXYZ2Index(x, y, z)];
	}
	[[nodiscard]] inline Block GetBlock(uint32_t idx) const {
		static_assert(kCanReadBlock);
		return m_chunk->m_blocks[idx];
	}
	template <std::signed_integral T> inline Block GetBlockFromNeighbour(T x, T y, T z) const {
		static_assert(kCanReadBlock);
		return m_chunk->m_blocks[ChunkXYZ2Index((x + kChunkSize) % kChunkSize, (y + kChunkSize) % kChunkSize,
		                                        (z + kChunkSize) % kChunkSize)];
	}
	template <std::unsigned_integral T> inline Block GetBlockFromNeighbour(T x, T y, T z) const {
		static_assert(kCanReadBlock);
		return m_chunk->m_blocks[ChunkXYZ2Index(x % kChunkSize, y % kChunkSize, z % kChunkSize)];
	}

	// Block Write
	template <typename T> inline Block &GetBlockRef(T x, T y, T z) const {
		static_assert(kCanWriteBlock);
		return m_chunk->m_blocks[ChunkXYZ2Index(x, y, z)];
	}
	[[nodiscard]] inline Block &GetBlockRef(uint32_t idx) const {
		static_assert(kCanWriteBlock);
		return m_chunk->m_blocks[idx];
	}
	template <typename T> inline void SetBlock(T x, T y, T z, Block b) const {
		static_assert(kCanWriteBlock);
		m_chunk->m_blocks[ChunkXYZ2Index(x, y, z)] = b;
	}
	inline void SetBlock(uint32_t idx, Block b) const {
		static_assert(kCanWriteBlock);
		m_chunk->m_blocks[idx] = b;
	}

	// Sunlight Read
	[[nodiscard]] inline InnerPos1 GetSunlightHeight(uint32_t idx) const {
		static_assert(kCanReadSunlight);
		return m_chunk->m_sunlight_heights[idx];
	}
	template <typename T> inline InnerPos1 GetSunlightHeight(T x, T z) const {
		static_assert(kCanReadSunlight);
		return m_chunk->m_sunlight_heights[ChunkXZ2Index(x, z)];
	}
	template <std::integral T> inline bool GetSunlight(uint32_t idx, T y) const {
		static_assert(kCanReadSunlight);
		return y >= m_chunk->m_sunlight_heights[idx];
	}
	template <typename T> inline bool GetSunlight(T x, T y, T z) const {
		static_assert(kCanReadSunlight);
		return y >= m_chunk->m_sunlight_heights[ChunkXZ2Index(x, z)];
	}
	template <std::signed_integral T> inline bool GetSunlightFromNeighbour(T x, T y, T z) const {
		static_assert(kCanReadSunlight);
		return GetSunlight((x + kChunkSize) % kChunkSize, (y + kChunkSize) % kChunkSize, (z + kChunkSize) % kChunkSize);
	}
	template <std::unsigned_integral T> inline Block GetSunlightFromNeighbour(T x, T y, T z) const {
		static_assert(kCanReadSunlight);
		return GetSunlight(x % kChunkSize, y % kChunkSize, z % kChunkSize);
	}

	// Sunlight Write
	inline void SetSunlightHeight(uint32_t idx, InnerPos1 h) const {
		static_assert(kCanWriteSunlight);
		m_chunk->m_sunlight_heights[idx] = h;
	}
	template <typename T> inline void SetSunlightHeight(T x, T z, InnerPos1 h) const {
		static_assert(kCanWriteSunlight);
		m_chunk->m_sunlight_heights[ChunkXZ2Index(x, z)] = h;
	}
};

template <ChunkLockType Type, ChunkLockType... ArgTypes> auto Chunk::fetch_locks(auto &&chunks, auto &&...args) {
	using BlockLock = LockedChunk<Type>::BlockLock;
	using SunlightLock = LockedChunk<Type>::SunlightLock;

	if constexpr (std::is_convertible_v<std::decay_t<decltype(chunks)>, std::shared_ptr<Chunk>>) {
		return std::tuple_cat(
		    std::tuple<BlockLock, SunlightLock>(BlockLock{chunks->m_block_mutex, std::defer_lock},
		                                        SunlightLock{chunks->m_sunlight_mutex, std::defer_lock}),
		    fetch_locks<ArgTypes...>(std::forward<decltype(args)>(args)...));
	} else {
		constexpr std::size_t Extent = std::tuple_size_v<std::decay_t<decltype(chunks)>>;
		return [&]<std::size_t... I>(std::index_sequence<I...>) {
			return std::tuple_cat(
			    std::tuple<BlockLock>((BlockLock{chunks[I]->m_block_mutex, std::defer_lock}, ...)),
			    std::tuple<SunlightLock>((SunlightLock{chunks[I]->m_sunlight_mutex, std::defer_lock}, ...)),
			    fetch_locks<ArgTypes...>(std::forward<decltype(args)>(args)...));
		}(std::make_index_sequence<Extent>{});
	}
}

template <ChunkLockType Type, ChunkLockType... ArgTypes>
auto Chunk::fetch_locked_chunks(auto &&chunks, auto &&...args) {
	using BlockLock = LockedChunk<Type>::BlockLock;
	using SunlightLock = LockedChunk<Type>::SunlightLock;
	if constexpr (std::is_convertible_v<std::decay_t<decltype(chunks)>, std::shared_ptr<Chunk>>) {
		return std::tuple_cat(
		    std::tuple<LockedChunk<Type>>(LockedChunk<Type>(chunks, BlockLock{chunks->m_block_mutex, std::adopt_lock},
		                                                    SunlightLock{chunks->m_sunlight_mutex, std::adopt_lock})),
		    fetch_locked_chunks<ArgTypes...>(std::forward<decltype(args)>(args)...));
	} else {
		constexpr std::size_t Extent = std::tuple_size_v<std::decay_t<decltype(chunks)>>;
		return [&]<std::size_t... I>(std::index_sequence<I...>) {
			return std::tuple_cat(
			    std::tuple<std::array<LockedChunk<Type>, Extent>>(std::array<LockedChunk<Type>, Extent>{
			        LockedChunk<Type>(chunks[I], BlockLock{chunks[I]->m_block_mutex, std::adopt_lock},
			                          SunlightLock{chunks[I]->m_sunlight_mutex, std::adopt_lock})...}),
			    fetch_locked_chunks<ArgTypes...>(std::forward<decltype(args)>(args)...));
		}(std::make_index_sequence<Extent>{});
	}
}

template <ChunkLockType... Types> auto Chunk::Lock(auto &&...chunks) {
	std::apply([](auto &&...locks) { ((locks.lock(), locks.release()), ...); }, fetch_locks<Types...>(chunks...));
	return fetch_locked_chunks<Types...>(chunks...);
}

} // namespace hc::client

#endif
