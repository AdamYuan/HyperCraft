#version 450

out gl_PerVertex { vec4 gl_Position; };

layout(location = 0) in uvec2 aVertData;

layout(location = 0) out vec3 vPosition;
layout(location = 1) out flat uint vFace;
layout(location = 2) out float vAO;
layout(location = 3) out float vSunlight;
layout(location = 4) out float vTorchlight;
layout(location = 5) out flat uint vTexture;
layout(location = 6) out vec2 vTexcoord;

layout(set = 1, binding = 0) uniform uuCamera { mat4 uViewProjection; };

const float kAOCurve[4] = {0.54, 0.7569, 0.87, 1.0};
const float kSunlightCurve[16] = {0.000000, 0.066667, 0.133333, 0.200000, 0.266667, 0.333333, 0.400000, 0.466667,
                                  0.533333, 0.600000, 0.666667, 0.733333, 0.800000, 0.866667, 0.933333, 1.000000};
const float kTorchlightCurve[16] = {0.000000, 0.100000, 0.200000, 0.300000, 0.400000, 0.500000, 0.600000, 0.700000,
                                    0.800000, 0.900000, 1.000000, 1.100000, 1.200000, 1.300000, 1.400000, 1.500000};

layout(push_constant) uniform uuPushConstant { int uBaseX, uBaseY, uBaseZ; };

void main() {
	uint x5_y5_z5_face3_ao2_sl4_tl4 = aVertData.x, tex8_u5_v5 = aVertData.y;

	vPosition.x = int(x5_y5_z5_face3_ao2_sl4_tl4 & 0x1fu) + uBaseX;
	x5_y5_z5_face3_ao2_sl4_tl4 >>= 5u;
	vPosition.y = int(x5_y5_z5_face3_ao2_sl4_tl4 & 0x1fu) + uBaseY;
	x5_y5_z5_face3_ao2_sl4_tl4 >>= 5u;
	vPosition.z = int(x5_y5_z5_face3_ao2_sl4_tl4 & 0x1fu) + uBaseZ;
	x5_y5_z5_face3_ao2_sl4_tl4 >>= 5u;

	gl_Position = uViewProjection * vec4(vPosition, 1.0f);

	vFace = x5_y5_z5_face3_ao2_sl4_tl4 & 0x7u;
	x5_y5_z5_face3_ao2_sl4_tl4 >>= 3u;

	vAO = kAOCurve[x5_y5_z5_face3_ao2_sl4_tl4 & 0x3u];
	x5_y5_z5_face3_ao2_sl4_tl4 >>= 2u;

	vSunlight = kSunlightCurve[x5_y5_z5_face3_ao2_sl4_tl4 & 0xfu];
	x5_y5_z5_face3_ao2_sl4_tl4 >>= 4u;
	vTorchlight = kTorchlightCurve[x5_y5_z5_face3_ao2_sl4_tl4 & 0xfu];

	vTexture = tex8_u5_v5 & 0xffu;
	tex8_u5_v5 >>= 8u;

	vTexcoord.x = tex8_u5_v5 & 0x1fu;
	tex8_u5_v5 >>= 5u;
	vTexcoord.y = tex8_u5_v5 & 0x1fu;
}
