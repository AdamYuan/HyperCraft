#version 450

layout(location = 0) out uint vExtent;

layout(push_constant) uniform uuPushConstant {
	ivec4 uPosition_AABBCount;
	uvec2 uAABBs[4];
};

layout(set = 0, binding = 0) uniform uuCamera {
	vec4 uViewPosition;
	mat4 uViewProjection;
};

void main() {
	vec3 translate = vec3(uPosition_AABBCount.xyz) - uViewPosition.xyz;
	uvec2 aabb = uAABBs[gl_VertexIndex];
	vec3 aabb_min = vec3(uvec3(aabb.x & 0xffu, (aabb.x >> 8u) & 0xffu, aabb.x >> 16u)) * 0.0625;

	gl_Position = vec4(aabb_min + translate, 1.0f);
	vExtent = aabb.y - aabb.x;
}
