#version 450

out gl_PerVertex { vec4 gl_Position; };

layout(location = 0) in uvec2 aVertData;

// layout(location = 0) out vec3 vPosition;
layout(location = 1) out flat uint vFace;
layout(location = 2) out float vAO;
layout(location = 3) out float vSunlight;
layout(location = 4) out float vTorchlight;
layout(location = 5) out flat uint vTexture;
layout(location = 6) out vec2 vTexcoord;

layout(set = 1, binding = 0) uniform uuCamera {
	vec4 uViewPosition;
	mat4 uViewProjection;
};
struct MeshInfo {
	uint index_count, first_index, vertex_offset;
	float aabb_min_x, aabb_min_y, aabb_min_z, aabb_max_x, aabb_max_y, aabb_max_z;
	int base_pos_x, base_pos_y, base_pos_z;
	uint transparent;
};
layout(std430, set = 2, binding = 0) readonly buffer uuMeshInfo { MeshInfo uMeshInfo[]; };

const float kAOCurve[4] = {0.54, 0.7569, 0.87, 1.0};

void main() {
	uint x10_y10_z10_axis2 = aVertData.x, tex10_trans3_face3_ao2_sl6_tl6 = aVertData.y;

	vec3 pos;
	pos.x = int(x10_y10_z10_axis2 & 0x3ffu) * 0.0625;
	x10_y10_z10_axis2 >>= 10u;
	pos.y = int(x10_y10_z10_axis2 & 0x3ffu) * 0.0625;
	x10_y10_z10_axis2 >>= 10u;
	pos.z = int(x10_y10_z10_axis2 & 0x3ffu) * 0.0625;
	x10_y10_z10_axis2 >>= 10u;

	vTexcoord = x10_y10_z10_axis2 == 0u ? -pos.zy : (x10_y10_z10_axis2 == 1u ? pos.xz : -pos.xy);
	vec3 translate = vec3(uMeshInfo[gl_InstanceIndex].base_pos_x, //
	                      uMeshInfo[gl_InstanceIndex].base_pos_y, //
	                      uMeshInfo[gl_InstanceIndex].base_pos_z) //
	                 - uViewPosition.xyz;
	gl_Position = uViewProjection * vec4(pos + translate, 1.0f);

	vTexture = tex10_trans3_face3_ao2_sl6_tl6 & 0x3ffu;
	tex10_trans3_face3_ao2_sl6_tl6 >>= 10u;

	// Texture transformation
	if ((tex10_trans3_face3_ao2_sl6_tl6 & 1u) != 0)
		vTexcoord = vTexcoord.yx;
	if ((tex10_trans3_face3_ao2_sl6_tl6 & 2u) != 0)
		vTexcoord.x = -vTexcoord.x;
	if ((tex10_trans3_face3_ao2_sl6_tl6 & 4u) != 0)
		vTexcoord.y = -vTexcoord.y;
	tex10_trans3_face3_ao2_sl6_tl6 >>= 3u;

	vFace = tex10_trans3_face3_ao2_sl6_tl6 & 0x7u;
	tex10_trans3_face3_ao2_sl6_tl6 >>= 3u;

	vAO = kAOCurve[tex10_trans3_face3_ao2_sl6_tl6 & 0x3u];
	tex10_trans3_face3_ao2_sl6_tl6 >>= 2u;

	vSunlight = float(tex10_trans3_face3_ao2_sl6_tl6 & 0x3fu) / 63.0;
	tex10_trans3_face3_ao2_sl6_tl6 >>= 6u;
	vTorchlight = float(tex10_trans3_face3_ao2_sl6_tl6 & 0x3fu) / 63.0;
}
