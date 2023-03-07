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

layout(location = 0) out vec3 oOpaque;

const vec3 kFaceNormal[6] = {vec3(1, 0, 0),  vec3(-1, 0, 0), vec3(0, 1, 0),
                             vec3(0, -1, 0), vec3(0, 0, 1),  vec3(0, 0, -1)};

#define DISCARD_THRESHOLD 0.0625
void main() {
	vec4 tex = texture(uBlockTexture, vec3(vTexcoord, vTexture));
	if (tex.a <= DISCARD_THRESHOLD ||
	    (tex.a != 1.0 && textureLod(uBlockTexture, vec3(vTexcoord, vTexture), 0.0).a <= DISCARD_THRESHOLD))
		discard;
	vec3 color = tex.rgb;
	color *= vAO * texture(uLightmapTexture, vec3(vTorchlight, vSunlight, 0.0)).xyz;
	color *= max(dot(kFaceNormal[vFace], normalize(vec3(10, 5, 3))), 0) * 0.5 + 0.5;
	oOpaque = vec3(color);
}
