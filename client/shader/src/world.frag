#version 450

layout(set = 0, binding = 0) uniform sampler2DArray uBlockTexture;

layout(location = 0) in vec3 vPosition;
layout(location = 1) in flat uint vFace;
layout(location = 2) in float vAO;
layout(location = 3) in float vSunlight;
layout(location = 4) in float vTorchlight;
layout(location = 5) in flat uint vTexture;
layout(location = 6) in vec2 vTexcoord;

layout(location = 0) out vec4 oColor;

const vec3 kFaceNormal[6] = {vec3(1, 0, 0),  vec3(-1, 0, 0), vec3(0, 1, 0),
                             vec3(0, -1, 0), vec3(0, 0, 1),  vec3(0, 0, -1)};

void main() {
	// vec4 tex_color = texture(uTextures[vTexture], vTexcoord);
	vec4 tex = texture(uBlockTexture, vec3(vTexcoord, vTexture));
	if (tex.a == 0.0)
		discard;
	vec3 color = tex.rgb;
	color *= vAO;
	color *= max(dot(kFaceNormal[vFace], normalize(vec3(10, 5, 3))), 0) * 0.5 + 0.5;
	oColor = vec4(pow(color, vec3(1.0 / 2.2)), 1.0);
}
