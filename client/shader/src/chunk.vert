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

layout(set = 1, binding = 0) uniform uuCamera { mat4 uViewProjection; };
struct MeshInfo {
	uint index_count, first_index, vertex_offset;
	float aabb_min_x, aabb_min_y, aabb_min_z, aabb_max_x, aabb_max_y, aabb_max_z;
	int base_pos_x, base_pos_y, base_pos_z;
	uint transparent;
};
layout(std430, set = 2, binding = 0) readonly buffer uuMeshInfo { MeshInfo uMeshInfo[]; };

const float kAOCurve[4] = {0.54, 0.7569, 0.87, 1.0};
const float kSunlightCurve[16] = {0.000000, 0.066667, 0.133333, 0.200000, 0.266667, 0.333333, 0.400000, 0.466667,
                                  0.533333, 0.600000, 0.666667, 0.733333, 0.800000, 0.866667, 0.933333, 1.000000};
const float kTorchlightCurve[16] = {0.000000, 0.100000, 0.200000, 0.300000, 0.400000, 0.500000, 0.600000, 0.700000,
                                    0.800000, 0.900000, 1.000000, 1.100000, 1.200000, 1.300000, 1.400000, 1.500000};

void main() {
	uint x10_y10_z10 = aVertData.x, tex8_face3_ao2_sl4_tl4 = aVertData.y;

	vec3 pos;
	pos.x = int(x10_y10_z10 & 0x3ffu) * 0.0625;
	x10_y10_z10 >>= 10u;
	pos.y = int(x10_y10_z10 & 0x3ffu) * 0.0625;
	x10_y10_z10 >>= 10u;
	pos.z = int(x10_y10_z10 & 0x3ffu) * 0.0625;

	gl_Position = uViewProjection * vec4(pos + vec3(uMeshInfo[gl_InstanceIndex].base_pos_x, //
	                                                uMeshInfo[gl_InstanceIndex].base_pos_y, //
	                                                uMeshInfo[gl_InstanceIndex].base_pos_z),
	                                     1.0f);

	vTexture = tex8_face3_ao2_sl4_tl4 & 0xffu;
	tex8_face3_ao2_sl4_tl4 >>= 8u;

	vFace = tex8_face3_ao2_sl4_tl4 & 0x7u;
	tex8_face3_ao2_sl4_tl4 >>= 3u;

	vTexcoord = (vFace & 0x6u) == 0u ? -pos.zy : ((vFace & 0x6u) == 0x2u ? pos.xz : -pos.xy);

	vAO = kAOCurve[tex8_face3_ao2_sl4_tl4 & 0x3u];
	tex8_face3_ao2_sl4_tl4 >>= 2u;

	vSunlight = kSunlightCurve[tex8_face3_ao2_sl4_tl4 & 0xfu];
	tex8_face3_ao2_sl4_tl4 >>= 4u;
	vTorchlight = kTorchlightCurve[tex8_face3_ao2_sl4_tl4 & 0xfu];
}
