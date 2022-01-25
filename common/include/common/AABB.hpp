#ifndef CUBECRAFT3_COMMON_AABB_HPP
#define CUBECRAFT3_COMMON_AABB_HPP

#include <cinttypes>
#include <glm/glm.hpp>
#include <numeric>
#include <type_traits>

template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>> class AABB {
private:
	glm::vec<3, T> m_min{std::numeric_limits<T>::max()}, m_max{std::numeric_limits<T>::min()};

public:
	inline AABB() = default;
	inline AABB(const glm::vec<3, T> &min, const glm::vec<3, T> &max) : m_min{min}, m_max{max} {}
	inline AABB(const AABB<T> &r) : m_min{r.m_min}, m_max{r.m_max} {}
	AABB &operator=(const AABB<T> &r) {
		m_min = r.m_min;
		m_max = r.m_max;
		return *this;
	}

	template <typename M> inline explicit operator AABB<M>() const {
		return {(glm::vec<3, M>)m_min, (glm::vec<3, M>)m_max};
	}

	AABB &operator+=(const glm::vec<3, T> &r) {
		m_min += r;
		m_max += r;
		return *this;
	}
	AABB operator+(const glm::vec<3, T> &r) const { return {m_min + r, m_max + r}; }

	inline const glm::vec<3, T> &GetMin() const { return m_min; }
	inline glm::vec<3, T> &GetMin() { return m_min; }

	inline const glm::vec<3, T> &GetMax() const { return m_max; }
	inline glm::vec<3, T> &GetMax() { return m_max; }

	inline void Merge(const glm::vec<3, T> &p) {
		m_min = glm::min(m_min, p);
		m_max = glm::max(m_max, p);
	}
	inline void Merge(const AABB<T> &aabb) {
		m_min = glm::min(m_min, aabb.m_min);
		m_max = glm::max(m_max, aabb.m_max);
	}
};

using fAABB = AABB<float>;
using u32AABB = AABB<uint32_t>;
using i32AABB = AABB<int32_t>;
using u16AABB = AABB<uint16_t>;
using i16AABB = AABB<int16_t>;
using u8AABB = AABB<uint8_t>;
using i8AABB = AABB<int8_t>;

#endif
