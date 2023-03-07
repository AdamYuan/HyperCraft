#ifndef CUBECRAFT3_CLIENT_CAMERA_HPP
#define CUBECRAFT3_CLIENT_CAMERA_HPP

#include <client/Config.hpp>

#include <myvk/Buffer.hpp>
#include <myvk/DescriptorSet.hpp>

#include <cinttypes>
#include <glm/glm.hpp>

#define PIF 3.14159265358979323846f

struct GLFWwindow;

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
	glm::mat4 fetch_matrix() const;

public:
	inline static std::shared_ptr<Camera> Create() { return std::make_shared<Camera>(); }

	void Control(GLFWwindow *window, double delta);
	void Update(UniformData *p_data);
};

#endif
