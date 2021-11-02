#ifndef CUBECRAFT3_COMMON_BYTE_STREAM_HPP
#define CUBECRAFT3_COMMON_BYTE_STREAM_HPP

class InputByteStream {
private:
	std::vector<uint8_t> m_data{};

public:
	inline const std::vector<uint8_t> &GetData() const { return m_data; }
	inline size_t Size() const { return m_data.size(); }
	template <typename T> inline typename std::enable_if<std::is_arithmetic<T>::value, void>::type Push(T x) {
		union {
			T data;
			uint8_t bytes[sizeof(T)];
		} u = {x};
#ifdef IS_BIG_ENDIAN // reverse bytes in big endian machine
		std::reverse(u.bytes, u.bytes + sizeof(T));
#endif
		m_data.insert(m_data.end(), u.bytes, u.bytes + sizeof(T));
	}
	inline void Push(std::string_view str) {
		m_data.insert(m_data.end(), (uint8_t *)str.data(), (uint8_t *)str.data() + str.length());
		m_data.push_back(0);
	}
};

class OutputByteStream {
private:
	const uint8_t *m_begin{}, *m_end{};

public:
	inline OutputByteStream(const uint8_t *begin, const uint8_t *end) : m_begin{begin}, m_end{end} {}
	inline size_t Size() const { return m_end - m_begin; }
	template <typename T> inline typename std::enable_if<std::is_arithmetic<T>::value, T>::type Pop() {
		union {
			T data;
			uint8_t bytes[sizeof(T)];
		} u;
		std::copy(m_begin, m_begin + sizeof(T), u.bytes);
#ifdef IS_BIG_ENDIAN // reverse bytes in big endian machine
		std::reverse(u.bytes, u.bytes + sizeof(T));
#endif
		m_begin += sizeof(T);
		return u.data;
	}
	template <typename T> inline typename std::enable_if<std::is_same<T, std::string>::value, std::string>::type Pop() {
		std::string ret = {(char *)m_begin};
		m_begin += ret.length() + 1;
		return ret;
	}
};

#endif
