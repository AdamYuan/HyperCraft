#ifndef LIGHT_HPP
#define LIGHT_HPP

#include <cinttypes>

using LightLvl = uint8_t;

class Light {
private:
	uint8_t m_data;

public:
	inline constexpr Light() : m_data{} {}
	inline constexpr Light(LightLvl sunlight, LightLvl torchlight) : m_data((torchlight << 4u) | sunlight) {}

	inline constexpr LightLvl GetSunlight() const { return m_data & 0xfu; }
	inline void SetSunlight(LightLvl sunlight) { m_data = (m_data & 0xf0u) | sunlight; }
	inline constexpr LightLvl GetTorchlight() const { return m_data >> 4u; }
	inline void SetTorchlight(LightLvl torchlight) { m_data = (m_data & 0xfu) | (torchlight << 4u); }

	inline constexpr uint8_t GetData() const { return m_data; }
	inline void SetData(uint16_t data) { m_data = data; }

	bool operator==(Light r) const { return m_data == r.m_data; }
	bool operator!=(Light r) const { return m_data != r.m_data; }
};

#endif
