#include <client/Camera.hpp>

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_internal.h>

std::shared_ptr<Camera> Camera::Create(const std::shared_ptr<myvk::Device> &device) {
	std::shared_ptr<Camera> ret = std::make_shared<Camera>();
	ret->m_descriptor_pool =
	    myvk::DescriptorPool::Create(device, kFrameCount, {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, kFrameCount}});
	{
		VkDescriptorSetLayoutBinding camera_binding = {};
		camera_binding.binding = 0;
		camera_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		camera_binding.descriptorCount = 1;
		camera_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

		ret->m_descriptor_set_layout = myvk::DescriptorSetLayout::Create(device, {camera_binding});
	}

	for (uint32_t i = 0; i < kFrameCount; ++i) {
		ret->m_uniform_buffers[i] =
		    myvk::Buffer::Create(device, sizeof(UniformData),
		                         VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
		                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO);
		ret->m_descriptor_sets[i] = myvk::DescriptorSet::Create(ret->m_descriptor_pool, ret->m_descriptor_set_layout);
		ret->m_descriptor_sets[i]->UpdateUniformBuffer(ret->m_uniform_buffers[i], 0);
	}

	return ret;
}

void Camera::move_forward(float dist, float dir) {
	m_position.x -= glm::sin(m_yaw + dir) * dist;
	m_position.z -= glm::cos(m_yaw + dir) * dist;
}

void Camera::Control(GLFWwindow *window, float delta) {
	glm::dvec2 cur_pos;
	glfwGetCursorPos(window, &cur_pos.x, &cur_pos.y);

	if (!ImGui::GetCurrentContext()->NavWindow ||
	    (ImGui::GetCurrentContext()->NavWindow->Flags & ImGuiWindowFlags_NoBringToFrontOnFocus)) {
		float speed = delta * m_speed;
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			move_forward(speed, 0.0f);
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			move_forward(speed, PIF * 0.5f);
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			move_forward(speed, -PIF * 0.5f);
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			move_forward(speed, PIF);
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
			m_position.y += speed;
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			m_position.y -= speed;

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
			glfwGetCursorPos(window, &cur_pos.x, &cur_pos.y);
			float offset_x = float(cur_pos.x - m_last_mouse_pos.x) * m_sensitive;
			float offset_y = float(cur_pos.y - m_last_mouse_pos.y) * m_sensitive;

			m_yaw -= offset_x;
			m_pitch -= offset_y;

			m_pitch = glm::clamp(m_pitch, -PIF * 0.5f, PIF * 0.5f);
			m_yaw = glm::mod(m_yaw, PIF * 2);
		}
	}
	m_last_mouse_pos = cur_pos;
}

glm::mat4 Camera::fetch_matrix() const {
	glm::mat4 ret = glm::perspective(m_fov, m_aspect_ratio, kCameraNear, kCameraFar);
	ret[1][1] *= -1;
	ret = glm::rotate(ret, -m_pitch, glm::vec3(1.0f, 0.0f, 0.0f));
	ret = glm::rotate(ret, -m_yaw, glm::vec3(0.0f, 1.0f, 0.0f));
	// ret = glm::translate(ret, -m_position);
	return ret;
}

void Camera::Update(uint32_t current_frame) {
	glm::mat4 view_projection = fetch_matrix();
	m_uniform_buffers[current_frame]->UpdateData((UniformData){glm::vec4(m_position, 0.0), view_projection});
}
