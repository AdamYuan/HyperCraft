#version 450

layout(location = 0) out uint vExtent;

layout(push_constant) uniform uuPushConstant {
	ivec3 uPosition;
	uint uAABBs[4];
};

layout(set = 0, binding = 0) uniform uuCamera {
	vec4 uViewPosition;
	mat4 uViewProjection;
};

void main() {
	vec3 translate = vec3(uPosition) - uViewPosition.xyz;
	uint aabb = uAABBs[gl_VertexIndex];
	vec3 aabb_min = vec3(uvec3(aabb & 0x1fu, (aabb >> 5u) & 0x1fu, (aabb >> 10u) & 0x1fu)) * 0.0625;
	gl_Position = vec4(aabb_min + translate, 1.0f);
	vExtent = (aabb >> 16u) - (aabb & 0xffffu);
}
