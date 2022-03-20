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

private:
	glm::dvec2 m_last_mouse_pos{0.0, 0.0};

	std::shared_ptr<myvk::DescriptorPool> m_descriptor_pool;
	std::shared_ptr<myvk::DescriptorSetLayout> m_descriptor_set_layout;
	std::shared_ptr<myvk::Buffer> m_uniform_buffers[kFrameCount];
	std::shared_ptr<myvk::DescriptorSet> m_descriptor_sets[kFrameCount];

	struct UniformData {
		glm::mat4 m_view_projection;
	};
	void move_forward(float dist, float dir);
	glm::mat4 fetch_matrix() const;

public:
	static std::shared_ptr<Camera> Create(const std::shared_ptr<myvk::Device> &device);

	void Control(GLFWwindow *window, float delta);
	void Update(uint32_t current_frame);

	const std::shared_ptr<myvk::DescriptorSetLayout> &GetDescriptorSetLayout() const { return m_descriptor_set_layout; }
	const std::shared_ptr<myvk::DescriptorSet> &GetFrameDescriptorSet(uint32_t current_frame) const {
		return m_descriptor_sets[current_frame];
	}
};

#endif
