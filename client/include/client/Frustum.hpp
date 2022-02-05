#ifndef CUBECRAFT3_CLIENT_FRUSTUM_HPP
#define CUBECRAFT3_CLIENT_FRUSTUM_HPP

#include <common/AABB.hpp>
#include <glm/gtc/matrix_access.hpp>

class Frustum {
private:
	glm::vec4 m_planes[5];

public:
	inline void Update(const glm::mat4 &matrix);
	inline bool Cull(const fAABB &aabb) const;
};

void Frustum::Update(const glm::mat4 &matrix) {
	glm::mat4 tmat = glm::transpose(matrix); // make it row-major
	m_planes[0] = tmat[3] - tmat[0];
	m_planes[1] = tmat[3] + tmat[0];
	m_planes[2] = tmat[3] - tmat[1];
	m_planes[3] = tmat[3] + tmat[1];
	m_planes[4] = tmat[3] - tmat[2];
	// m_planes[5] = tmat[3] + tmat[2];
	m_planes[0] /= glm::length(m_planes[0].xyz());
	m_planes[1] /= glm::length(m_planes[1].xyz());
	m_planes[2] /= glm::length(m_planes[2].xyz());
	m_planes[3] /= glm::length(m_planes[3].xyz());
	m_planes[4] /= glm::length(m_planes[4].xyz());
	// m_planes[5] /= glm::length(m_planes[5].xyz());
}

bool Frustum::Cull(const fAABB &aabb) const {
	glm::vec3 axis;
	for (const glm::vec4 &plane : m_planes) {
		axis.x = plane.x < 0 ? aabb.GetMin().x : aabb.GetMax().x;
		axis.y = plane.y < 0 ? aabb.GetMin().y : aabb.GetMax().y;
		axis.z = plane.z < 0 ? aabb.GetMin().z : aabb.GetMax().z;
		if (glm::dot(plane.xyz(), axis) + plane.w < 0.0f)
			return true;
	}
	return false;
}

#endif
