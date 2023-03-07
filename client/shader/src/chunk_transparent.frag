#version 450

layout(set = 0, binding = 2) uniform sampler2DArray uBlockTexture;
layout(set = 0, binding = 3) uniform sampler3D uLightmapTexture;

// layout(location = 0) in vec3 vPosition;
layout(location = 1) in flat uint vFace;
layout(location = 2) in float vAO;
layout(location = 3) in float vSunlight;
layout(location = 4) in float vTorchlight;
layout(location = 5) in flat uint vTexture;
layout(location = 6) in vec2 vTexcoord;

layout(location = 0) out vec4 oAccum;
layout(location = 1) out float oReveal;

const vec3 kFaceNormal[6] = {vec3(1, 0, 0),  vec3(-1, 0, 0), vec3(0, 1, 0),
                             vec3(0, -1, 0), vec3(0, 0, 1),  vec3(0, 0, -1)};

#define Z_NEAR 0.01
#define Z_FAR 640.0
float linearize_depth(in const float d) { return Z_NEAR * Z_FAR / (Z_FAR + d * (Z_NEAR - Z_FAR)); }

void main() {
	vec4 tex = texture(uBlockTexture, vec3(vTexcoord, vTexture));
	if (tex.a == 0.0)
		discard;
	vec3 color = tex.rgb;
	color *= vAO * texture(uLightmapTexture, vec3(vTorchlight, vSunlight, 0.0)).xyz;
	color *= max(dot(kFaceNormal[vFace], normalize(vec3(10, 5, 3))), 0) * 0.5 + 0.5;

	float z = linearize_depth(gl_FragCoord.z);
	float weight = pow(tex.a + 0.01, 4.0) +
	               max(1e-2, min(3.0 * 1e3, 100.0 / (1e-5 + pow(abs(z) / 10.0, 3.0) + pow(abs(z) / 200.0, 6.0))));
	weight *= gl_FrontFacing ? 1.0 : 0.25;
	oAccum = vec4(color * tex.a, tex.a) * weight;
	oReveal = tex.a;
}
