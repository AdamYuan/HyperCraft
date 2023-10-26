#ifndef HYPERCRAFT_CLIENT_CAMERA_HPP
#define HYPERCRAFT_CLIENT_CAMERA_HPP

#include <client/Config.hpp>

#include <myvk/Buffer.hpp>
#include <myvk/DescriptorSet.hpp>

#include <cinttypes>
#include <glm/glm.hpp>

#define PIF 3.14159265358979323846f

struct GLFWwindow;

namespace hc::client {

class Camera {
public:
	glm::vec3 m_position{0.0f, 0.0f, 0.0f};
	float m_yaw{0.0f}, m_pitch{0.0f};
	float m_sensitive{0.005f}, m_speed{2.0f}, m_fov{PIF / 3.0f},
	    m_aspect_ratio{float(kDefaultWidth) / float(kDefaultHeight)};

	struct UniformData {
		glm::vec4 view_position;
		glm::mat4 view_projection;
	};

private:
	glm::dvec2 m_last_mouse_pos{0.0, 0.0};

	void move_forward(float dist, float dir);
	[[nodiscard]] glm::mat4 fetch_matrix() const;

public:
	inline static std::shared_ptr<Camera> Create() { return std::make_shared<Camera>(); }
	[[nodiscard]] inline glm::vec3 GetViewDirection() const {
		float xz_len = glm::cos(m_pitch);
		return glm::vec3{-xz_len * glm::sin(m_yaw), glm::sin(m_pitch), -xz_len * glm::cos(m_yaw)};
	}

	void DragControl(GLFWwindow *window, double delta);
	void MoveControl(GLFWwindow *window, double delta);
	void Update(UniformData *p_data);
};

} // namespace hc::client

#endif
